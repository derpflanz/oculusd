/*
 * Oculus Network Monitor (C) 2003-2004 Bart Friederichs
 * oculusd - command.c v0.7
 *
 * oculusd is the server part of oculus
 * command.c
 */

/* Changes
 * 0.8
 *  o Added RSTR command
 * 0.7
 *  o Added SHPL, RELD and UNLD commands
 * 0.4
 *  o Don't copy the contents of the default_cmds array, just point at the content
 *  o Cleaned up KILL code
 *  o Bugfixes:
 *    - Close connection on empty command
 * 0.2-0.3
 *  no changes kept
 * 0.1 
 *  initial release
 */


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "config.h"
#include "command.h"
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "liboculus.h"
#include <signal.h>
#include "plugin.h"
#include <dlfcn.h>

/* these commands will always be available */
struct builtin_cmd_table builtin_commands[] =  {
	{"QUIT", handle_quit, "QUIT closes your current connection"},
	{"HELP", handle_help, "HELP shows info on what is allowed and supported"},
	{"KILL", handle_kill, 
		"KILL kills the listener. All connections stay alive until QUIT."},
	{"RELD", handle_reload, "Unload all plugins and reload the plugin dir."},
	{"UNLD", handle_unload, "Unload plugin, takes <name> as a parameter.\nUse UNLD ALL to unload all plugins."},
	{"SHPL", handle_showplugins, "Shows all loaded plugins."},
	{"RSTR", handle_rehash, "Restarts the listener: re-reads config file and reloads plugins.\nNote: you need to re-connect to the listener for changes to take effect."},
	{"EVNT", handle_event, "Shows currently registered events."},
	{NULL, NULL, NULL}									/* needed for end-of-array checking */
};

int send_reply(int socket, int code) {
	char message[1024];				/* buffer length must be longest possible message */
	
	switch (code) {
		case OK:
			sprintf (message, "+%d: Command completed successfully.\n", OK);
		break;
		case ERR_NOALLOW:
			sprintf(message, "-%d: Command not allowed.\n", ERR_NOALLOW);
		break;
		case ERR_NOSUPPORT:
			sprintf(message, "-%d: Command not supported.\n", ERR_NOSUPPORT);
		break;
		case ERR_FAILED:
			sprintf(message, "-%d: Command failed.\n", ERR_FAILED);
		break;
		case HELP:
			sprintf(message, "+%d: Oculus help system.\n", HELP);
		break;
		case WELCOME:
			sprintf(message, "+%d: Welcome to oculusd v%s.\n", WELCOME, VERSION);
		break;
		case WRN_WRONGPARAM:
			sprintf(message, "-%d: Incorrect parameters for command.\n", WRN_WRONGPARAM);
		break;
		case QUIT:
			sprintf(message, "+%d: Connection closed.\n", QUIT);
		break;
		case KILL_SERVER:
			sprintf(message, "+%d: Server killed.\n", KILL_SERVER);
		break;
		case RHSH_SERVER:
			sprintf(message, "+%d: Server rehashed.\n", RHSH_SERVER);
		break;
		case WRN_NOCMD:
			sprintf(message, "-%d: No command specified.\n", WRN_NOCMD);
		break;
		case WRN_PARAMNOALLOW:
			sprintf(message, "-%d: Parameter not allowed.\n", WRN_PARAMNOALLOW);
		break;
		default:
			sprintf(message, "-%d: Unknown error.\n", ERR_UNKNOWN);
		break;
	}
	
	send (socket, message, strlen(message), 0);
	return code;
}

char *get_help_text(const char *command) {
	char *helptext = NULL;
	
	/* check for builtin command */
	int i = 0;
	while (builtin_commands[i].cmd) {
		if (!strncmp(builtin_commands[i].cmd, command, strlen(command))) {
			helptext = builtin_commands[i].helptext;
		}
		i++;
	}

	if (!helptext) {
		/* no helptext yet, check in the plugin list */
		struct plugin *list;
		list = plugins;
		
		while (list) {
			int i = 0;
			while (list->commands[i].cmd) {
				if (!strncmp(list->commands[i].cmd, command, strlen(command))) {
					helptext = list->commands[i].helptext;
					break;
				}
				i++;
			}		
			list = list->next;
		}
	}
	
	return helptext;
}

/* check if the command is supported
 * return handler when supported, NULL otherwise
 */
struct reply *(*get_cmd_handler(const char *command))(char **cmdline, const struct connection *conn) {
	struct reply * (*ret)(char **cmdline, const struct connection *conn) = NULL;

