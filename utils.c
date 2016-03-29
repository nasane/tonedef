/*
 *  utils.c
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

/*
 * Static wrapper function for memory allocation functions.
 *
 * This is a convienience function for checking if we've run out of memory.
 * If the pointer argument is NULL, then we immediately exit the program with a
 * failure code.
 */
void *detect_oom(void *ptr)
{
	if (NULL == ptr) {
		fprintf(stderr, "out of memory\n");

#ifndef TESTING
		exit(EXIT_FAILURE_CODE);
#endif
	}

	return ptr;
}
