#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

#include "ltrace.h"
#include "trace.h"

void insert_breakpoint(int pid, unsigned long addr, unsigned char * value)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	value[0] = a & 0xFF;
	a &= 0xFFFFFF00;
	a |= 0xCC;
	ptrace(PTRACE_POKETEXT, pid, addr, a);
}

void delete_breakpoint(int pid, unsigned long addr, unsigned char * value)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	a &= 0xFFFFFF00;
	a |= value[0];
	ptrace(PTRACE_POKETEXT, pid, addr, a);
}

unsigned long get_eip(int pid)
{
	unsigned long eip;

	eip = ptrace(PTRACE_PEEKUSER, pid, 4*EIP, 0);

	return eip-1;			/* Length of breakpoint (0xCC) is 1 byte */
}

unsigned long get_esp(int pid)
{
	unsigned long esp;

	esp = ptrace(PTRACE_PEEKUSER, pid, 4*UESP, 0);

	return esp;
}

unsigned long get_return(int pid, unsigned long esp)
{
	return ptrace(PTRACE_PEEKTEXT, pid, esp, 0);
}

unsigned long get_arg(int pid, unsigned long esp, int arg_num)
{
	return ptrace(PTRACE_PEEKTEXT, pid, esp+4*arg_num);
}

int is_there_a_breakpoint(int pid, unsigned long eip)
{
	int a;
	a = ptrace(PTRACE_PEEKTEXT, pid, eip, 0);
	if ((a & 0xFF) == 0xCC) {
		return 1;
	} else {
		return 0;
	}
}

void continue_after_breakpoint(int pid, unsigned long eip, unsigned char * value, int delete_it)
{
	delete_breakpoint(pid, eip, value);
	ptrace(PTRACE_POKEUSER, pid, 4*EIP, eip);
	if (delete_it) {
		continue_process(pid, 0);
	} else {
		int status;

		ptrace(PTRACE_SINGLESTEP, pid, 0, 0);

		pid = wait4(-1, &status, 0, NULL);
		if (pid==-1) {
			if (errno == ECHILD) {
				fprintf(output, "No more children\n");
				exit(0);
			}
			perror("wait4");
			exit(1);
		}
		insert_breakpoint(pid, eip, value);
		continue_process(pid, 0);
	}
}
