#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>

#include "elf.h"
#include "trace.h"
#include "symbols.h"

extern void print_function(const char *, int);
extern void read_config_file(const char *);

int pid;

int debug = 0;
FILE * output = stderr;

unsigned long return_addr;
unsigned char return_value;
struct library_symbol * current_symbol;

static void usage(void)
{
	fprintf(stderr," Usage: ltrace [-d][-o output] <program> [<arguments>...]\n");
}

int main(int argc, char **argv)
{
	int status;
	struct library_symbol * tmp = NULL;

	while ((argc>2) && (argv[1][0] == '-') && (argv[1][2] == '\0')) {
		switch(argv[1][1]) {
			case 'd':	debug++;
					break;
			case 'o':	output = fopen(argv[2], "w");
					if (!output) {
						fprintf(stderr, "Can't open %s for output: %s\n", argv[2], sys_errlist[errno]);
						exit(1);
					}
					argc--; argv++;
					break;
			default:	fprintf(stderr, "Unknown option '%c'\n", argv[1][1]);
					usage();
					exit(1);
		}
		argc--; argv++;
	}

	if (argc<2) {
		usage();
		exit(1);
	}
	if (!read_elf(argv[1])) {
		fprintf(stderr, "%s: Not dynamically linked\n", argv[1]);
		exit(1);
	}
	pid = attach_process(argv[1], argv+1);
	fprintf(output, "pid %u attached\n", pid);

#if 1
	/* Enable breakpoints: */
	fprintf(output, "Enabling breakpoints...\n");
	enable_all_breakpoints(pid);
#endif
	fprintf(output, "Reading config file(s)...\n");
	read_config_file("/etc/ltrace.cfg");
	read_config_file(".ltracerc");
	continue_process(pid, 0);

	while(1) {
		unsigned long eip;
		int esp;
		int function_seen;

		pid = wait4(-1, &status, 0, NULL);
		if (pid==-1) {
			if (errno == ECHILD) {
				fprintf(output, "No more children\n");
				exit(0);
			}
			perror("wait4");
			exit(1);
		}
		if (WIFEXITED(status)) {
			fprintf(output, "pid %u exited\n", pid);
			continue;
		}
		if (WIFSIGNALED(status)) {
			fprintf(output, "pid %u exited on signal %u\n", pid, WTERMSIG(status));
			continue;
		}
		if (!WIFSTOPPED(status)) {
			fprintf(output, "pid %u ???\n", pid);
			continue;
		}
		if (WSTOPSIG(status) != SIGTRAP) {
			fprintf(output, "[0x%08lx] Signal: %u\n", get_eip(pid), WSTOPSIG(status));
			continue_process(pid, WSTOPSIG(status));
			continue;
		}
		/* pid is stopped... */
		eip = get_eip(pid);
		esp = ptrace(PTRACE_PEEKUSER, pid, 4*UESP, 0);
#if 0
		fprintf(output,"EIP = 0x%08x\n", eip);
		fprintf(output,"ESP = 0x%08x\n", esp);
#endif
		fprintf(output,"[0x%08x] ", ptrace(PTRACE_PEEKTEXT, pid, esp, 0));
		tmp = library_symbols;
		function_seen = 0;
		while(tmp) {
			if (eip == tmp->addr) {
				function_seen = 1;
				print_function(tmp->name, esp);
				continue_after_breakpoint(pid, eip, tmp->old_value, 0);
				break;
			}
			tmp = tmp->next;
		}
		if (!function_seen) {
			fprintf(output, "pid %u stopped; continuing it...\n", pid);
			continue_process(pid, 0);
		}
	}
	exit(0);
}
