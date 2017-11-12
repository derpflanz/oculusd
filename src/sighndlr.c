/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.1
 *
 * oculusd is the server part of oculus
 * signal.c - signal handling code
 */
 
#include <signal.h>
#include <sys/wait.h>
#include "sighndlr.h"
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include "options.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "liboculus.h"
#include "command.h"

struct sigaction sigchild, sigterm, sighup;
	
int initialise_signals() {
	extern char verbose;
	if (verbose) {
		printf("\nRegistering signal handlers.\n");
	}
	
	signal(SIGHUP, sighup_handler);
	signal(SIGTERM, sigterm_handler);
	signal(SIGCHLD, sigchild_handler);
	
	return 0;
}

void sigchild_handler(int s) {
	while (wait(NULL) > 0);
}

void sigterm_handler(int s) {
	oc_writelog("Caught SIGTERM, exiting nicely.\n");
	oc_cleanup();
	oc_exit(0);
}

void sighup_handler(int s) {
	extern char caughthup;
	extern int sockfd;
	caughthup = 1;
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
}