	if (command == NULL) {
		ret = NULL;
	} else {
		/* check for builtin command */
		int i = 0;
		while (builtin_commands[i].cmd) {
			if (!strncmp(builtin_commands[i].cmd, command, strlen(command))) {
				ret = builtin_commands[i].handler;
			}
			i++;
		}
		
		if (!ret) {
			/* not builtin, in a plugin? */
			struct plugin *list;
			list = plugins;
			while (list && ret == NULL) {
				int i = 0;
				while (list->commands[i].cmd) {
					if (!strncmp(list->commands[i].cmd, command, strlen(command))) {
						ret = dlsym(list->handle, list->commands[i].handler);
						break;
					}
					i++;
				}
				list = list->next;
			}
		}
	}

	return ret;
}

/* check if the command is allowed
 * return 1 if allowed, 0 otherwise
 */
int cmd_is_allowed(const char *command) {
	extern char **allowed_commands;
	int ret = 0, i = 0;
	
	if (allowed_commands && command) {
		while (allowed_commands[i]) {
			if (!strncmp(command, allowed_commands[i], 4)) {
				ret = 1;
				break;
			}
			i++;
		}
	}

	return ret;
}


/* pre: commandline is created with string_array_create
 * post:return the command's result code 
 */
int handle_command (char **commandline, const struct connection *conn) {
	int ret = OK;
	struct reply * (*handler)(char **cmdline, const struct connection *conn);

	if (commandline[0] == NULL) {
		send_reply(conn->socket, WRN_NOCMD);
	} else {
		handler = get_cmd_handler(commandline[0]);
		if (NULL == handler) {
			ret = send_reply(conn->socket, ERR_NOSUPPORT);
		} else {
			if (!cmd_is_allowed(commandline[0])) {
				/* command is not allowed */
				ret = send_reply(conn->socket, ERR_NOALLOW);
			} else {
				struct reply *rpl;
				rpl = (*handler)(commandline, conn);
				/* handlers may return NULL, so this check is important */
				if (rpl) {
					/* Send out reply code and the reply itself */
					send_reply(conn->socket, rpl->code);
					if (rpl->result) send (conn->socket, rpl->result, strlen(rpl->result), 0);
					ret = rpl->code;
					free_result(rpl);
				} else {
					ret = send_reply(conn->socket, ERR_FAILED);
				}
			}
		}
	}
	
	return ret;
}

struct reply * handle_showplugins(char **cmdline, const struct connection *conn) {
	struct reply *rpl = NULL;
	struct plugin *list;
	
	rpl = malloc(sizeof(struct reply));
	rpl->code = OK;
	
	rpl->result = buffer_setup();

	list = plugins;
	while (list) {
		int i = 0;
		rpl->result = buffer_addf(rpl->result, 
			"---------------------------\n"
			"PLUGIN NAME: %s\n"
			"VERSION    : %s\n"
			"DESCRIPTION: %s\n"
			"COMMANDS   : ", 
			list->name, list->version, list->description);
		while (list->commands[i].cmd) {
			rpl->result = buffer_addf(rpl->result, "%s ", list->commands[i].cmd);
			i++;
		}
		rpl->result = buffer_addf(rpl->result, "\n");
		list = list->next;
	}
	
	if (plugins) {
		rpl->result = buffer_addf(rpl->result, "---------------------------\n");
	} else {
		rpl->result = buffer_addf(rpl->result, "No plugins loaded.\n");
	}
	
	return rpl;
}

struct reply * handle_reload(char **cmdline, const struct connection *conn) {
	char **unld_cmdline = NULL;
	struct reply *rpl = NULL, *tmp = NULL;
	
	rpl = malloc(sizeof(struct reply));
	rpl->code = OK;
	rpl->result = buffer_setup();
	
	/* unload all plugins, by calling the UNLD handler */
	unld_cmdline = string_array_setup();
	unld_cmdline = string_array_add(unld_cmdline, "UNLD");
	unld_cmdline = string_array_add(unld_cmdline, "ALL");
	tmp = handle_unload(unld_cmdline, NULL);
	string_array_free(unld_cmdline);

	if (tmp->code != OK) {
		rpl->code = ERR_FAILED;
		rpl->result = buffer_add(rpl->result, "Plugin unloading failed.\n");
	} else if (plugin_init() == -1) {
		rpl->code = ERR_FAILED;
		rpl->result = buffer_addf(rpl->result, "Plugin re-initialisation failed.\n");
	} else {
		rpl->result = buffer_addf(rpl->result, "Plugins re-initialised.\n");
	}

	free_result(tmp);	
	return rpl;
}

/* unloads the plugin defined by cmdline[0]
 */
