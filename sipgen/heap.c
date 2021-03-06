/*
 * Wrappers around standard functions that use the heap.
 *
 * Copyright (c) 2009 Riverbank Computing Limited <info@riverbankcomputing.com>
 * 
 * This file is part of SIP.
 * 
 * This copy of SIP is licensed for use under the terms of the SIP License
 * Agreement.  See the file LICENSE for more details.
 * 
 * SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include "sip.h"


static void nomem(void);


/*
 * Wrap malloc() and handle any errors.
 */
void *sipMalloc(size_t n)
{
	void *h;

	if ((h = malloc(n)) == NULL)
		nomem();

    memset(h, 0, n);

	return h;
}


/*
 * Wrap calloc() and handle any errors.
 */
void *sipCalloc(size_t nr, size_t n)
{
	void *h;

	if ((h = calloc(nr, n)) == NULL)
		nomem();

    return h;
}


/*
 * Wrap strdup() and handle any errors.
 */
char *sipStrdup(const char *s)
{
	char *h;

	if ((h = strdup(s)) == NULL)
		nomem();

	return h;
}


/*
 * Return a string on the heap which is the concatonation of all the arguments.
 */
char *concat(const char *s, ...)
{
	const char *sp;
	char *new;
	size_t len;
	va_list ap;

	/* Find the length of the final string. */

	len = 1;
	va_start(ap,s);

	for (sp = s; sp != NULL; sp = va_arg(ap, const char *))
		len += strlen(sp);

	va_end(ap);

	/* Create the new string. */

	new = sipMalloc(len);
	*new = '\0';

	va_start(ap,s);

	for (sp = s; sp != NULL; sp = va_arg(ap, const char *))
		strcat(new,sp);

	va_end(ap);

	return new;
}


/*
 * Append a string to another that is on the heap.
 */

void append(char **s, const char *new)
{
	if ((*s = realloc(*s,strlen(*s) + strlen(new) + 1)) == NULL)
		nomem();

	strcat(*s,new);
}


/*
 * Display a standard error message when the heap is exhausted.
 */

static void nomem(void)
{
	fatal("Unable to allocate memory on the heap\n");
}
