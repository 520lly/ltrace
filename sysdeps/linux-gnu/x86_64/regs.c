#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void *
get_instruction_pointer(pid_t pid) {
	return (void *)ptrace(PTRACE_PEEKUSER, pid, 8*RIP, 0);
}

void
set_instruction_pointer(pid_t pid, long addr) {
	ptrace(PTRACE_POKEUSER, pid, 8*RIP, addr);
}

void *
get_stack_pointer(pid_t pid) {
	return (void *)ptrace(PTRACE_PEEKUSER, pid, 8*RSP, 0);
}

void *
get_return_addr(pid_t pid, void * stack_pointer) {
	return (void *)ptrace(PTRACE_PEEKTEXT, pid, stack_pointer, 0);
}
