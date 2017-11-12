/* 
 * File:   event.h
 * Author: bart
 *
 * Created on July 26, 2010, 10:23 PM
 */

#ifndef _EVENT_H
#define	_EVENT_H

#include <stdlib.h>
#include "liboculus.h"

struct event {
	int (*action)(const char **args, void *extra);
	char *event_name;
	struct event *next;
	const char **args;
};

struct evt_table {
	const char *event_name;
};

extern struct event *event_list;

int event_register(const char *name, int (*action) (const char **args, void *extra), char **args);
struct event *event_create(const char *name, int (*action) (const char **args, void *extra), char **args);
int event_fire(const char *name, void *extra);
int event_cleanup();

#endif	/* _EVENT_H */

