/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.1
 *
 * oculusd is the server part of oculus
 * oc_options.c
 *
 * Oculusd commandline options
 * This file registers global variables as oculus options:
 * -h		print a help screen
 * -v		print verbose messages
 * -q		don't print anything, only to logs
 * -d		run as a daemon
 */
 
void parse_cmdline_options (int argc, char *argv[]);
