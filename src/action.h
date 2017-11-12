/* 
 * File:   action.h
 * Author: bart
 *
 * Created on July 26, 2010, 10:14 PM
 */

/*
 * 'actions' are functions that can be connected to an event. the only
 * rule is that the calls have to same prototype:
 * int action_name(void *args);
 */

#ifndef _ACTION_H
#define	_ACTION_H

#include "liboculus.h"

struct action {
	const char *name;
	const char **args;
	int (*action)(const char **args, void *extra);
};

int action_alarm_master(const char **args, void *extra);
int action_send_mail(const char **args, void *extra);
int action_set_alarm(const char **args, void *extra);

int (*action_fetch_handler(const char *action_name)) (const char **args, void *extra);

struct action *action_lookup(const char *action_name);

extern struct action action_map[];

#endif	/* _ACTION_H */

