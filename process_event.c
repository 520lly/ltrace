#if HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>

#include "ltrace.h"
#include "output.h"
#include "options.h"
#include "elf.h"
#include "debug.h"

#ifdef __powerpc__
#include <sys/ptrace.h>
#endif

static void process_signal(struct event *event);
static void process_exit(struct event *event);
static void process_exit_signal(struct event *event);
static void process_syscall(struct event *event);
static void process_arch_syscall(struct event *event);
static void process_sysret(struct event *event);
static void process_arch_sysret(struct event *event);
static void process_fork(struct event *event);
static void process_clone(struct event *event);
static void process_exec(struct event *event);
static void process_breakpoint(struct event *event);
static void remove_proc(Process *proc);

static void callstack_push_syscall(Process *proc, int sysnum);
static void callstack_push_symfunc(Process *proc,
				   struct library_symbol *sym);
static void callstack_pop(Process *proc);

static char *
shortsignal(Process *proc, int signum) {
	static char *signalent0[] = {
#include "signalent.h"
	};
	static char *signalent1[] = {
#include "signalent1.h"
	};
	static char **signalents[] = { signalent0, signalent1 };
	int nsignals[] = { sizeof signalent0 / sizeof signalent0[0],
		sizeof signalent1 / sizeof signalent1[0]
	};

	if (proc->personality > sizeof signalents / sizeof signalents[0])
		abort();
	if (signum < 0 || signum >= nsignals[proc->personality]) {
		return "UNKNOWN_SIGNAL";
	} else {
		return signalents[proc->personality][signum];
	}
}

static char *
sysname(Process *proc, int sysnum) {
	static char result[128];
	static char *syscalent0[] = {
#include "syscallent.h"
	};
	static char *syscalent1[] = {
#include "syscallent1.h"
	};
	static char **syscalents[] = { syscalent0, syscalent1 };
	int nsyscals[] = { sizeof syscalent0 / sizeof syscalent0[0],
		sizeof syscalent1 / sizeof syscalent1[0]
	};

	if (proc->personality > sizeof syscalents / sizeof syscalents[0])
		abort();
	if (sysnum < 0 || sysnum >= nsyscals[proc->personality]) {
		sprintf(result, "SYS_%d", sysnum);
		return result;
	} else {
		sprintf(result, "SYS_%s",
			syscalents[proc->personality][sysnum]);
		return result;
	}
}

static char *
arch_sysname(Process *proc, int sysnum) {
	static char result[128];
	static char *arch_syscalent[] = {
#include "arch_syscallent.h"
	};
	int nsyscals = sizeof arch_syscalent / sizeof arch_syscalent[0];

	if (sysnum < 0 || sysnum >= nsyscals) {
		sprintf(result, "ARCH_%d", sysnum);
		return result;
	} else {
		sprintf(result, "ARCH_%s",
				arch_syscalent[sysnum]);
		return result;
	}
}

void
process_event(struct event *event) {
	switch (event->thing) {
	case EVENT_NONE:
		debug(1, "event: none");
		return;
	case EVENT_SIGNAL:
		debug(1, "event: signal (%s [%d])",
		      shortsignal(event->proc, event->e_un.signum),
		      event->e_un.signum);
		process_signal(event);
		return;
	case EVENT_EXIT:
		debug(1, "event: exit (%d)", event->e_un.ret_val);
		process_exit(event);
		return;
	case EVENT_EXIT_SIGNAL:
		debug(1, "event: exit signal (%s [%d])",
		      shortsignal(event->proc, event->e_un.signum),
		      event->e_un.signum);
		process_exit_signal(event);
		return;
	case EVENT_SYSCALL:
		debug(1, "event: syscall (%s [%d])",
		      sysname(event->proc, event->e_un.sysnum),
		      event->e_un.sysnum);
		process_syscall(event);
		return;
	case EVENT_SYSRET:
		debug(1, "event: sysret (%s [%d])",
		      sysname(event->proc, event->e_un.sysnum),
		      event->e_un.sysnum);
		process_sysret(event);
		return;
	case EVENT_ARCH_SYSCALL:
		debug(1, "event: arch_syscall (%s [%d])",
				arch_sysname(event->proc, event->e_un.sysnum),
				event->e_un.sysnum);
		process_arch_syscall(event);
		return;
	case EVENT_ARCH_SYSRET:
		debug(1, "event: arch_sysret (%s [%d])",
				arch_sysname(event->proc, event->e_un.sysnum),
				event->e_un.sysnum);
		process_arch_sysret(event);
		return;
	case EVENT_FORK:
		debug(1, "event: fork (%u)", event->e_un.newpid);
		process_fork(event);
		return;
	case EVENT_CLONE:
		debug(1, "event: clone (%u)", event->e_un.newpid);
		process_clone(event);
		return;
	case EVENT_EXEC:
		debug(1, "event: exec()");
		process_exec(event);
		return;
	case EVENT_BREAKPOINT:
		debug(1, "event: breakpoint");
		process_breakpoint(event);
		return;
	default:
		fprintf(stderr, "Error! unknown event?\n");
		exit(1);
	}
}

