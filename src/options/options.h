#ifndef OPTIONS_H
#define OPTIONS_H

#include <string.h>
#include <stdlib.h>

struct option {
	const char *name;
	int letter;
	int has_arg;
	char *arg;
	int flag;
};

void set_option(struct option *option, const char *name,
		int letter, int has_arg, char *arg,
		int flag);
int get_options(int argc, char *argv[], struct option *options);

#endif
