/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd 
 *
 * oculusd is the server part of oculus
 * liboculus.c 
 * this holds some generic functions for use for oculus
 */

#include "liboculus.h" 
#include "config.h"
#include <errno.h>
#include "plugin.h"
#include "network.h"
#include "sighndlr.h"
#include "monitor.h"
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <pcre.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml2/libxml/tree.h>

/* Global variables, usable by every oculusd source file, and all the plugins 
 */
char 
	help = 0, verbose = 0, quiet = 0, 
	runasdaemon = 0, caughthup = 0;

char 
	*logfile_name = NULL, *oc_host = NULL, 
	*plugindir = NULL, *config_file = NULL,
	*runtimedir = NULL, *master_host = NULL;

int 
	cmdline_port = -1, oc_port = OC_PORT, 
	backlog = BACKLOG, master_port = 0;

struct connection *connection_list = NULL;

FILE *logfile;			/* this will be stdout when not running as a daemon */

/* configuration arrays, default NULL, set by config.c:parse_config_file() */
char **allowed_commands = NULL;
struct in_addr *allowed_hosts = NULL;
struct plugin *plugins = NULL;		/* the root element of a linked list */
struct monitor *monitors = NULL;	/* root element of linked list */

/* result freeing function */
void free_result(struct reply *rpl) {
	free(rpl->result);
	free(rpl);
}

