/*
 * libocp_process.c
 * Oculusd plugin that monitors processes
 */
 
#include <errno.h>
#include "../src/plugindata.h"
#include "../src/liboculus.h"
#include "libocp_process.h"
#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <dirent.h>
#include <string.h>

/*
 * This plugin registers the following commands:
 * PROC: Show information about running processes
 *   PROC	[name]		Show a list of monitored projects, 
 *  								and their status
 *   PROC HUMAN	[name]
 * 									Idem, human readable
 *   LOG [name]	    Show some lines of the logfile  (defined in configfile)
 *   SIG [name] <signal> 
 *									Send <signal> to process <name>
 */
 
#define DEFAULT		0
#define HUMAN		1

#include "../src/liboculus.h"

char *plugin_configfile = NULL;	/* this plugin's config file */
char **processes = NULL;				/* the processes to monitor */

/* XML pointers */
xmlDocPtr xml_config_file = NULL;
xmlXPathContextPtr xpathCtx = NULL;

char plugin_name[] = "process";
char plugin_description[] = "Handles all kinds of process monitoring functions.";
char plugin_version[] = "1.0";

/* commands supported by this plugin */
struct cmd_table plugin_commands[] =  {
	{"PLST", "handle_plst", "Show monitored processes.\n" 
		"Shows the processes being monitored, in a numerical way.\n"
		"Usage:\n"
		"PLST [HUMAN] [process]\n"
		"HUMAN: Show information in a human readable format\n"
		"[process]: Show detailed process information.\n"
	},
	{"PLOG", "handle_plog", "Show last lines of logfile.\n"
		"Usage:\n"
		"  PLOG [process]\n"
		"Send <signal> to <process>. What signals can be received by what \n"
		"processes is defined in the configuration file.\n"
	},
	{"PSIG", "handle_psig", "Send a signal to a process, if allowed.\n"
		"Usage:\n"
		"  PSIG [process] [signal]\n"
	},
	{NULL, NULL, NULL}			/* needed for end-of-array checking */
};

/* plugin initialising */
int plugin_initialise(void) {
	int ret = 0;

	xml_config_file = get_xmldoc(config_file);

	if (xml_config_file != NULL) {
		char **temp = NULL;
		xpathCtx = xmlXPathNewContext(xml_config_file);
		temp = xpath_execute(xpathCtx, 
			"/oculusd/server/plugin[@name='libocp_process']/config/text()");
		if (temp[0]) {
			plugin_configfile = buffer_setup();
			plugin_configfile = buffer_add(plugin_configfile, temp[0]);
		}
		string_array_free(temp);
		free_xmldoc(xml_config_file);
	} else {
		ret = 1;
	}

	if (NULL != plugin_configfile) {
		xml_config_file = get_xmldoc(plugin_configfile);
		if (xml_config_file != NULL) {
			char *process;
			xpathCtx = xmlXPathNewContext(xml_config_file);
			processes = xpath_execute(xpathCtx, "/processplugin/process/@name");
		} else {

			oc_writelog("Couldn't open plugin configuration file %s: %s\n",
				plugin_configfile, strerror(errno));
			ret = 1;
		}
	} else {
		if (verbose) printf("\n");	/* against screen clutter */
		oc_writelog("libocp_process: No config file given for plugin.\n");
		ret = 1;
	}
	return ret;
}

struct reply *handle_psig(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc(sizeof(struct reply));
	
	if (!cmdline[1]) {
		rpl->code = WRN_WRONGPARAM;
		rpl->result = buffer_setup();
		rpl->result = buffer_add(rpl->result, "Missing process name.\n");
	} else if (!cmdline[2]) {
		rpl->code = WRN_WRONGPARAM;
		rpl->result = buffer_setup();
		rpl->result = buffer_add(rpl->result, "Missing signal.\n");	
	} else {
		/* the command line was okay, now check for validity */
		if (string_array_search(processes, cmdline[1]) == -1) {
			/* unmonitored process */
			rpl->code = WRN_WRONGPARAM;
			rpl->result = buffer_setup();
			rpl->result = buffer_add(rpl->result, "Process is not monitored.\n");
		} else {
			int signal = 0;
			char *XPath = buffer_setup();
			char **allowed_signal;
			
			signal = atoi(cmdline[2]);
			XPath = buffer_setup();
			XPath = buffer_addf(XPath,
				"/processplugin/process[@name='%s']/allowed-signal[@id='%d']/@id", 
				cmdline[1], signal);

			allowed_signal = xpath_execute(xpathCtx, XPath);
			
		
			if (allowed_signal[0] == NULL) {
				rpl->code = WRN_PARAMNOALLOW;
				rpl->result = buffer_setup();
				rpl->result = buffer_add(rpl->result, "Signal not allowed for process.\n");
			} else {
				/* allowed signal is allowed_signal[0] */
				int signal = atoi (allowed_signal[0]);
				pid_t pid = getpidfromname(cmdline[1]);

				if (pid == 0) {
					rpl->code = ERR_FAILED;
					rpl->result = buffer_setup();
					rpl->result = buffer_addf(rpl->result, "Process is not running\n");
				} else if ((kill(pid, signal)) == -1) {
					rpl->code = ERR_FAILED;
					rpl->result = buffer_setup();
					rpl->result = buffer_addf(rpl->result, 
						"Signalling failed: %s\n", strerror(errno));
				} else {
					rpl->code = OK;
					rpl->result = buffer_setup();
					rpl->result = buffer_add(rpl->result, "Signal sent\n");
				}
			}
		}
	}
	
	return rpl;
}