static void
process_signal(struct event *event) {
	if (exiting && event->e_un.signum == SIGSTOP) {
		pid_t pid = event->proc->pid;
		disable_all_breakpoints(event->proc);
		untrace_pid(pid);
		remove_proc(event->proc);
		return;
	}
	output_line(event->proc, "--- %s (%s) ---",
		    shortsignal(event->proc, event->e_un.signum),
		    strsignal(event->e_un.signum));
	continue_after_signal(event->proc->pid, event->e_un.signum);
}

static void
process_exit(struct event *event) {
	output_line(event->proc, "+++ exited (status %d) +++",
		    event->e_un.ret_val);
	remove_proc(event->proc);
}

static void
process_exit_signal(struct event *event) {
	output_line(event->proc, "+++ killed by %s +++",
		    shortsignal(event->proc, event->e_un.signum));
	remove_proc(event->proc);
}

static void
remove_proc(Process *proc) {
	Process *tmp, *tmp2;

	debug(1, "Removing pid %u\n", proc->pid);

	if (list_of_processes == proc) {
		tmp = list_of_processes;
		list_of_processes = list_of_processes->next;
		free(tmp);
		return;
	}
	tmp = list_of_processes;
	while (tmp->next) {
		if (tmp->next == proc) {
			tmp2 = tmp->next;
			tmp->next = tmp->next->next;
			free(tmp2);
			continue;
		}
		tmp = tmp->next;
	}
}

static void
process_syscall(struct event *event) {
	if (options.syscalls) {
		output_left(LT_TOF_SYSCALL, event->proc,
			    sysname(event->proc, event->e_un.sysnum));
	}
	if (fork_p(event->proc, event->e_un.sysnum)) {
		disable_all_breakpoints(event->proc);
	} else if (event->proc->breakpoints_enabled == 0) {
		enable_all_breakpoints(event->proc);
	}
	callstack_push_syscall(event->proc, event->e_un.sysnum);
	continue_process(event->proc->pid);
}

static void
process_fork(struct event * event) {
	output_line(event->proc, "--- fork() = %u ---",
			event->e_un.newpid);
	continue_process(event->proc->pid);
}

static void
process_clone(struct event * event) {
	output_line(event->proc, "--- clone() = %u ---",
			event->e_un.newpid);
	abort();
}

static void
process_exec(struct event * event) {
	output_line(event->proc, "--- exec() ---");
	abort();
}

static void
process_arch_syscall(struct event *event) {
	if (options.syscalls) {
		output_left(LT_TOF_SYSCALL, event->proc,
				arch_sysname(event->proc, event->e_un.sysnum));
	}
	if (event->proc->breakpoints_enabled == 0) {
		enable_all_breakpoints(event->proc);
	}
	callstack_push_syscall(event->proc, 0xf0000 + event->e_un.sysnum);
	continue_process(event->proc->pid);
}

struct timeval current_time_spent;

