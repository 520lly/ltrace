#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int syscall_p(struct process * proc, int status, int * sysnum)
{
	int depth;

	if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGTRAP) {
		*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, 4*ORIG_EAX, 0);
		if (*sysnum>=0) {
			depth = proc->callstack_depth;
			if (depth>0 &&
					proc->callstack[depth-1].is_syscall &&
					proc->callstack[depth-1].c_un.syscall==*sysnum) {
				return 2;
			} else {
				return 1;
			}
		}
	}
	return 0;
}

void continue_after_breakpoint(struct process *proc, struct breakpoint * sbp)
{
	if (sbp->enabled) disable_breakpoint(proc->pid, sbp);
	ptrace(PTRACE_POKEUSER, proc->pid, 4*EIP, sbp->addr);
	if (sbp->enabled == 0) {
		continue_process(proc->pid);
	} else {
		proc->breakpoint_being_enabled = sbp;
		ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0);
	}
}

long gimme_arg(enum tof type, struct process * proc, int arg_num)
{
	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EAX, 0);
	}

	if (type==LT_TOF_FUNCTION) {
		return ptrace(PTRACE_PEEKTEXT, proc->pid, proc->stack_pointer+4*(arg_num+1), 0);
	} else if (type==LT_TOF_SYSCALL) {
#if 0
		switch(arg_num) {
			case 0:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EBX, 0);
			case 1:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*ECX, 0);
			case 2:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EDX, 0);
			case 3:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*ESI, 0);
			case 4:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EDI, 0);
			default:
				fprintf(stderr, "gimme_arg called with wrong arguments\n");
				exit(2);
		}
#else
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4*arg_num, 0);
#endif
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}

	return 0;
}

int umovestr(struct process * proc, void * addr, int len, void * laddr)
{
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