struct reply *handle_plog(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc(sizeof(struct reply));
	
	if (!cmdline[1]) {
		rpl->code = WRN_WRONGPARAM;
		rpl->result = buffer_setup();
		rpl->result = buffer_add(rpl->result, "Missing process name.\n");
	} else {
		char **logfilename = NULL, **linestr = NULL;
		char * XPath = buffer_setup();
		FILE *logfile = NULL;
		int lines = 0;
		
		if (string_array_search(processes, cmdline[1]) == -1) {
			/* unmonitored process */
			rpl->code = WRN_WRONGPARAM;
			rpl->result = buffer_setup();
			rpl->result = buffer_add(rpl->result, "Process is not monitored.\n");
		} else {
			rpl->code = OK;
			rpl->result = NULL;
			XPath = buffer_addf(XPath, 
				"/processplugin/process[@name='%s']/log/name/text()", cmdline[1]);
			logfilename = xpath_execute(xpathCtx, XPath);
			
			buffer_free(XPath);
			XPath = buffer_setup();
			XPath = buffer_addf(XPath,
				"/processplugin/process[@name='%s']/log/lines/text()", cmdline[1]);
			linestr = xpath_execute(xpathCtx, XPath);
			
			if (linestr[0]) lines = atoi (linestr[0]);
			
			if (lines == 0) lines = 5;	/* default */
	
			/* default */
			if (logfilename[0] == NULL) {
				char *temp = NULL;
				temp = buffer_setup();
				temp = buffer_addf(temp, "/var/log/%s.log", cmdline[1]);
				logfilename = string_array_setup();
				logfilename = string_array_add(logfilename, temp);
				oc_writelog("Logfile for %s not found, using default %s\n", 
					cmdline[1], logfilename[0]);
				buffer_free(temp);
			}
			
			logfile = fopen(logfilename[0], "r");
			if (!logfile) {
				rpl->code = ERR_FAILED;
				rpl->result = buffer_setup();
				rpl->result = buffer_addf(rpl->result, 
					"Could not open %s for reading: %s\n", logfilename[0], strerror(errno));
			} else {
				long offset = 0, endoffile = 0, length = 0, len;
				char *line = malloc(10);
				int i = 0;
				char *buffer = NULL;
				fseek(logfile, offset, SEEK_END);
				endoffile = ftell(logfile);
				for (i = 0; i <= lines; ++i) {
					while (fgetc(logfile) != '\n' && offset < endoffile) {
						offset++;
						fseek(logfile, -offset, SEEK_END);
					}
				}
				if (offset >= endoffile) fseek(logfile, 0, SEEK_SET);
	
				/* the offset was negative relative to the end, we need it 
				 * positive now
				 */
				offset = endoffile - offset;
				
				length = (endoffile - offset) + 1;
				buffer = malloc(length+2);
				length = fread(buffer, 1, length, logfile);
				
				/* fix files that do not end with a newline. this also means an extra
				 * line will be shown when a file doesn't end on a newline 
				 */
				if (buffer[length-1] != '\n') {
					/* ugly logfile, didn't end with \n */
					buffer[length] = '\n';
					buffer[length+1] = 0;
				} else {
					/* good logfile, finish it */
					buffer[length] = 0;
				}
				
				rpl->result = buffer;
				fclose(logfile);
			}

			string_array_free(logfilename);
		}
	}
	
	return rpl;
}

struct reply *handle_plst(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	extern char *config_file;
	rpl = malloc (sizeof(struct reply));	

	if (!cmdline[1]) {
		handle_proc_list(rpl, DEFAULT, NULL);
	} else if (cmdline[1] && strncmp(cmdline[1], "HUMAN", 5) == 0) {
		handle_proc_list(rpl, HUMAN, cmdline[2]);
	} else {
		handle_proc_list(rpl, DEFAULT, cmdline[1]);
	}

	return rpl;
}



/* Get the numerical pid(s) of a process, from its name
 * Return value:
 * -1:	an error occurred
 *  0:  process is not running
 *  n: the PID of that process
 */
