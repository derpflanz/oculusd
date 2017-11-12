/*
 * monitor.c
 *
 * Implementation of monitor thread
 */
 
#include <pthread.h>
#include "monitor.h"
#include <stdio.h>
#include <unistd.h>
#include "globals.h"
#include "plugindata.h"
#include "plugin.h"

pthread_t monitor_thread;

/* Start the monitor thread. The thread will call everything
 * that is registered in the plugins linked list.
 */
int monitor_initialise() {
	int r = 0; 
	int p = 0;
	
	r = pthread_create(&monitor_thread, NULL, monitor_loop, NULL);
	
	return r;
}

void *monitor_loop(void *p) {
	unsigned int seconds = 0;
	
	while (1) {
		struct plugin *el = plugins;
		
		while (el->next != NULL) {
			struct mon_table *monitors = el->monitors;

			if (monitors != NULL) {
				// found an array with monitor functions
				int i = 0;
				while (monitors[i].handler != NULL) {
					monitor_fire(el->handle, &monitors[i]);
					i++;
				}
			}
			el = el->next; 
		}
		
		/* we sleep one second */
		usleep(1000000);
	}
	
	return NULL;
}

void monitor_fire(void *handle, struct mon_table  *monitor) {
	if (monitor->status == MSTAT_RUNNING) {
		if (monitor->timer == 0) {
			int (*monitor_handler)(void) = NULL;
			
			monitor_handler = (int (*)()) dlsym(handle, monitor->handler);
			
			/* ok to check for NULL, because this should never be NULL anyway */
			if (monitor_handler != NULL) {
				int result = 0;
				result = monitor_handler();
				if (result == MONITOR_STOP) {
					monitor->status = MSTAT_STOPPED;
				}
			}

			monitor->timer = monitor->interval;
		}
		
		monitor->timer--;
	}
}
