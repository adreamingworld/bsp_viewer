#include "options.h"

#include <stdio.h>

void 
set_option(struct option *option, const char *name,
		int letter, int has_arg, char *arg,
		int flag)
	{
		option->name = name;
		option->letter = letter;
		option->has_arg = has_arg;
		option->arg = arg;
		option->flag = flag;
	}

int
get_option_index_by_short(struct option *opts, int letter)
	{
	int index = 0;

	while (opts[index].name)
		{
		if (opts[index].letter == letter) return index;
		index++;
		}

	return -1;

	}

/***
Seperating short and long is maybe redundant.
***/
int
get_option_index_by_long(struct option *opts, char *long_name)
	{
	int index = 0;

	while (opts[index].name)
		{
		if (!strcmp(opts[index].name, long_name)) return index;
		index++;
		}

	return -1;

	}
int
get_long_arg(struct option *options, int argc, char *argv[], int i)
	{
	int index = -1;

	index = get_option_index_by_long(options, argv[i]+2);

	if (index == -1)
		{
		printf("Invalid argument %s\n", argv[i]+2);
		return -1;
		}

	options[index].flag = 1;

	if (options[index].has_arg)
	{
		options[index].arg = argv[i+1];
	}

	return 0;
	}

int
get_short_arg(struct option *options, int argc, char *argv[], int i)
	{
	int index = -1;

	index = get_option_index_by_short(options, *(argv[i]+1));

	if (index == -1)
		{
		printf("Invalid argument %s\n", argv[i]+1);
		return -1;
		}

	options[index].flag = 1;

	if (options[index].has_arg)
	{
		options[index].arg = argv[i+1];
	}

	return 0;
	}

int
get_options(int argc, char *argv[], struct option *options)
	{
	int i = 0;
	int option_index = -1;

	for (i = 0; i<argc; i++)
		{
		if (argv[i][0] == '-')
			{
			if (argv[i][1] == '-')
				{
				if (get_long_arg(options, argc, argv, i) == -1) exit(-1);
				}

			else
				{
				if (get_short_arg(options, argc, argv, i) == -1) exit(-1);
				}
			}
		}

	return 0;
	}