pid_t getpidfromname(char *processname) {
	DIR *proc = NULL;
	struct dirent *entry = NULL;
	pid_t ret = 0;

	proc = opendir("/proc");
	if (proc == NULL) {
		oc_writelog("libocp_process: Couldn't open directory /proc\n");
		ret = -1;
	} else {
		while ((entry = readdir(proc))  && 0 == ret) {
			if (entry->d_type == DT_DIR) {
				FILE *status = NULL;
				char *buf = buffer_setup();
				buf = buffer_addf(buf, "/proc/%s/stat", entry->d_name);
				status = fopen(buf, "r");
				
				if (status) {
					char *namefromstatus = malloc(256);
					/* Use intermediate value, otherwise the while loop will stop too early */
					int pid; 
					fscanf(status, "%d (%256s", &pid, namefromstatus);
					if (!strncmp(namefromstatus, processname, strlen(processname))) ret = pid;
					free(namefromstatus);
					fclose(status);
				}
				
				buffer_free(buf);
			}
		}
	}
	
	return ret;
}

FILE *getstatusfile(pid_t pid) {
	FILE *ret = NULL;
	char *buf = buffer_setup();
	buf = buffer_addf(buf, "/proc/%d/stat", pid);
	ret = fopen(buf, "r");	
	
	buffer_free(buf);
	return ret;
}

char getstatus(pid_t pid) {
	FILE *status = getstatusfile(pid);
	char ret;
	
	fscanf(status, "%*d %*s %c", &ret);
	
	fclose(status);
	return ret;
}

void handle_proc_list(struct reply *rpl, int human, const char *process) {
	int i = 0;

	rpl->result = buffer_setup();
	if (process) {
		if (string_array_search(processes, process) == -1) {
			/* unmonitored process */
			rpl->code = WRN_PARAMNOALLOW;
			if (human) rpl->result = buffer_add(rpl->result, "Process is not monitored.\n");
			else rpl->result = buffer_add(rpl->result, "not_monitored\n");
		} else {
			char *statusfilename = buffer_setup();
			pid_t pid = getpidfromname((char *)process);
			FILE *statusfile = NULL;
			
			switch (pid) {
				case 0:
					rpl->code = OK;
					if (human) rpl->result = buffer_addf(rpl->result, "Process not running.\n");
					else rpl->result = buffer_addf(rpl->result, "0\n");
				break;
				case -1:
					rpl->code = ERR_FAILED;
					if (human) rpl->result = buffer_addf(rpl->result, "PID retrieval failed, see logfile for details.\n");
					else rpl->result = buffer_addf(rpl->result, "0\n");
				break;
				default:
					if (human) statusfilename = buffer_addf(statusfilename, "/proc/%d/status", pid);
					else statusfilename = buffer_addf(statusfilename, "/proc/%d/stat", pid);
					
					statusfile = fopen(statusfilename, "r");
					if (!statusfile) {
						rpl->code = ERR_FAILED;
						if (human) rpl->result = buffer_addf(rpl->result, 
							"Couldn't open status file %s: %s.\n", statusfile, strerror(errno));
						else rpl->result = buffer_addf(rpl->result, "0 (%s: %s)\n", statusfile, strerror(errno));
					} else {
						int lastchar = 0;
						char *buffer = malloc(1024);						
						rpl->code = OK;

						while (!feof(statusfile)) {
							buffer = fgets(buffer, 1024, statusfile);
							if (buffer) rpl->result = buffer_addf(rpl->result, "%s", buffer);
						}
						free(buffer);
					}
				break;
			}
		}	
	} else {
		if (human) rpl->result = buffer_addf(rpl->result, "Monitored processes:\n");
		while (processes[i]) {
			pid_t pid = getpidfromname(processes[i]); 
			rpl->result = buffer_addf(rpl->result, "%s", processes[i]);		/* name */
			switch (pid) {
				case -1:
					if (human) {
						rpl->result = buffer_addf(rpl->result, 
							", PID retrieval failed, see logfile for details\n");	/* PID -1 */
					} else {
						rpl->result = buffer_addf(rpl->result, " -1");
					}
					rpl->code = ERR_FAILED;
				break;	
				case 0:																											/* PID 0 */
					if (human) rpl->result = buffer_addf(rpl->result, ", not running\n");
					else rpl->result = buffer_addf(rpl->result, " 0\n");
					rpl->code = OK;
				break;
				default:
					if (human) {																							/* PID n */
						rpl->result = buffer_addf(rpl->result, ", running, PID %d (%c)\n",
							pid, getstatus(pid));
					} else {
						rpl->result = buffer_addf(rpl->result, " %d %c\n",
							pid, getstatus(pid));
					}
					rpl->code = OK;
				break;
			}
			i++;
		}
	}
	
}
