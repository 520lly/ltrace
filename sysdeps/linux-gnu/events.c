#if HAVE_CONFIG_H
#include "config.h"
#endif

#define	_GNU_SOURCE	1
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"
#include "debug.h"

static Event event;

Event *
next_event(void) {
	pid_t pid;
	int status;
	int tmp;
	int stop_signal;

	if (!list_of_processes) {
		debug(1, "No more children");
		exit(0);
	}
	pid = wait(&status);
	if (pid == -1) {
		if (errno == ECHILD) {
			debug(1, "No more children");
			exit(0);
		} else if (errno == EINTR) {
			debug(1, "wait received EINTR ?");
			event.thing = EVENT_NONE;
			return &event;
		}
		perror("wait");
		exit(1);
	}
	event.proc = pid2proc(pid);
	if (!event.proc) {
		output_line(NULL, "event from wrong pid %u ?!?\n", pid);
//		exit(1);
		event.thing = EVENT_NONE;
		return &event;
	}
	get_arch_dep(event.proc);
	event.proc->instruction_pointer = NULL;
	debug(3, "event from pid %u", pid);
	if (event.proc->breakpoints_enabled == -1) {
		enable_all_breakpoints(event.proc);
		event.thing = EVENT_NONE;
		trace_set_options(event.proc, event.proc->pid);
		continue_process(event.proc->pid);
		return &event;
	}
	if (opt_i) {
		event.proc->instruction_pointer =
		    get_instruction_pointer(event.proc);
	}
	switch (syscall_p(event.proc, status, &tmp)) {
		case 1:
			event.thing = EVENT_SYSCALL;
			event.e_un.sysnum = tmp;
			return &event;
		case 2:
			event.thing = EVENT_SYSRET;
			event.e_un.sysnum = tmp;
			return &event;
		case 3:
			event.thing = EVENT_ARCH_SYSCALL;
			event.e_un.sysnum = tmp;
			return &event;
		case 4:
			event.thing = EVENT_ARCH_SYSRET;
			event.e_un.sysnum = tmp;
			return &event;
		case -1:
			event.thing = EVENT_NONE;
			continue_process(event.proc->pid);
			return &event;
	}
	if (WIFSTOPPED(status) && ((status>>16 == PTRACE_EVENT_FORK) || (status>>16 == PTRACE_EVENT_VFORK) || (status>>16 == PTRACE_EVENT_CLONE))) {
		unsigned long data;
		ptrace(PTRACE_GETEVENTMSG, pid, NULL, &data);
		event.thing = EVENT_FORK;
		event.e_un.newpid = data;
		return &event;
	}
	/* TODO: check for EVENT_CLONE */
	if (WIFSTOPPED(status) && (status>>16 == PTRACE_EVENT_EXEC)) {
		event.thing = EVENT_EXEC;
		return &event;
	}
	if (WIFEXITED(status)) {
		event.thing = EVENT_EXIT;
		event.e_un.ret_val = WEXITSTATUS(status);
		return &event;
	}
	if (WIFSIGNALED(status)) {
		event.thing = EVENT_EXIT_SIGNAL;
		event.e_un.signum = WTERMSIG(status);
		return &event;
	}
	if (!WIFSTOPPED(status)) {
		event.thing = EVENT_NONE;
		return &event;
	}

	stop_signal = WSTOPSIG(status);

	/* On some targets, breakpoints are signalled not using
	   SIGTRAP, but also with SIGILL, SIGSEGV or SIGEMT.  Check
	   for these. */
	if (stop_signal == SIGSEGV
	    || stop_signal == SIGILL
#ifdef SIGEMT
	    || stop_signal == SIGEMT
#endif
	    ) {
		// If we didn't need to know IP so far, get it now.
		void * addr = opt_i
		  ? event.proc->instruction_pointer
		  : (event.proc->instruction_pointer = get_instruction_pointer (event.proc));

		if (address2bpstruct(event.proc, addr))
			stop_signal = SIGTRAP;
	}

	if (stop_signal != (SIGTRAP | event.proc->tracesysgood)
	    && stop_signal != SIGTRAP) {
		event.thing = EVENT_SIGNAL;
		event.e_un.signum = stop_signal;
		return &event;
	}

	if (was_exec(event.proc, status)) {
		pid_t saved_pid;

		event.thing = EVENT_NONE;
		event.e_un.signum = WSTOPSIG(status);
		debug(1, "Placing breakpoints for the new program");
		event.proc->mask_32bit = 0;
		event.proc->personality = 0;
		event.proc->arch_ptr = NULL;
		event.proc->filename = pid2name(event.proc->pid);
		saved_pid = event.proc->pid;
		event.proc->pid = 0;
		breakpoints_init(event.proc);
		event.proc->pid = saved_pid;
		continue_process(event.proc->pid);
		return &event;
	}

	event.thing = EVENT_BREAKPOINT;
	if (!event.proc->instruction_pointer) {
		event.proc->instruction_pointer =
		    get_instruction_pointer(event.proc);
	}
	event.e_un.brk_addr =
	    event.proc->instruction_pointer - DECR_PC_AFTER_BREAK;
	return &event;
}