static void
calc_time_spent(Process *proc) {
	struct timeval tv;
	struct timezone tz;
	struct timeval diff;
	struct callstack_element *elem;

	elem = &proc->callstack[proc->callstack_depth - 1];

	gettimeofday(&tv, &tz);

	diff.tv_sec = tv.tv_sec - elem->time_spent.tv_sec;
	if (tv.tv_usec >= elem->time_spent.tv_usec) {
		diff.tv_usec = tv.tv_usec - elem->time_spent.tv_usec;
	} else {
		diff.tv_sec++;
		diff.tv_usec = 1000000 + tv.tv_usec - elem->time_spent.tv_usec;
	}
	current_time_spent = diff;
}

static void
process_sysret(struct event *event) {
	if (opt_T || options.summary) {
		calc_time_spent(event->proc);
	}
	if (fork_p(event->proc, event->e_un.sysnum)) {
		if (options.follow) {
			arg_type_info info;
			info.type = ARGTYPE_LONG;
			pid_t child =
			    gimme_arg(LT_TOF_SYSCALLR, event->proc, -1, &info);
			if (child > 0) {
				open_pid(child, 0);
			}
		}
		enable_all_breakpoints(event->proc);
	}
	callstack_pop(event->proc);
	if (options.syscalls) {
		output_right(LT_TOF_SYSCALLR, event->proc,
			     sysname(event->proc, event->e_un.sysnum));
	}
	continue_process(event->proc->pid);
}

static void
process_arch_sysret(struct event *event) {
	if (opt_T || options.summary) {
		calc_time_spent(event->proc);
	}
	callstack_pop(event->proc);
	if (options.syscalls) {
		output_right(LT_TOF_SYSCALLR, event->proc,
				arch_sysname(event->proc, event->e_un.sysnum));
	}
	continue_process(event->proc->pid);
}

static void
process_breakpoint(struct event *event) {
	int i, j;
	Breakpoint *sbp;

	debug(2, "event: breakpoint (%p)", event->e_un.brk_addr);

#ifdef __powerpc__
	/* Need to skip following NOP's to prevent a fake function from being stacked.  */
	long stub_addr = (long) get_count_register(event->proc);
	Breakpoint *stub_bp = NULL;
	char nop_instruction[] = PPC_NOP;

	stub_bp = address2bpstruct (event->proc, event->e_un.brk_addr);

	if (stub_bp) {
		unsigned char *bp_instruction = stub_bp->orig_value;

		if (memcmp(bp_instruction, nop_instruction,
			    PPC_NOP_LENGTH) == 0) {
			if (stub_addr != (long) event->e_un.brk_addr) {
				set_instruction_pointer (event->proc, event->e_un.brk_addr + 4);
				continue_process(event->proc->pid);
				return;
			}
		}
	}
#endif
	if ((sbp = event->proc->breakpoint_being_enabled) != 0) {
		/* Reinsert breakpoint */
		continue_enabling_breakpoint(event->proc->pid,
					     event->proc->
					     breakpoint_being_enabled);
		event->proc->breakpoint_being_enabled = NULL;
		return;
	}

	for (i = event->proc->callstack_depth - 1; i >= 0; i--) {
		if (event->e_un.brk_addr ==
		    event->proc->callstack[i].return_addr) {
#ifdef __powerpc__
			/*
			 * PPC HACK! (XXX FIXME TODO)
			 * The PLT gets modified during the first call,
			 * so be sure to re-enable the breakpoint.
			 */
			unsigned long a;
			struct library_symbol *libsym =
			    event->proc->callstack[i].c_un.libfunc;
			void *addr = sym2addr(event->proc, libsym);

			if (libsym->plt_type != LS_TOPLT_POINT) {
				unsigned char break_insn[] = BREAKPOINT_VALUE;

				sbp = address2bpstruct(event->proc, addr);
				assert(sbp);
				a = ptrace(PTRACE_PEEKTEXT, event->proc->pid,
					   addr);

				if (memcmp(&a, break_insn, BREAKPOINT_LENGTH)) {
					sbp->enabled--;
					insert_breakpoint(event->proc, addr,
							  libsym);
				}
			} else {
				sbp = libsym->brkpnt;
				assert(sbp);
				if (addr != sbp->addr) {
					insert_breakpoint(event->proc, addr,
							  libsym);
				}
			}
#elif defined(__mips__)
			void *addr;
			void *old_addr;
			struct library_symbol *sym= event->proc->callstack[i].c_un.libfunc;
			assert(sym && sym->brkpnt);
			old_addr=sym->brkpnt->addr;
			addr=sym2addr(event->proc,sym);
			assert(old_addr !=0 && addr !=0);
			if(addr != old_addr){
				struct library_symbol *new_sym;
				new_sym=malloc(sizeof(*new_sym));
				memcpy(new_sym,sym,sizeof(*new_sym));
				new_sym->next=event->proc->list_of_symbols;
				event->proc->list_of_symbols=new_sym;
				new_sym->brkpnt=0;
				insert_breakpoint(event->proc, addr, new_sym);
			}
#endif
			for (j = event->proc->callstack_depth - 1; j > i; j--) {
				callstack_pop(event->proc);
			}
			if (opt_T || options.summary) {
				calc_time_spent(event->proc);
			}
			callstack_pop(event->proc);
			event->proc->return_addr = event->e_un.brk_addr;
			output_right(LT_TOF_FUNCTIONR, event->proc,
				     event->proc->callstack[i].c_un.libfunc->
				     name);
			continue_after_breakpoint(event->proc,
						  address2bpstruct(event->proc,
								   event->e_un.
								   brk_addr));
			return;
		}
	}

	if ((sbp = address2bpstruct(event->proc, event->e_un.brk_addr))) {
		event->proc->stack_pointer = get_stack_pointer(event->proc);
		event->proc->return_addr =
		    get_return_addr(event->proc, event->proc->stack_pointer);
		output_left(LT_TOF_FUNCTION, event->proc, sbp->libsym->name);
		callstack_push_symfunc(event->proc, sbp->libsym);
#ifdef PLT_REINITALISATION_BP
		if (event->proc->need_to_reinitialize_breakpoints
		    && (strcmp(sbp->libsym->name, PLTs_initialized_by_here) ==
			0))
			reinitialize_breakpoints(event->proc);
#endif

		continue_after_breakpoint(event->proc, sbp);
		return;
	}

	output_line(event->proc, "unexpected breakpoint at %p",
		    (void *)event->e_un.brk_addr);
	continue_process(event->proc->pid);
}