void oc_start(void) {
	extern char *config_file;
	extern char quiet, verbose;
	extern int cmdline_port, oc_port;
	DIR *d_runtime;

	/* config file parsing */
	if (parse_config_file(config_file) == -1) {
		if (!quiet) oc_writelog("Couldn't parse config file %s. Using defaults.\n", config_file);
		use_defaults();
	}

	/* check and create runtimedir */
	d_runtime = opendir(runtimedir);
	if (d_runtime == NULL) {
		/* doesn't exist, try to create */
		if (mkdir(runtimedir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
			oc_writelog("Warning: %s does not exist and cannot be created: %s\n", runtimedir, strerror(errno));
		} else {
			oc_writelog("Created runtime dir: %s\n", runtimedir);
		}
	} else {
		oc_writelog("Using runtime dir: %s\n", runtimedir);
	}

	if (cmdline_port != -1) oc_port = cmdline_port;

	/* initialise plugins */
	if (plugin_init() == -1) {
		if (!quiet) {
			oc_writelog("FATAL: Couldn't register any commands.\n");
			exit(-1);
		}
	}

	/* register signal handlers */
	if (initialise_signals()  != 0) {
		oc_writelog("FATAL: Couldn't initialise signal handlers\n");
		oc_exit(-1);
	}
	
	if (verbose) print_config(stdout);
	
	/* welcome message */
	if (!quiet) oc_writelog("Oculusd v%s started.\n", VERSION);
	
	if (oc_start_daemon() == -1) {
		oc_writelog("FATAL: Couldn't start daemon.\n");
		oc_exit(-1);
	}
}

/* exit nicely */
void oc_exit(int errcode) {
	exit(errcode);
}

void oc_cleanup(void) {
	struct reply *tmp;
	char **unld_cmdline;
	extern char **allowed_commands;
	extern struct in_addr *allowed_hosts;
	extern char *plugindir, *logfile_name;
	extern char verbose;

	/* unload all plugins, by calling the UNLD handler */
	unld_cmdline = string_array_setup();
	unld_cmdline = string_array_add(unld_cmdline, "UNLD");
	unld_cmdline = string_array_add(unld_cmdline, "ALL");
	tmp = handle_unload(unld_cmdline, NULL);
	string_array_free(unld_cmdline);
	if (verbose) oc_writelog("Cleaning up:\n");
	if (verbose) oc_writelog("  plugins unloaded\n");
	
	/* free the command lists */
	string_array_free(allowed_commands);
	free(plugindir);
	free(allowed_hosts);
	free(oc_host);
	free(logfile_name);
	
	if (verbose) {
		oc_writelog("  freed memory: allowed_commands, allowed_hosts, oc_host, \n");
		oc_writelog("  logfile_name, plugindir\n");
	}

	/* clean up hardcoded stuff */
	event_cleanup();
}

char *create_timestring(time_t t, const char *format) {
	struct tm *tmp;
	char *ret = (char *)malloc(1024);
	tmp = localtime(&t);

	strftime(ret, 1024, format, tmp);

	return ret;
}

/* write in the current logfile (which might be stdout)
 * in the following format: Sep 03 13:11:55 <entry>
 * <entry> can be a printf-style string
 */
void oc_writelog(char const *entry,  ...) {
	extern FILE *logfile;
	FILE *output;
	char *timestring;
	va_list ap;

	timestring = create_timestring(time(NULL), "%Y-%m-%d %H:%M:%S");
	
	if (logfile) output = logfile;
	else output = stdout;

	fprintf (output, "%s ", timestring);
	
	va_start(ap, entry);
	vfprintf (output, entry, ap);
	va_end(ap);
	
	free(timestring);
}


/* String array functions. Usage:
 * char **str_arr;
 * str_arr = string_array_setup();
 * str_arr = string_array_add(str_arr, "String to add");
 * string_array_free(str_arr);
 */

/* create an empty string array, with only one element: NULL */
char **string_array_setup() {
	char **ret = malloc(sizeof(char *));
	ret[0] = NULL;
	
	return ret;
}

/* add a string to the array, a copy will be made of the string (NULL terminated) 
 * return 1 on success, 0 otherwise
 */
char **string_array_add(char **array, char *string) {
	char **ret = NULL;
	int size = -1;
	char *new, **t;

	if (array == NULL) return NULL;		/* not set up correctly */
	if (string == NULL) return array;		/* nothing to add */
	
	/* make a copy of the string */
	new = malloc(strlen(string)+1);
	strcpy(new, string);
	
	/* add it to the array */
	while (array[++size]);
	
	t = realloc(array, (size+2)*sizeof(char *));
	if (t) {
		ret = t;
		ret[size] = new;
		ret[size+1] = NULL;
	}

	return ret;
}

/* search for a beginning string in the array, return -1 if not found, 
 * the index otherwise, it 
 */
int string_array_search(char **array, const char *string) {
	int i = 0, ret = -1;
	int length = strlen(string);
	
	while (array[i]) {
		if (strncmp(string, array[i], length) == 0) {
			ret = i;
			break;
		}
		i++;
	}
	
	return ret;
}

/* print a string array to stdout */
void string_array_print(char **array) {
	int i=0;
	
	if (array) {
		while (array[i]) {
			printf("%d: %s\n", i, array[i]);
			i++;
		}
	} else {
		printf("Array is leeg.\n");
	}
}

// PRE: a valid string array (NULL terminated!)
int string_array_size(char **string_array) {
	int i = 0;

	while (string_array[i] != NULL) i++;

	return i;
}


/* create a NULL terminated array of strings from a whitespace
 * delimited buffer -- this will be deprecated eventually
 */
char **string_array_create(char *string) {
	char **ret = NULL, **t;
	char *buffer;
	pcre *re;
	const char *error;
	int erroffset, rc = 1, arraylength = 0, ovector[2], i = 0;
	
	/* the temp buffer can never hold more than the entire string */
	buffer = malloc(strlen(string)+1);

	/* we accept words or sentences with double quotes */
	re = pcre_compile("[A-Za-z0-9]+|\\\"[A-Za-z0-9\\s\\.'\\?]+\\\"", PCRE_MULTILINE,  &error,  &erroffset,  NULL);
	if (re == NULL ) oc_writelog ("%s\n", error);

	while (rc > 0) {
		rc = pcre_exec(re, NULL, string, strlen(string),  i,  0,  ovector, 2);
		pcre_copy_substring(string, ovector, rc, 0, buffer, strlen(string));
		
		if (rc > 0) {
			/* we found a word, put it in our string array list */
			t = (char **)realloc(ret, (arraylength+1)*sizeof(char *));
			if (t) {
				ret = t;
				ret[arraylength] = malloc(strlen(buffer)+1);
				strncpy(ret[arraylength], buffer, strlen(buffer));
				/* close string, because the buffer isn't null terminated */
				ret[arraylength][strlen(buffer)] = '\0';
				arraylength++;
			} else {
				oc_writelog("Realloc failed in string_array_create(). Stop.\n");
				oc_exit(1);
			}
		}
		i = ovector[1];
	} 
	
	/* close the array */
	t = (char **)realloc(ret, (arraylength+1)*sizeof(char *));
	if (t) {
		ret = t;
		ret[arraylength] = NULL;
	} else {
		oc_writelog("Realloc failed in string_array_create(). Stop.\n");
		oc_exit(1);
	}
	
	return ret;
}


/* free the string array created in string_array_create 
 */
void string_array_free(char **array) {
	int i = 0;
	
	if (array) {
		while (array[i]) {
			free(array[i++]);
		}
		free(array);
	}
}


/* Remove leading and trailing whitespace from the the NULL terminated str
 *
 * This code was taken from 
 * http://www.ks.uiuc.edu/Research/vmd/plugins/doxygen/corplugin_8c-source.html
 * and edited to also strip newlines and tabs as well as for style.
 */
void strip(char *str) {
	int n = strlen(str);
    char *beg, *end;
    beg = str;
    end = str + (n-1); /* Point to the last non-null character in the string */
 
   /* Remove leading whitespace */
   while(*beg == ' ' || *beg == '\n' || *beg == '\t') beg++;
 
   /* Remove trailing whitespace and null-terminate */
   while(*end == ' ' || *end == '\n' || *end == '\t') end--;
   *(end+1) = '\0';
 
   /* Shift the string, this introduces some overhead */
   memmove(str, beg, (end - beg + 2));
   return;
}

/* Dynamic buffer functions. Usage:
 * char *buffer;
 * buffer = buffer_setup();
 * buffer = buffer_addf(buffer, "Add this number: %d.\n", somenumber);
 * buffer_free(buffer);
 */

/* Setup a dynamic buffer */
char * buffer_setup() {
	char *buffer;
	buffer = malloc(1);
	buffer[0] = '\0';
	return buffer;
}


/* Add a string to a buffer, only for internal use.
 * Use buffer_addf(char *buffer, char *data, ...) in your program
 */
char * buffer_add(char * buffer, const char *data) {
	if (data != NULL) {
		char *temp;

		temp = realloc(buffer, strlen(buffer)+strlen(data)+1);

		if (temp) {
			buffer = temp;
			strncat(buffer, data, strlen(data));
		}
	}
	return buffer;
}

/* Add a printf-style formatted string to the buffer 
 * Maximum length of the string to be added is fixed
 */
char *buffer_addf(char *buffer, const char *data, ...) {
	char temp_buffer[1024];
	va_list ap;
	va_start (ap, data);
	vsnprintf(temp_buffer, 1023, data, ap);
	va_end(ap);

	/* Okay, we built the string, now copy it into our buffer, and return */
	return buffer_add(buffer, temp_buffer);
}

/* Free the buffer */
void buffer_free(char *buffer) {
	free(buffer);
}

const char *xml_get_attr(xmlNodePtr node, const char *attr_name) {
	const char *r = NULL;

	if (node != NULL) {
		xmlAttr *attr = node->properties;
		while (attr) {
			if (!strcmp(attr->name, attr_name)) {
				r = attr->children->content;
				break;
			}
			attr = attr->next;
		}
	}
}

/* Get a char * array of XML nodenames
 */
char **xpath_execute (
	xmlXPathContextPtr xpathCtx, xmlChar *XPathExpr) {
	char **ret = NULL;
	xmlXPathObjectPtr xpathObj = NULL;
	int i = 0;
	xpathObj = xmlXPathEvalExpression(XPathExpr, xpathCtx);	

//	printf("Executed %s:\n", XPathExpr);

	if (xpathObj != NULL) {
		xmlNodeSetPtr nodes = xpathObj->nodesetval;
		int size = (nodes)?nodes->nodeNr:0;
		/* we found at least one result */
//		printf("Found %d results \n", size);
		ret = string_array_setup();
		for (i = 0; i < size; ++i) {
			xmlNodePtr node = nodes->nodeTab[i];
			xmlNodePtr child;
			
			switch (node->type) {
				case XML_ATTRIBUTE_NODE:
				case XML_ELEMENT_NODE:
				default:
				 	child = node->children;
					while (child) {
						if (child->type == XML_TEXT_NODE) {
							ret = string_array_add(ret, child->content);	
						}	
						child = child->next;
					}
				break;
				case XML_TEXT_NODE:
					ret = string_array_add(ret, node->content);
//					printf("Adding: %s\n", node->content);
				break;
			}
		}
	}


//	string_array_print(ret);

	return ret;
}

xmlDocPtr get_xmldoc(const char * filename) {
	xmlDocPtr doc = NULL;
	xmlInitParser();
	LIBXML_TEST_VERSION
	
	doc = xmlReadFile(filename, NULL, XML_PARSE_DTDVALID);
	return doc;
}

void free_xmldoc(xmlDocPtr doc) {
	xmlFreeDoc(doc);
	xmlCleanupParser();
}