struct reply * handle_unload(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc(sizeof(struct reply));
	rpl->code = ERR_FAILED;
	
	if (NULL == cmdline[1]) {
		rpl->code = WRN_WRONGPARAM;
	} else if (!strncmp(cmdline[1], "ALL", 3)) {
		/* remove ALL plugins */
		struct plugin *list;
		list = plugins;
		while (list) {
			struct plugin *tmp;
			tmp = list->next;
			plugin_unload(list->handle);
			free(list);
			list = tmp;
		}
		plugins = NULL;
		rpl->code = OK;
		rpl->result = buffer_setup();
		rpl->result = buffer_addf(rpl->result, "All plugins removed.\n");
	} else {
		/* try to unload the defined plugin */
		struct plugin *list, *prev;
		list = plugins;
		prev = NULL;
		
		while (list) {
			if (!strncmp(list->name, cmdline[1], strlen(cmdline[1]))) {
				/* we have a plugin name match */
				if (NULL == prev) {
					/* the match is the first in the list */
					plugins = list->next;
				} else {
					prev->next = list->next;
				}
				plugin_unload(list->handle);
				free(list);
				rpl->code = OK;
				rpl->result = buffer_setup();
				rpl->result = buffer_addf(rpl->result, "Plugin %s removed.\n", cmdline[1]);
				break;
			} else {
				/* no name match, continue in the list */
				prev = list;
				list = list->next;
			}
		}
	}

	/* setup reply, when failure */
	switch (rpl->code) {
		case WRN_WRONGPARAM:
			rpl->result = buffer_setup();
			rpl->result = buffer_addf(rpl->result, "No plugin name given.\n");
		break;
		case ERR_FAILED:
			rpl->result = buffer_setup();
			rpl->result = buffer_addf(rpl->result, "No such plugin %s.\n", cmdline[1]);
		break;
	}
	
	return rpl;
}

/* QUIT is default, it closes the connection 
 * All the function does is returning a struct reply with the code
 * set to QUIT
 */
struct reply * handle_quit(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc(sizeof(struct reply));
	rpl->code = QUIT;
	rpl->result = NULL;

	return rpl;
}

/* RHSH, rehash server 
 * Do this by sending SIGHUP to the parent process
 */
struct reply * handle_rehash(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc(sizeof(struct reply));
	rpl->code = RHSH_SERVER;
	rpl->result = NULL;

	return rpl;
}

/* KILL kills off the server
 */
struct reply *handle_kill(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	rpl = malloc(sizeof(struct reply));
	rpl->result = NULL;	
	rpl->code = KILL_SERVER;

	return rpl;
}

/* show a list of the currently registered events */
struct reply * handle_event(char **cmdline, const struct connection *conn) {
	struct reply *ret;
	struct event *ev;
	ret = (struct reply *)malloc(sizeof(struct reply));
	ret->code = OK;
	ret->result = buffer_setup();
	ret->result = buffer_add(ret->result, "Currently set events:\n");

	ev = event_list;
	while (ev) {
		ret->result = buffer_addf(ret->result, "%s calls %p\n", ev->event_name, ev->action);
		ev = ev->next;
	}

	return ret;
}

/* HELP shows the supported and allowed commands
 * HELP <command> shows the command helptext
 * 0.7: Using the new plugin system
 * 0.4: Changed so that it returns a struct reply *
 * 0.3: HELP <keyword> shows help about that keyword
 */
struct reply *handle_help(char **cmdline, const struct connection *conn) {
	extern char **allowed_commands;
	struct reply *ret;
	
	ret = malloc (sizeof(struct reply));
	ret->code = HELP;
	ret->result = buffer_setup();

	if (cmdline[1]) {
		char *helptext = NULL;
		helptext = get_help_text(cmdline[1]);
		if (!helptext) {
			ret->result = buffer_addf(ret->result, "No help available for command.");
		}	else {
			ret->result = buffer_addf(ret->result, "%s: %s", cmdline[1], helptext);
		}
	} else {
		struct plugin *list;
		int i = 0;
		list = plugins;
		ret->result = buffer_addf(ret->result, "Supported commands: ");

		while (builtin_commands[i].cmd) {
			ret->result = buffer_addf(ret->result, "%s ", builtin_commands[i].cmd);
			i++;
		}
		
		while (list) {
			struct cmd_table *commands = NULL;
			i = 0;
			commands = list->commands;
			while (commands[i].cmd) {
				ret->result = buffer_addf(ret->result, "%s ", commands[i].cmd);
				i++;
			}
			list = list->next;
		}
		
		ret->result = buffer_addf(ret->result,  "\nAllowed commands: ");
		
		i = 0;
		while (allowed_commands[i]) {
			ret->result = buffer_addf(ret->result,  "%s ", allowed_commands[i]);
			i++;
		}
		ret->result = buffer_addf(ret->result,  
			"\nUse HELP <command> for help on a specific command.");
	}
	
	ret->result = buffer_addf(ret->result,  "\n");	
	return ret; 
}


