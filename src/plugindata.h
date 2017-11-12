/*
 * plugindata.h ..
 * data structures for plugins
 */

#ifndef __PLUGINDATA_H
#define __PLUGINDATA_H

/* return types for monitors */
#define MONITOR_OK		1		/* return this to keep running */
#define MONITOR_STOP	2		/* return this to stop monitor */


struct reply {
	int code;
	char *result;
};

struct cmd_table {
	char *cmd;
	char *handler;
	char *helptext;
};

struct mon_table {
	char *handler;
	unsigned int interval;		/* in seconds, define the RESET of the timer */
	unsigned int timer;			/* in seconds, timer used to control firing of monitor, initialise on >0 to wait n seconds after startup */
	int status;			/* can be any of the MSTAT_* statuses */
};

#endif
