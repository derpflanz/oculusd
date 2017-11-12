/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.8
 *
 * oculusd is the server part of oculus
 * oc_config.c
 */

/* The new and improved configfile parser. As of 0.3 we have:
 * o An oculusd DTD
 * o A validating parser
 * o XPath for selecting elements
 *
 * Some functions use the following 'exception handling' mechanism, to prevent 
 * multi-layered if structures
 *
 * void *p, *q;
 * p = some_function();
 * if (p == NULL) goto label:
 * q = some_other_function(p);
 * 
 * label:
 * cleanup_things();
 */

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include "config.h"
#include <string.h>
#include <time.h>
#include "liboculus.h"
#include <pcre.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "plugindata.h"

/* Parse the configuration file, with DTD check
 * return -1 to denote an error has occurred
 * return 0 for success 
 */
int parse_config_file(const char *filename) {
	xmlDocPtr doc = NULL;

	int ret = -1;

	/* setup defaults */
	use_defaults();
	
	doc = get_xmldoc(filename);
	
	/* TODO: check version, it must be equal to VERSION
	 */
	if (doc != NULL) ret = read_configuration(doc);
	free_xmldoc(doc);

	return ret;
}

/* Read out the configuration file, using XPath 
 * return -1 to denote errors, 0 on success
 */
int read_configuration(xmlDocPtr doc) {
	int ret = -1;
	xmlXPathContextPtr xpathCtx = NULL;
	char **temparr;

	/* setup XPath context */
	xpathCtx = xmlXPathNewContext(doc);
	
	oc_host = buffer_setup();
	
	logfile_name = buffer_setup();
	plugindir = buffer_setup();
	
	/* read config */
	if (xpathCtx != NULL) {
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/server/host/text()"))) {
			oc_host = buffer_add(oc_host, temparr[0]);
			string_array_free(temparr);
		}
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/server/port/text()"))) {
			oc_port = atoi(temparr[0]);
			string_array_free(temparr);
		}
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/master/host/text()"))) {
//			printf("-------------------- %s --------------------\n\n\n\n", temparr[0]);
			if (temparr[0] != NULL) {
				master_host = buffer_setup();
				master_host = buffer_add(master_host, temparr[0]);
				master_port = OC_PORT;
			}
			string_array_free(temparr);
		}
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/master/port/text()"))) {
			if (temparr[0] != NULL) {
				master_port = atoi(temparr[0]);
			}
			string_array_free(temparr);
		}
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/server/logfile/text()"))) {
			logfile_name = buffer_add(logfile_name, temparr[0]);
			string_array_free(temparr);
		}
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/server/plugindir/text()"))) {
			plugindir = buffer_add(plugindir, temparr[0]);
			string_array_free(temparr);
		}
		if ((temparr = xpath_execute(xpathCtx, 
			"/oculusd/onmp/allowed-commands/command"))) {
				allowed_commands = temparr;
				temparr = NULL;
		}
		if ((temparr = xpath_execute(xpathCtx, "/oculusd/server/hosts-allow/host"))) {
			int num_hosts = -1;
			
			/* set up the allowed_hosts array */
			while (temparr[++num_hosts]);
			allowed_hosts = malloc ((num_hosts+1) * sizeof(struct in_addr));

			/* fill the array, close it with a 0.0.0.0 address */
			num_hosts = -1;
			while (temparr[++num_hosts]) 
				inet_aton(temparr[num_hosts], &allowed_hosts[num_hosts]);
			
			inet_aton("0.0.0.0", &allowed_hosts[num_hosts ]);
			
			string_array_free(temparr);
		}
		ret = 0;
	}

	if (!runasdaemon) 
		logfile = stdout;
	else {
		/* try to open the logfile */
		logfile = fopen(logfile_name, "a");
		if (!logfile) {
			/* no logfile available, log to stdout, 
			 * even as a daemon 
			 */
			fprintf(stderr, "Couldn't open logfile, daemon will log to stdout.\n");
			logfile = stdout;
		} else {
			oc_writelog("Logfile re-opened\n");
			stderr = logfile;
		}
	}
	
	/* clean up the XPath context */
	xmlXPathFreeContext(xpathCtx);	
	return ret;
}

void print_config(FILE *out) {

	int i = 0;
	
	fprintf(out, "\nOculus configuration:\n");
	fprintf(out, "  hostname      : %s\n", oc_host);
	fprintf(out, "  port          : %d\n", oc_port);
	fprintf(out, "  master        : %s:%d\n", master_host, master_port);
	fprintf(out, "  logfile       : %s (daemon mode)\n", logfile_name);
	fprintf(out, "  plugindir     : %s\n", plugindir);
	fprintf(out, "  runtime files : %s\n", runtimedir);
	
/*	fprintf (out, "  supported commands: ");
	while (supported_commands[i].cmd) {
		fprintf (out, "%s ", supported_commands[i].cmd);
		i++;
	}*/

	i = 0;	
	fprintf (out, "\n  allowed hosts: ");
	if (allowed_hosts) while (allowed_hosts[i].s_addr) fprintf(out, "%s ", inet_ntoa(allowed_hosts[i++]));
	else fprintf (out, "(none)");
	
	i = 0;
	fprintf (out, "\n  allowed commands: ");
	if (allowed_commands) while (allowed_commands[i])  fprintf(out, "%s ", allowed_commands[i++]);
	else fprintf(out, "(none)");

	fprintf(out, "\n\n");
}

/* Set up default configuration, as hardcoded in config.h 
 * oc_host, oc_port, logfile_name, plugindir
 * allowed_commands, supported_commands and allowed_hosts
 *
 * Note: some of these are already set on initisalisation, but included
 * here anyway for completeness.
 */
void use_defaults(void) {
	char *home;
	int rtd_length;

	/* allowed commands and hosts should be in here */
	oc_host = malloc(8);
	strncpy(oc_host, "0.0.0.0", 7);
	
	oc_port = OC_PORT;
	backlog = BACKLOG;
	
	logfile_name = malloc(strlen(LOGFILE)+1);
	strcpy(logfile_name, LOGFILE);	
	
	plugindir = malloc(strlen(PLUGINDIR)+1);
	strcpy(plugindir, PLUGINDIR);	

	allowed_commands = NULL;
	plugins = NULL;
	allowed_hosts = NULL;

	home = getenv("HOME");
	rtd_length = strlen(home) + 2 + strlen(DEFAULT_RUNTIMEDIR);
	runtimedir = malloc(rtd_length * sizeof(char));
	snprintf(runtimedir, rtd_length, "%s/%s", home, DEFAULT_RUNTIMEDIR);
}
