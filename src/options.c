/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.1
 *
 * oculusd is the server part of oculus
 * oc_options.c
 *
 * Oculusd commandline options
 * This file registers global variables as oculus options:
 * -h		    print a help screen
 * -v		    print verbose messages
 * -q		    don't print anything, only to logs
 * -p port  listen on port
 * -c file  configfile to use
 */

#include "options.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liboculus.h"

void parse_cmdline_options (int argc, char *argv[]) {	
	/* default configfile */
	int length = strlen(DEFAULT_CONFIGFILE);
	config_file = malloc (length+1);
	strncpy(config_file, DEFAULT_CONFIGFILE, length);
	
	while (argc-1) {
		argc--;
		if (!argc) return;
		if (!strncmp(argv[argc], "-h", 2)) help = 1;
		if (!strncmp(argv[argc], "-q", 2)) quiet = 1;
		if (!strncmp(argv[argc], "-v", 2)) verbose = 1;
		if (!strncmp(argv[argc], "-d", 2)) runasdaemon = 1;
		if (!strncmp(argv[argc], "-p", 2)) {
			if (argv[argc+1]) 
				cmdline_port = atoi(argv[argc+1]);
			else
				if (!quiet) fprintf(stderr, "No port given after '-p' option.\n");
		}
		if (!strncmp(argv[argc], "-c", 2)) {
			if (argv[argc+1]) {
				int len = strlen(argv[argc+1]);
				if (!quiet) fprintf (stderr, "Using %s as config file.\n", argv[argc+1]);
				free (config_file);
				config_file = malloc(len+1);
				strncpy(config_file, argv[argc+1], len+1);
			} else {
				if (!quiet) fprintf(stderr, "No configfile given with '-c' option\n");
			}
		}
	}
}
