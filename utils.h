/*
 *  utils.h
 *
 *  Copyright (C) 2016  Nathan Bossart
 */

#ifndef UTILS_H
#define UTILS_H

/* failure code to exit with if we hit a critical error */
#define EXIT_FAILURE_CODE	1

#define MIN(a, b)		((a < b) ? (a) : (b))
#define AUDIO_SAMPLE_BUFSIZE	512
#define MALLOC_SAFELY(a)	detect_oom(malloc(a))
#define CALLOC_SAFELY(a, b)	detect_oom(calloc(a, b))
#define FREE_SAFELY(a)		free(a); a = NULL

void *detect_oom(void *ptr);

#endif
