#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "ltrace.h"
#include "options.h"
#include "elf.h"

struct process * open_program(char * filename)
{
	struct process * proc;
	proc = malloc(sizeof(struct process));
	if (!proc) {
		perror("malloc");
		exit(1);
	}
	proc->filename = filename;
	proc->pid = 0;
	proc->breakpoints_enabled = -1;
	proc->current_syscall = -1;
	proc->current_symbol = NULL;
	proc->breakpoint_being_enabled = NULL;
	proc->next = NULL;
	if (opt_L) {
		proc->list_of_symbols = read_elf(filename);
	} else {
		proc->list_of_symbols = NULL;
	}

	proc->next = list_of_processes;
	list_of_processes = proc;
	return proc;
}

void open_pid(pid_t pid, int verbose)
{
	struct process * proc;
	char * filename;

	filename = pid2name(pid);

	if (!filename) {
		if (verbose) {
			fprintf(stderr, "Cannot trace pid %u: %s\n", pid, strerror(errno));
		}
		return;
	}

	if (trace_pid(pid)<0) {
		if (verbose) {
			fprintf(stderr, "Cannot attach to pid %u: %s\n", pid, strerror(errno));
		}
		return;
	}

	proc = open_program(filename);
	proc->pid = pid;
}