static void
callstack_push_syscall(Process *proc, int sysnum) {
	struct callstack_element *elem;

	/* FIXME: not good -- should use dynamic allocation. 19990703 mortene. */
	if (proc->callstack_depth == MAX_CALLDEPTH - 1) {
		fprintf(stderr, "Error: call nesting too deep!\n");
		return;
	}

	elem = &proc->callstack[proc->callstack_depth];
	elem->is_syscall = 1;
	elem->c_un.syscall = sysnum;
	elem->return_addr = NULL;

	proc->callstack_depth++;
	if (opt_T || options.summary) {
		struct timezone tz;
		gettimeofday(&elem->time_spent, &tz);
	}
}

static void
callstack_push_symfunc(Process *proc, struct library_symbol *sym) {
	struct callstack_element *elem;

	/* FIXME: not good -- should use dynamic allocation. 19990703 mortene. */
	if (proc->callstack_depth == MAX_CALLDEPTH - 1) {
		fprintf(stderr, "Error: call nesting too deep!\n");
		return;
	}

	elem = &proc->callstack[proc->callstack_depth];
	elem->is_syscall = 0;
	elem->c_un.libfunc = sym;

	elem->return_addr = proc->return_addr;
	if (elem->return_addr) {
		insert_breakpoint(proc, elem->return_addr, 0);
	}

	proc->callstack_depth++;
	if (opt_T || options.summary) {
		struct timezone tz;
		gettimeofday(&elem->time_spent, &tz);
	}
}

static void
callstack_pop(Process *proc) {
	struct callstack_element *elem;
	assert(proc->callstack_depth > 0);

	elem = &proc->callstack[proc->callstack_depth - 1];
	if (!elem->is_syscall && elem->return_addr) {
		delete_breakpoint(proc, elem->return_addr);
	}
	proc->callstack_depth--;
}
