#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ltrace.h"
#include "options.h"

static int display_char(int what);
static int display_string(enum tof type, struct process *proc,
			  void* addr, size_t maxlen);
static int display_value(enum tof type, struct process *proc,
			 long value, arg_type_info *info);
static int display_unknown(enum tof type, struct process *proc, long value);
static int display_format(enum tof type, struct process *proc, int arg_num);

static int string_maxlength = INT_MAX;

static long get_length(enum tof type, struct process *proc, int len_spec)
{
    if (len_spec > 0)
	return len_spec;
    return gimme_arg(type, proc, -len_spec - 1);
}

static int display_pointer(enum tof type, struct process *proc, long value,
			   arg_type_info * info)
{
    long pointed_to;
    arg_type_info *inner = info->u.ptr_info.info;

    if (value == 0)
      return fprintf(output, "NULL");
    else if (umovelong(proc, (void *) value, &pointed_to) < 0)
      return fprintf(output, "?");
    else
      return display_value(type, proc, pointed_to, inner);
}

static int display_enum(enum tof type, struct process *proc,
                        arg_type_info* info, long value)
{
    int ii;
    for (ii = 0; ii < info->u.enum_info.entries; ++ii) {
	if (info->u.enum_info.values[ii] == value)
	    return fprintf(output, "%s", info->u.enum_info.keys[ii]);
    }

    return display_unknown(type, proc, value);
}

/* Args:
   type - syscall or shared library function or memory
   proc - information about the traced process
   value - the value to display
   info - the description of the type to display
*/
int display_value(enum tof type, struct process *proc,
                  long value, arg_type_info *info)
{
	int tmp;

	switch (info->type) {
	case ARGTYPE_VOID:
		return 0;
        case ARGTYPE_IGNORE:
        	return 0; /* Empty gap between commas */
	case ARGTYPE_INT:
          return fprintf(output, "%d", (int) value);
	case ARGTYPE_UINT:
          return fprintf(output, "%u", (unsigned) value);
	case ARGTYPE_LONG:
		if (proc->mask_32bit)
			return fprintf(output, "%d", (int) value);
		else
			return fprintf(output, "%ld", value);
	case ARGTYPE_ULONG:
		if (proc->mask_32bit)
			return fprintf(output, "%u", (unsigned) value);
		else
			return fprintf(output, "%lu", (unsigned long) value);
	case ARGTYPE_OCTAL:
		return fprintf(output, "0%o", (unsigned) value);
	case ARGTYPE_CHAR:
		tmp = fprintf(output, "'");
		tmp += display_char(value == -1 ? value : (char) value);
		tmp += fprintf(output, "'");
		return tmp;
	case ARGTYPE_ADDR:
		if (!value)
			return fprintf(output, "NULL");
		else
			return fprintf(output, "0x%08lx", value);
	case ARGTYPE_FORMAT:
		fprintf(stderr, "Should never encounter a format anywhere but at the top level (for now?)\n");
		exit(1);
	case ARGTYPE_STRING:
		return display_string(type, proc, (void*) value,
				      string_maxlength);
	case ARGTYPE_STRING_N:
		return display_string(type, proc, (void*) value,
				      get_length(type, proc,
						 info->u.string_n_info.size_spec));
        case ARGTYPE_ENUM:
		return display_enum(type, proc, info, value);
	case ARGTYPE_POINTER:
		return display_pointer(type, proc, value, info);
 	case ARGTYPE_UNKNOWN:
	default:
		return display_unknown(type, proc, value);
	}
}

int display_arg(enum tof type, struct process *proc, int arg_num,
		arg_type_info * info)
{
    long arg;

    if (info->type == ARGTYPE_VOID) {
	return 0;
    } else if (info->type == ARGTYPE_FORMAT) {
	return display_format(type, proc, arg_num);
    } else {
	arg = gimme_arg(type, proc, arg_num);
	return display_value(type, proc, arg, info);
    }
}

static int display_char(int what)
{
	switch (what) {
	case -1:
		return fprintf(output, "EOF");
	case '\r':
		return fprintf(output, "\\r");
	case '\n':
		return fprintf(output, "\\n");
	case '\t':
		return fprintf(output, "\\t");
	case '\b':
		return fprintf(output, "\\b");
	case '\\':
		return fprintf(output, "\\\\");
	default:
		if (isprint(what)) {
			return fprintf(output, "%c", what);
		} else {
			return fprintf(output, "\\%03o", (unsigned char)what);
		}
	}
}

