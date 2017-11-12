/*
 * Implements the disk monitoring functions
 */

#include "libocp_disk.h"
#include <stdlib.h>
#include "../src/liboculus.h"


char plugin_name[] = "disk";
char plugin_description[] = "Handles disk usage statistics and monitoring thread.";
char plugin_version[] = "1.0";

/* commands supported by this plugin */
struct cmd_table plugin_commands[] =  {
	{"DISK", "handle_disk", "Shows configured disks usage."},
	{NULL, NULL, NULL}									/* needed for end-of-array checking */
};

struct mon_table plugin_monitors[] = {
	{ "monitor_disk", 10, 0, MSTAT_RUNNING },
	{ "monitor_free", 5, 0, MSTAT_RUNNING },
	{ NULL, 0, 0, 0 }
};

/* plugin initialising */
int plugin_initialise(void) {
	return 0;
}

struct reply *handle_disk(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	
	rpl = malloc (sizeof(struct reply));	
	rpl->code = OK;
	
	rpl->result = buffer_setup();
	rpl->result = buffer_addf(rpl->result, "%s", "(not implemented yet)\n");

	return rpl;
}

int monitor_disk() {
/*	oc_writelog("(disk monitor)\n");*/
	return MONITOR_OK;
}

int monitor_free() {
/*	oc_writelog("(freespace monitor)\n");*/
	return MONITOR_OK;
}
