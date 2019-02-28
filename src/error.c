#include "error.h"

void error(int e, char *string)
{
	fprintf(stderr, "Error: %s \n", string);
	exit(e);
}