#define MIN(a,b) (((a)<(b)) ? (a) : (b))

static int display_string(enum tof type, struct process *proc, void *addr,
			  size_t maxlength)
{
	unsigned char *str1;
	int i;
	int len = 0;

	if (!addr) {
		return fprintf(output, "NULL");
	}

	str1 = malloc(MIN(opt_s, maxlength) + 3);
	if (!str1) {
		return fprintf(output, "???");
	}
	umovestr(proc, addr, MIN(opt_s, maxlength) + 1, str1);
	len = fprintf(output, "\"");
	for (i = 0; i < MIN(opt_s, maxlength); i++) {
		if (str1[i]) {
			len += display_char(str1[i]);
		} else {
			break;
		}
	}
	len += fprintf(output, "\"");
	if (str1[i] && (opt_s <= maxlength)) {
		len += fprintf(output, "...");
	}
	free(str1);
	return len;
}

static int display_unknown(enum tof type, struct process *proc, long value)
{
	if (proc->mask_32bit) {
		if ((int)value < 1000000 && (int)value > -1000000)
			return fprintf(output, "%d", (int)value);
		else
			return fprintf(output, "%p", (void *)value);
	} else if (value < 1000000 && value > -1000000) {
		return fprintf(output, "%ld", value);
	} else {
		return fprintf(output, "%p", (void *)value);
	}
}

static int display_format(enum tof type, struct process *proc, int arg_num)
{
	void *addr;
	unsigned char *str1;
	int i;
	int len = 0;

	addr = (void *)gimme_arg(type, proc, arg_num);
	if (!addr) {
		return fprintf(output, "NULL");
	}

	str1 = malloc(MIN(opt_s, string_maxlength) + 3);
	if (!str1) {
		return fprintf(output, "???");
	}
	umovestr(proc, addr, MIN(opt_s, string_maxlength) + 1, str1);
	len = fprintf(output, "\"");
	for (i = 0; len < MIN(opt_s, string_maxlength) + 1; i++) {
		if (str1[i]) {
			len += display_char(str1[i]);
		} else {
			break;
		}
	}
	len += fprintf(output, "\"");
	if (str1[i] && (opt_s <= string_maxlength)) {
		len += fprintf(output, "...");
	}
	for (i = 0; str1[i]; i++) {
		if (str1[i] == '%') {
			int is_long = 0;
			while (1) {
				unsigned char c = str1[++i];
				if (c == '%') {
					break;
				} else if (!c) {
					break;
				} else if (strchr("lzZtj", c)) {
					is_long++;
					if (c == 'j')
						is_long++;
					if (is_long > 1
					    && (sizeof(long) < sizeof(long long)
						|| proc->mask_32bit)) {
						len += fprintf(output, ", ...");
						str1[i + 1] = '\0';
						break;
					}
				} else if (c == 'd' || c == 'i') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", %d",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", %ld",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == 'u') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", %u",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", %lu",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == 'o') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", 0%o",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", 0%lo",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == 'x' || c == 'X') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", %#x",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", %#lx",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (strchr("eEfFgGaACS", c)
					   || (is_long
					       && (c == 'c' || c == 's'))) {
					len += fprintf(output, ", ...");
					str1[i + 1] = '\0';
					break;
				} else if (c == 'c') {
					len += fprintf(output, ", '");
					len +=
					    display_char((int)
							 gimme_arg(type, proc,
								   ++arg_num));
					len += fprintf(output, "'");
					break;
				} else if (c == 's') {
					len += fprintf(output, ", ");
					len +=
					    display_string(type, proc,
							   (void *)gimme_arg(type,
									     proc,
									     ++arg_num),
							   string_maxlength);
					break;
				} else if (c == 'p' || c == 'n') {
					len +=
					    fprintf(output, ", %p",
						    (void *)gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == '*') {
					len +=
					    fprintf(output, ", %d",
						    (int)gimme_arg(type, proc,
								   ++arg_num));
				}
			}
		}
	}
	free(str1);
	return len;
}
