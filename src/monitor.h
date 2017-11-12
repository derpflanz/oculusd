/*
 * monitor.h
 *
 * Monitor thread that plugins can register to
 */

#include "plugindata.h"

#define MSTAT_RUNNING	1
#define MSTAT_STOPPED	2

/* starts the monitor_loop thread */
int monitor_initialise();

/* the thread; loops over the plugins, looking for monitors, 
 * timing is done here */
void *monitor_loop(void *p);

/* fire a monitor; includes checking status and timer */
void monitor_fire(void *handle, struct mon_table  *monitor);
