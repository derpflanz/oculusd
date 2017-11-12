/* Created by Anjuta version 1.1.97 */
/*	This file will not be overwritten */

/* oculusd main.c v0.7
 * 
 * Changes (not only to main.c, also to the entire project)
 * 0.7 
 *  Introduction of the plugins array, needed for the new RELD command
 * 0.4
 *  o Rebuilt the project to use handmade Makefile.am files
 *  o generic.c changed into liboculus.c, so that plugins can also use its functionality
 *  o Fixed plugin Makefile.am
 * 0.2-0.3
 *  No changes kept
 * 0.1
 *  Initial release
 */

#include "plugin.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "sighndlr.h"
#include "network.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "monitor.h"
#include "options.h"
#include "liboculus.h"
#include "plugin.h"
#include "command.h"

int main(int argc, char *argv[]) {
	extern char quiet;
	
	/* messages that go into stdout */	
	if (!quiet) printf("Oculus Network Monitor v%s\n", VERSION);
	
	/* parse the commandline options */
	parse_cmdline_options(argc, argv);
	
	if (help) {
		printf ("\nUsage:\n");
		printf ("oculusd [-h] [-q] [-v [-d] [-c configfile]  [-p port]\n");
		printf("-c configfile  Use another configfile than %s\n", DEFAULT_CONFIGFILE);
		printf("-h             Show this help screen\n");
		printf("-v             Show verbose messages (for debugging)\n");
		printf("-q             Don't show anything, only in the logs\n");
		printf("-d             Run as a daemon\n");
		printf("-p <port>      Listen on <port>\n");
		printf("\n");
		oc_exit(0);
	}

	/* start and deamonise */
	oc_start();

	/* initialise and start monitor thread, we need to do that here, 
	 * because oc_start will initialise the plugins, and they can
	 * register functions in the monitor thread
	 */
	if (monitor_initialise() != 0) {
		oc_writelog("Could not start monitor thread.");
	}


	if (runasdaemon) daemon(0, 0);
	
	/* start main listener */
	main_loop();

	return (0);
}
