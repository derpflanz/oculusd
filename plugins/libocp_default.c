/*
 *	Default oculusd plugin
 */

/* See README for info on creating plugins.
 */

/*
 * Changes
 * 0.4
 *  o Code cleanups, mainly in buffers
 *  o Implemented plugin_initialise function
 * 0.2-0.3
 *  o No changes kept
 * 0.1
 *  o Initial release
 */
#include "../src/network.h"
#include <stdio.h>
#include "libocp_default.h"
#include "../src/plugindata.h"
#include <sys/types.h>
#include <unistd.h>
#include "../src/command.h"
#include <stdlib.h>
#include "../src/liboculus.h"
#include <string.h>
#include <time.h>
#include "../src/config.h"

char plugin_name[] = "default";
char plugin_description[] = "Handles generic server monitoring facilities.";
char plugin_version[] = "1.0";

/* commands supported by this plugin */
struct cmd_table plugin_commands[] =  {
/*	{"DATE", "handle_date", "Show time and date of server."}, */
	{"UNAM", "handle_unam", "Shows the name and build of the server."},
	{"UPTI", "handle_upti", "Shows the current uptime of the server.\n"
				"Use UPTI HUMAN for human readable uptime"},
	{"CONN", "handle_conn", "Shows the current connected clients."},
	{NULL, NULL, NULL}									/* needed for end-of-array checking */
};

/* plugin initialising */
int plugin_initialise(void) {	
	return 0;
}

/* Creates a string with uptime info from /proc:
 * utime1 utime2 loadavg1 loadavg2 loadavg3
 */
char * get_proc_info() {
	FILE *f;
	char *proc_info, buffer[1024];
	
	proc_info = buffer_setup();
	f = fopen("/proc/uptime", "r");
	fgets(buffer, 1024, f);
	proc_info = buffer_addf(proc_info, "%s ", buffer);	
	proc_info[strlen(proc_info)-2] = '\0';
	fclose(f);
	
	f = fopen("/proc/loadavg", "r");
	fgets(buffer, 1024, f);
	proc_info = buffer_addf(proc_info, " %s", buffer);
	fclose(f);
	
	return proc_info;
}

/* Make the string from get_proc_info human-readable
 */
char * humanise_proc_info(char *proc_info) {
	int utime1, utime2, days, hours, minutes, seconds, t1, t2;
	float loadavg1, loadavg5, loadavg15;
	char * timestring, * human_proc_info;	
	time_t tp;
	struct tm *tm;

	tp = time(NULL);
	tm = localtime(&tp);

	timestring = malloc (9);
	if (!strftime (timestring, 16, "%H:%M:%S", tm) ) {
		oc_writelog("libocp_default: Couldn't make timestring\n");
	}
		
	sscanf(proc_info, "%d.%d %d.%d %f %f %f", &utime1, &t1, &utime2, &t2, &loadavg1, &loadavg5, &loadavg15);
	days = utime1 / 86400;						/* seconds per day */
	hours = (utime1 % 86400) / 3600;
	minutes = ((utime1 % 86400) % 3600) / 60;
	seconds = ((utime1 % 86400) % 3600) % 60;
	
	human_proc_info = buffer_setup();
	human_proc_info = buffer_addf(human_proc_info, "%s, up ", timestring);
	if (days > 0) human_proc_info = buffer_addf(human_proc_info, "%d days, ", days);
	human_proc_info = buffer_addf(human_proc_info, "%02d:%02d:%02d, load average: %.2f, %.2f, %.2f\n", 
		hours, minutes, seconds, loadavg1, loadavg5, loadavg15);

	
	free(timestring);	
	return human_proc_info;
}

/* Connection handler
 */
struct reply *handle_conn(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc (sizeof(struct reply));
	rpl->code = OK;

	rpl->result = buffer_setup();
	rpl->result = buffer_add(rpl->result, "Currently connected clients:\n");

	struct connection *ptr = connection_list;
	char buffer[32];
	struct tm *tmp;
	while (ptr) {
		tmp = localtime(&(ptr->logon));
		strftime(buffer, 32,"%Y-%m-%d %H:%M:%S %z", tmp);

		rpl->result = buffer_addf(rpl->result, "Connection id: %d, from: %s, logged on since: %s\n", 
			ptr->socket, inet_ntoa(ptr->peer_address.sin_addr), buffer);

		ptr = ptr->next;
	}

	return rpl;
}

/* Uptime handler
 * Show the contents of /proc/uptime
 */
struct reply *handle_upti(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	char * proc_info;
	proc_info = get_proc_info();
	
	rpl = malloc (sizeof(struct reply));	
	rpl->code = OK;
	
	if (cmdline[1] && strncmp(cmdline[1], "HUMAN", 5) == 0) {
		rpl->result = humanise_proc_info(proc_info);	
		free(proc_info);
	} else {
		rpl->result = proc_info;
	}
	
	return rpl; 
}

struct reply *handle_date(char **cmdline, const struct connection *conn) {
	
	return NULL; 
}

struct reply *handle_unam(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	char buffer[1024];
	FILE *uname;
	
	uname = fopen("/proc/version", "r");
	fgets (buffer, 1024, uname);
	rpl = (struct reply *)malloc (sizeof(struct reply));
	
	rpl->code = OK;
	rpl->result = (char *)malloc(strlen(buffer)+1);
	strncpy(rpl->result, buffer, strlen(buffer));
	rpl->result[strlen(buffer)] = '\0';						/* needed, because strncpy doesnt NULL terminate */

	return rpl; 	
}
