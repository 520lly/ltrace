#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/unistd.h>

#include "ltrace.h"
#include "options.h"

/* Returns 1 if the sysnum may make a new child to be created
 * (ie, with fork() or clone())
 * Returns 0 otherwise.
 */
int
fork_p(int sysnum) {
	return 0
#if defined(__NR_fork)
		|| (sysnum == __NR_fork)
#endif
#if defined(__NR_clone)
		|| (sysnum == __NR_clone)
#endif
#if defined(__NR_vfork)
		|| (sysnum == __NR_vfork)
#endif
		;
}

/* Returns 1 if the sysnum may make the process exec other program
 */
int
exec_p(int sysnum) {
	return (sysnum == __NR_execve);
}

void
trace_me(void) {
	if (ptrace(PTRACE_TRACEME, 0, 1, 0)<0) {
		perror("PTRACE_TRACEME");
		exit(1);
	}
}

int
trace_pid(pid_t pid) {
	if (ptrace(PTRACE_ATTACH, pid, 1, 0) < 0) {
		return -1;
	}
	return 0;
}

void
untrace_pid(pid_t pid) {
	ptrace(PTRACE_DETACH, pid, 1, 0);
}

void
continue_after_signal(pid_t pid, int signum) {
	/* We should always trace syscalls to be able to control fork(), clone(), execve()... */
	ptrace(PTRACE_SYSCALL, pid, 0, signum);
}

void
continue_process(pid_t pid) {
	continue_after_signal(pid, 0);
}

void
continue_enabling_breakpoint(pid_t pid, struct breakpoint * sbp) {
	enable_breakpoint(pid, sbp);
	continue_process(pid);
}

void
continue_after_breakpoint(struct process *proc, struct breakpoint * sbp) {
	if (sbp->enabled) disable_breakpoint(proc->pid, sbp);
	set_instruction_pointer(proc->pid, sbp->addr);
	if (sbp->enabled == 0) {
		continue_process(proc->pid);
	} else {
		proc->breakpoint_being_enabled = sbp;
		ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0);
	}
}

int
umovestr(struct process * proc, void * addr, int len, void * laddr) {
	long a;
	int i;
	int offset=0;

	while(offset<len) {
		a = ptrace(PTRACE_PEEKTEXT, proc->pid, addr+offset, 0);
		for(i=0; i<sizeof(long); i++) {
			if (((char*)&a)[i] && offset+i < len) {
				*(char *)(laddr+offset+i) = ((char*)&a)[i];
			} else {
				*(char *)(laddr+offset+i) = '\0';
				return 0;
			}
		}
		offset += sizeof(long);
	}
	*(char *)(laddr+offset) = '\0';
	return 0;
}
