#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "elf.h"
#include "i386.h"
#include "symbols.h"
#include "functions.h"
#include "process.h"

extern void read_config_file(const char *);

FILE * output = stderr;
int opt_d = 0;
int opt_i = 0;

unsigned long return_addr;
unsigned char return_value;
struct library_symbol * current_symbol;

static void usage(void)
{
	fprintf(stderr,"Usage: ltrace [-d] [-o filename] command [arg ...]\n\n");
}

int main(int argc, char **argv)
{
	int pid;

	while ((argc>2) && (argv[1][0] == '-') && (argv[1][2] == '\0')) {
		switch(argv[1][1]) {
			case 'd':	opt_d++;
					break;
			case 'o':	output = fopen(argv[2], "w");
					if (!output) {
						fprintf(stderr, "Can't open %s for output: %s\n", argv[2], sys_errlist[errno]);
						exit(1);
					}
					argc--; argv++;
					break;
			case 'i':	opt_i++;
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

	init_sighandler();

	if (opt_d>0) {
		fprintf(output, "Reading config file(s)...\n");
	}
	read_config_file("/etc/ltrace.cfg");
	read_config_file(".ltracerc");

	pid = execute_process(argv[1], argv+1);
	if (opt_d>0) {
		fprintf(output, "pid %u launched\n", pid);
	}

	while(1) {
		pause();
		if (!list_of_processes) {
			break;
		}
	}

	exit(0);
}
