#include <config.h>
#include "error.h"

void
error(int e, char *string)
	{
	fprintf(stderr, PACKAGE_NAME": error: %s \n", string);
	exit(e);
	}

