static u_long * function_args;

struct debug_functions {
	const char * function_name;
	int return_type;
	int no_params;
	int params_type[10];
};

/*
 *	Lista de types:
 */

#define _TYPE_UNKNOWN	-1
#define _TYPE_VOID	0
#define _TYPE_INT	1
#define _TYPE_UINT	2
#define _TYPE_OCTAL	3
#define _TYPE_CHAR	4
#define _TYPE_STRING	5
#define _TYPE_ADDR	6
#define _TYPE_FILE	7
#define _TYPE_HEX	8

#define MAX_STRING 30

static struct debug_functions * function_actual;

static struct debug_functions functions_info[] = {
	{"__libc_init", _TYPE_VOID,  0, {0}},
	{"__setjmp",    _TYPE_INT,   1, {_TYPE_ADDR}},
	{"__overflow",  _TYPE_CHAR,  2, {_TYPE_FILE, _TYPE_CHAR}},
	{"__random",    _TYPE_INT,   0, {0}},
	{"__setfpucw",  _TYPE_VOID,  1, {_TYPE_UINT}},
	{"__srandom",   _TYPE_VOID,  1, {_TYPE_UINT}},
	{"__uflow",     _TYPE_INT,   1, {_TYPE_FILE}},
	{"_fxstat",     _TYPE_INT,   3, {_TYPE_INT, _TYPE_INT, _TYPE_ADDR}},
	{"_lxstat",     _TYPE_INT,   3, {_TYPE_INT, _TYPE_STRING, _TYPE_ADDR}},
	{"_xmknod",     _TYPE_INT,   4, {_TYPE_INT, _TYPE_STRING, _TYPE_OCTAL, _TYPE_ADDR}},
	{"_xstat",      _TYPE_INT,   3, {_TYPE_INT, _TYPE_STRING, _TYPE_ADDR}},
	{"access",      _TYPE_INT,   2, {_TYPE_STRING, _TYPE_INT}},
	{"alarm",       _TYPE_INT,   1, {_TYPE_INT}},
	{"atexit",      _TYPE_INT,   1, {_TYPE_ADDR}},
	{"atoi",        _TYPE_INT,   1, {_TYPE_STRING}},
	{"basename",    _TYPE_STRING,1, {_TYPE_STRING}},
	{"bcmp",        _TYPE_INT,   3, {_TYPE_ADDR, _TYPE_ADDR, _TYPE_UINT}},
	{"bind",        _TYPE_INT,   3, {_TYPE_INT, _TYPE_ADDR, _TYPE_INT}},
	{"bindresvport",_TYPE_INT,   2, {_TYPE_INT, _TYPE_ADDR}},
	{"brk",         _TYPE_INT,   1, {_TYPE_ADDR}},
	{"bzero",       _TYPE_VOID,  2, {_TYPE_ADDR, _TYPE_UINT}},
	{"calloc",      _TYPE_ADDR,  2, {_TYPE_UINT, _TYPE_UINT}},
	{"cfgetospeed", _TYPE_UINT,  1, {_TYPE_ADDR}},
	{"dup",         _TYPE_INT,   1, {_TYPE_UINT}},
	{"dup2",        _TYPE_INT,   2, {_TYPE_UINT, _TYPE_UINT}},
	{"chdir",       _TYPE_INT,   1, {_TYPE_STRING}},
	{"chmod",       _TYPE_INT,   2, {_TYPE_STRING, _TYPE_OCTAL}},
	{"chown",       _TYPE_INT,   3, {_TYPE_STRING, _TYPE_UINT, _TYPE_UINT}},
	{"close",       _TYPE_INT,   1, {_TYPE_INT}},
	{"closedir",    _TYPE_INT,   1, {_TYPE_ADDR}},
	{"ctime",       _TYPE_STRING,1, {_TYPE_ADDR}},
	{"endmntent",   _TYPE_INT,   1, {_TYPE_ADDR}},
	{"fchmod",      _TYPE_INT,   2, {_TYPE_INT, _TYPE_OCTAL}},
	{"fchown",      _TYPE_INT,   3, {_TYPE_INT, _TYPE_UINT, _TYPE_UINT}},
	{"fclose",      _TYPE_INT,   1, {_TYPE_FILE}},
	{"fdopen",      _TYPE_FILE,  2, {_TYPE_INT, _TYPE_STRING}},
	{"feof",        _TYPE_INT,   1, {_TYPE_FILE}},
	{"ferror",      _TYPE_INT,   1, {_TYPE_FILE}},
	{"fflush",      _TYPE_INT,   1, {_TYPE_FILE}},
	{"fgetc",       _TYPE_CHAR,  1, {_TYPE_FILE}},
	{"fgets",       _TYPE_ADDR,  3, {_TYPE_ADDR, _TYPE_UINT, _TYPE_ADDR}},
	{"fileno",      _TYPE_INT,   1, {_TYPE_ADDR}},
	{"fopen",       _TYPE_ADDR,  2, {_TYPE_STRING, _TYPE_STRING}},
	{"fork",        _TYPE_INT,   0, {0}},
	{"fprintf",     _TYPE_INT,   2, {_TYPE_FILE, _TYPE_STRING}},
	{"fputc",       _TYPE_INT,   2, {_TYPE_CHAR, _TYPE_FILE}},
	{"fputs",       _TYPE_INT,   2, {_TYPE_STRING, _TYPE_FILE}},
	{"fread",       _TYPE_UINT,  4, {_TYPE_FILE, _TYPE_UINT, _TYPE_UINT, _TYPE_ADDR}},
	{"free",        _TYPE_VOID,  1, {_TYPE_ADDR}},
	{"freopen",     _TYPE_ADDR,  3, {_TYPE_STRING, _TYPE_STRING, _TYPE_ADDR}},
	{"fseek",       _TYPE_INT,   3, {_TYPE_FILE, _TYPE_UINT, _TYPE_INT}},
	{"ftell",       _TYPE_UINT,  1, {_TYPE_FILE}},
	{"fwrite",      _TYPE_UINT,  4, {_TYPE_FILE, _TYPE_UINT, _TYPE_UINT, _TYPE_ADDR}},
	{"getdtablesize",_TYPE_INT,  0, {0}},
	{"getenv",      _TYPE_STRING,1, {_TYPE_STRING}},
	{"getgid",      _TYPE_INT,   0, {0}},
	{"getgrgid",    _TYPE_ADDR,  1, {_TYPE_UINT}},
	{"getgrnam",    _TYPE_ADDR,  1, {_TYPE_STRING}},
	{"gethostbyname",_TYPE_ADDR, 1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"gethostname", _TYPE_INT,   2, {_TYPE_STRING, _TYPE_UINT, 0, 0, 0}},
	{"getmntent",   _TYPE_ADDR,  1, {_TYPE_ADDR}},
	{"getopt",      _TYPE_CHAR,  3, {_TYPE_INT, _TYPE_ADDR, _TYPE_STRING}},
	{"getopt_long", _TYPE_CHAR,  5, {_TYPE_INT, _TYPE_ADDR,_TYPE_STRING,_TYPE_ADDR,_TYPE_ADDR}},
	{"getpagesize", _TYPE_UINT,  0, {0}},
	{"getpid",      _TYPE_UINT,  0, {0}},
	{"getpwnam",    _TYPE_ADDR,  1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"getpwuid",    _TYPE_ADDR,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"getservbyname",_TYPE_ADDR, 2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"gettimeofday",_TYPE_INT,   2, {_TYPE_ADDR, _TYPE_ADDR, 0, 0, 0}},
	{"htonl",       _TYPE_UINT,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"inet_addr",   _TYPE_ADDR,  1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"inet_ntoa",   _TYPE_STRING,1, {_TYPE_ADDR, 0, 0, 0, 0}},
	{"ioctl",       _TYPE_INT,   2, {_TYPE_INT, _TYPE_HEX}},
	{"isalnum",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isalpha",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isascii",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isatty",      _TYPE_INT,   1, {_TYPE_INT, 0, 0, 0, 0}},
	{"iscntrl",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isdigit",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isgraph",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"islower",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isprint",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"ispunct",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isspace",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isupper",     _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"isxdigit",    _TYPE_INT,   1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"malloc",      _TYPE_ADDR,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"memmove",     _TYPE_ADDR,  3, {_TYPE_ADDR, _TYPE_ADDR, _TYPE_INT, 0, 0}},
	{"memset",      _TYPE_ADDR,  3, {_TYPE_ADDR, _TYPE_HEX, _TYPE_UINT}},
	{"mmap",        _TYPE_ADDR,  5, {_TYPE_UINT, _TYPE_UINT, _TYPE_UINT, _TYPE_INT,_TYPE_UINT}},
	{"ntohl",       _TYPE_UINT,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"ntohs",       _TYPE_UINT,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"open",        _TYPE_INT,   3, {_TYPE_STRING, _TYPE_INT, _TYPE_OCTAL, 0, 0}},
	{"opendir",     _TYPE_ADDR,  1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"perror",      _TYPE_VOID,  1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"pipe",        _TYPE_INT,   1, {_TYPE_ADDR}},
	{"printf",      _TYPE_INT,   1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"putenv",      _TYPE_INT,   1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"puts",        _TYPE_INT,   1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"qsort",       _TYPE_VOID,  4, {_TYPE_ADDR, _TYPE_UINT, _TYPE_UINT, _TYPE_ADDR, 0}},
	{"read",        _TYPE_INT,   3, {_TYPE_INT, _TYPE_ADDR, _TYPE_UINT, 0, 0}},
	{"readdir",     _TYPE_ADDR,  1, {_TYPE_ADDR, 0, 0, 0, 0}},
	{"readline",    _TYPE_STRING,0, {0, 0, 0, 0, 0}},
	{"readlink",    _TYPE_INT,   3, {_TYPE_STRING, _TYPE_ADDR, _TYPE_UINT, 0, 0}},
	{"realloc",     _TYPE_ADDR,  2, {_TYPE_ADDR, _TYPE_UINT, 0, 0, 0}},
	{"rewind",      _TYPE_VOID,  1, {_TYPE_FILE, 0, 0, 0, 0}},
	{"rewinddir",   _TYPE_VOID,  1, {_TYPE_FILE, 0, 0, 0, 0}},
	{"rindex",      _TYPE_STRING,2, {_TYPE_STRING, _TYPE_INT, 0, 0, 0}},
	{"sbrk",        _TYPE_ADDR,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"select",      _TYPE_INT,   5, {_TYPE_INT, _TYPE_ADDR, _TYPE_ADDR, _TYPE_ADDR,_TYPE_ADDR}},
	{"setbuf",      _TYPE_VOID,  2, {_TYPE_ADDR, _TYPE_ADDR, 0, 0, 0}},
	{"setgid",      _TYPE_INT,   1, {_TYPE_UINT}},
	{"setlinebuf",  _TYPE_VOID,  1, {_TYPE_FILE, 0, 0, 0, 0}},
	{"setmntent",   _TYPE_ADDR,  2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"setuid",      _TYPE_INT,   1, {_TYPE_INT, 0, 0, 0, 0}},
	{"setvbuf",     _TYPE_INT,   4, {_TYPE_ADDR, _TYPE_ADDR, _TYPE_INT, _TYPE_UINT, 0}},
	{"sigaction",   _TYPE_INT,   3, {_TYPE_INT, _TYPE_ADDR, _TYPE_ADDR, 0, 0}},
	{"sleep",       _TYPE_UINT,  1, {_TYPE_UINT, 0, 0, 0, 0}},
	{"sprintf",     _TYPE_INT,   2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"sscanf",      _TYPE_INT,   2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strcasecmp",  _TYPE_INT,   2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"stpcpy",      _TYPE_STRING,2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strcmp",      _TYPE_INT,   2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strcspn",     _TYPE_INT,   2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strdup",      _TYPE_STRING,1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"strerror",    _TYPE_STRING,1, {_TYPE_INT, 0, 0, 0, 0}},
	{"strpbrk",     _TYPE_STRING,2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strrchr",     _TYPE_STRING,2, {_TYPE_STRING, _TYPE_CHAR, 0, 0, 0}},
	{"strspn",      _TYPE_UINT,  2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strtok",      _TYPE_STRING,2, {_TYPE_STRING, _TYPE_STRING, 0, 0, 0}},
	{"strtol",      _TYPE_INT,   3, {_TYPE_STRING, _TYPE_ADDR, _TYPE_UINT, 0, 0}},
	{"strtoul",     _TYPE_INT,   3, {_TYPE_STRING, _TYPE_ADDR, _TYPE_UINT, 0, 0}},
	{"tcgetattr",   _TYPE_INT,   2, {_TYPE_INT, _TYPE_ADDR, 0, 0, 0}},
	{"tcsetattr",   _TYPE_INT,   2, {_TYPE_INT, _TYPE_ADDR, 0, 0, 0}},
	{"time",        _TYPE_UINT,  1, {_TYPE_ADDR}},
	{"toascii",     _TYPE_CHAR,  1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"tolower",     _TYPE_CHAR,  1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"toupper",     _TYPE_CHAR,  1, {_TYPE_CHAR, 0, 0, 0, 0}},
	{"unlink",      _TYPE_INT,   1, {_TYPE_STRING, 0, 0, 0, 0}},
	{"uname",       _TYPE_INT,   1, {_TYPE_ADDR, 0, 0, 0, 0}},
	{"ungetc",      _TYPE_CHAR,  2, {_TYPE_INT, _TYPE_ADDR, 0, 0, 0}},
	{"utime",       _TYPE_INT,   2, {_TYPE_STRING, _TYPE_ADDR, 0, 0, 0}},
	{"vfprintf",    _TYPE_INT,   2, {_TYPE_ADDR, _TYPE_STRING, 0, 0, 0}},
	{"wait",        _TYPE_UINT,  1, {_TYPE_ADDR, 0, 0, 0, 0}},
	{"wait3",       _TYPE_UINT,  3, {_TYPE_ADDR, _TYPE_INT, _TYPE_ADDR, 0, 0}},
	{"waitpid",     _TYPE_UINT,  3, {_TYPE_UINT, _TYPE_ADDR, _TYPE_ADDR, 0, 0}},
	{"write",       _TYPE_INT,   3, {_TYPE_INT, _TYPE_ADDR, _TYPE_UINT, 0, 0}},
	{NULL, _TYPE_UNKNOWN, 1, {_TYPE_UNKNOWN, 0, 0, 0, 0}}
};

static char * print_char(unsigned long value)
{
	static char result[5];

	result[0]='\\';
	switch(value) {
		case '\r':	result[1]='r'; result[2]='\0'; break;
		case '\n':	result[1]='n'; result[2]='\0'; break;
		case '\t':	result[1]='t'; result[2]='\0'; break;
		case '\\':	result[1]='\\'; result[2]='\0'; break;
		default:	
			if ((value<32) || ((value>126) && (value<256))) {
				sprintf(result+1, "%lo", value);
			} else {
				result[0]=value; result[1]='\0'; break;
			}
	}
	return result;
}

static char * print_param(int type, unsigned long value)
{
	static char result[1024];

	switch(type) {
		case _TYPE_INT:
			sprintf(result, "%d", (int)value);
			break;
		case _TYPE_UINT:
			sprintf(result, "%u", (unsigned int)value);
			break;
		case _TYPE_ADDR:
			if (!value) {
				sprintf(result, "NULL");
			} else {
				sprintf(result, "0x%08x", (unsigned int)value);
			}
			break;
		case _TYPE_FILE:
#if 0
			if (value==(unsigned long)stdin) {
				sprintf(result, "stdin");
			} else if (value==(unsigned long)stdout) {
				sprintf(result, "stdout");
			} else if (value==(unsigned long)stderr) {
				sprintf(result, "stderr");
			} else {
#endif
				return print_param(_TYPE_ADDR, value);
#if 0
			}
#endif
			break;
		case _TYPE_OCTAL:
			sprintf(result, "0%o", (unsigned int)value);
			break;
		case _TYPE_CHAR:
			if (value==-1) {
				sprintf(result, "EOF");
			} else {
				sprintf(result, "'%s'", print_char(value));
			}
			break;
		case _TYPE_STRING:
			if (value==0) {
				sprintf(result, "<NULL>");
			} else {
				int i;
				sprintf(result, "\"");
				for(i=0; *((char*)value+i) && i<MAX_STRING; i++) {
					strcat(result, print_char(*((char*)value+i)));
				}
				strcat(result, "\"");
				if (i==MAX_STRING) {
					strcat(result, "...");
				}
			}
			break;
		case _TYPE_HEX:
			sprintf(result, "0x%lx", value);
			break;
		case _TYPE_VOID:
			sprintf(result, "<void>");
			break;
		case _TYPE_UNKNOWN:
			sprintf(result, "???");
			break;
		default:
			sprintf(result, "???");
	}
	return result;
}

static void print_results(u_long arg)
{
	char message[1024];
	int i;

#if 0
	if (!current_pid) {
		return;
	}
#endif

	if (pointer != (void *)*(pointer->got)) {
		pointer->real_func = (void *)*(pointer->got);
		bcopy((char *)&pointer, (char *)pointer->got, 4);
	}

#if 0
_sys_write(fd, pointer->name, strlen(pointer->name));
_sys_write(fd, ":\n", 2);
#endif

	function_actual = functions_info;
	while(function_actual->function_name) {
		if (!strcmp(pointer->name, function_actual->function_name)) {
			break;
		}
		function_actual++;
	}

	function_args = &arg;

#if 0
	if (!strcmp(pointer->name, "fork")) {
		if (_sys_getpid() != current_pid) {
			_sys_close(fd);
			current_pid=0;
			return;
		}
	}
#endif

#if 0
sprintf(message,"call to = 0x%08x\n"
		"got     = 0x%08x\n"
		"return  = 0x%08x\n"
		"args[0] = 0x%08x\n"
		"args[1] = 0x%08x\n"
		"args[2] = 0x%08x\n"
		"args[3] = 0x%08x\n"
		"args[4] = 0x%08x\n"
		"args[5] = 0x%08x\n",
		pointer, pointer->got, where_to_return,
		function_args[0], function_args[1], function_args[2],
		function_args[3], function_args[4], function_args[5]);
_sys_write(fd, message, strlen(message));
#endif

	message[0] = '\0';
	sprintf(message, "%s%s(", message, pointer->name);
	if (function_actual->no_params > 0) {
		sprintf(message, "%s%s", message,
			print_param(function_actual->params_type[0], function_args[0]));
	}
	for(i=1; i<function_actual->no_params; i++) {
		sprintf(message, "%s,%s", message,
			print_param(function_actual->params_type[i], function_args[i]));
	}
	sprintf(message, "%s) = %s\n", message,
		print_param(function_actual->return_type, returned_value));
	_sys_write(fd, message, strlen(message));
}
