/*
 * Oculusd plugin support
 *
 * plugin.c v0.7
 */
 
/*
 * Changes
 * 0.11
 *  o Added monitor loading
 * 0.7
 *  o Added a plugins array, holding pointers to the opened plugins.
 * 0.5
 *  o Intermediate version, fixes a small bug.
 * 0.4
 *  o No more copying of commands and helptexts, point to CS
 *  o Extra error checking on plugins: check for existance of handlers
 *  o Implemented plugin_initialise function
 * 0.2-0.3
 *  No changes kept
 * 0.1
 *  Initial release
 */
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "liboculus.h"
#include "plugindata.h"
#include "plugin.h"
#include <dirent.h>
#include <pcre.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

int match_libocp (const struct dirent *d) {
	pcre *re;
    const char *error;
    int erroffset, rc, ovector[2];
	
	/* regexp: [A-Z]{4} */
    re = pcre_compile("^libocp_.+\\.so$",  PCRE_MULTILINE,  &error,  &erroffset,  NULL); 
	if (re == NULL ) printf ("%s\n", error);
	
	rc = pcre_exec(re, NULL, d->d_name, strlen(d->d_name),  0,  0,  ovector, 2);
	free(re);
	if (rc == 1) return 1;
	else return 0;
}

/* Initialise plugins, call register_plugin to register them 
 * in supported_commands
 */
int plugin_init() {
	extern char *plugindir;
	int n = 0;
	struct dirent **namelist;
	extern char verbose;
	
	if (verbose) 
		printf("\nInitialising plugins...\n");

	if (!plugindir) {
		oc_writelog("No plugindir found. No plugins loaded.\n");
		return -1;
	} else {
		n = scandir(plugindir, &namelist, match_libocp, alphasort);
	}
	
 	if (n == -1) {		
		oc_writelog("Couldn't scan dir %s: %s\n", plugindir, strerror(errno));
	} else {
		while (n--) {
			/* namelist[n]->d_name holds the .so file */
			/* TODO: 
			 * Think about the dlopen handlers (the void *'s), now, it is not
			 * possible to dlclose()
			 */
			if (register_plugin(namelist[n]->d_name) == -1) {
				oc_writelog("Could not register %s\n", namelist[n]->d_name);
			}
			free(namelist[n]);
		}
		free(namelist);
	}
	
	return 0;
}

/* Register the plugin in the linked list 
 * Several checks are performed
 */
int register_plugin(const char *plugin) {
	int r = -1;
	extern char *plugindir, verbose;
	char *error;
	int (*plugin_initialise)(void);
	void *handle;
	struct cmd_table_pu *plugin_commands;
	char *plugin_fullpath;

	if (verbose) printf("  Registering %s: ", plugin);
	
	plugin_fullpath = malloc(strlen(plugindir) + strlen(plugin) + 2);
	strncpy(plugin_fullpath, plugindir, strlen(plugindir)+1);
	strncat(plugin_fullpath, "/", 1);
	strncat(plugin_fullpath, plugin, strlen(plugin)+1);

	handle = dlopen(plugin_fullpath, RTLD_LAZY);
	if (!handle) {
		if (verbose) printf("Failed.\n"); 	/* against screen clutter in verbose mode */
		oc_writelog("Couldn't open %s: %s\n", plugin, dlerror());
		return -1;
	}
	free(plugin_fullpath);

	if (plugin_validate(handle, plugin) != 0) {
		oc_writelog("%s is not a valid oculusd plugin. Not loaded.\n", plugin);
		r = -1;
	} else {
		plugins = plugin_add(handle);
		r = 0;
	}

	return r;
}

int plugin_unload(void *handle) {
	int (*__plugin_unload)(void);

	__plugin_unload = dlsym(handle, "plugin_unload");
	if (__plugin_unload != NULL) {
		/* plugin_unload exists, call it */
		__plugin_unload();
	}
	dlclose(handle);

	return 0;
}

int plugin_validate(void * handle, const char * plugin) {
	int (*plugin_initialise)(void);
	void *plugin_commands, *plugin_monitors;
	char *error;
	
	int commands_found = 0, monitors_found = 0;
	int r = 0;
	
	plugin_initialise = dlsym(handle, "plugin_initialise");
	if (dlerror() != NULL) {
		if (verbose) printf("\n");
		oc_writelog("%s is not an oculusd plugin, couldn't find plugin_initialise().\n", plugin);
		r = -1;
	}
	
	if (0 == r) {
		plugin_commands = dlsym(handle, "plugin_commands");
		if ((error = dlerror()) == NULL) {
			commands_found = 1;
		}
	}
	
	if (0 == r) {
		plugin_monitors = dlsym(handle, "plugin_monitors");
		if ((error = dlerror()) == NULL) {
			monitors_found = 1;
		}
	}

	if (monitors_found == 1 || commands_found == 1) {
		if (plugin_initialise() != 0) {
			oc_writelog("\nCouldn't initialise plugin %s\n", plugin);
			r = -1;
		}
	} else {
		oc_writelog("\nPlugin %s doesn't have commands (%d) or monitors (%d).\n", plugin, commands_found, monitors_found);
	}
		
	return r;
}

struct plugin *plugin_add(void *handle) {
	char error = 0;
	struct cmd_table *cmd = NULL;
	struct mon_table *mon = NULL;
	char *plugin_name = NULL;
	assert(handle != NULL);
	
	/* dlsym the obligatory symbols */
	cmd = dlsym(handle, "plugin_commands");
	mon = dlsym(handle, "plugin_monitors");
	plugin_name = dlsym(handle, "plugin_name");
	
	/* it is okay to check for NULL as dlsym() return value
	 * (see manpage), because we *need* these values to be
	 * not null 
	 */
	if (NULL == cmd && NULL == mon) error = PU_EMPTY;
	if (NULL == plugin_name) error = PU_NONAME;

	if (!error) {
		/* add the plugin to the list */
		struct plugin *new_element;
		new_element = malloc(sizeof(struct plugin));
		if (NULL == new_element) {
			oc_writelog("Could not allocate memory for plugin.\n");
		} else {
			new_element->handle = handle;
			new_element->commands = cmd;
			new_element->monitors = mon;
			new_element->name = plugin_name;
			new_element->description = dlsym(handle, "plugin_description");
			new_element->version = dlsym(handle, "plugin_version");

			if (verbose) {
				int i = 0;
				printf ("%s: ", new_element->name);
				while (cmd && new_element->commands[i].cmd) {
					printf("%s ", new_element->commands[i].cmd);
					i++;
				}
				i = 0;
				while (mon && new_element->monitors[i].handler) {
					i++;
				}
				printf ("%c, monitors: %d\n", 8, i);
			}

			plugin_register_events(handle);

			/* put the new element in the list */
			new_element->next = plugins;
			plugins = new_element;
		}
	}
	
	switch (error) {
		case PU_EMPTY:
			oc_writelog("No 'plugin_commands[]' or 'plugin_monitors[]' in plugin. Plugin not added.\n");
			break;
		case PU_NONAME:
			oc_writelog("No 'plugin_name[]' in plugin. Plugin not added.\n");
			break;
	}
	
	return plugins;
}

int plugin_register_events(void *handle) {
	struct evt_table *plugin_events = dlsym(handle, "plugin_events");
	xmlDocPtr doc = get_xmldoc(config_file);
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc), local_ctx;
	char buffer[1024];
	xmlXPathObjectPtr xpathObj = NULL, xpathObj2 = NULL;

	if (plugin_events != NULL && xpath_ctx != NULL) {
		int i = 0;

		// Cycle over the plugin events (hardcoded in plugin)
		while (plugin_events[i].event_name) {
			snprintf(buffer, 1024, "/oculusd/event[@name='%s']/action", plugin_events[i].event_name);
			xpathObj = xmlXPathEvalExpression(buffer, xpath_ctx);
			if (xpathObj != NULL) {
				/* event is found in config file */
				xmlNodeSetPtr nodes = xpathObj->nodesetval, nodes2;
				xmlNodePtr node, child;
				int size = ((nodes)?nodes->nodeNr:0), j;

				// cycle over all actions connected to this event
				for (j = 0; j < size; j++) {
					int register_event = 1;
					const char *action_name = xml_get_attr(nodes->nodeTab[j], "name");
					int k = 0;
					char **args = string_array_setup();
					struct action *action = NULL;

					oc_writelog("Registering action '%s' to event '%s'\n", action_name, plugin_events[i].event_name);
					action = action_lookup(action_name);

					// and fetch the arguments
					while (action->args[k]) {
						snprintf(buffer, 1024, "/oculusd/event[@name='%s']/action/arg[@name='%s']/text()", plugin_events[i].event_name, action->args[k]);

						xpathObj2 = xmlXPathEvalExpression(buffer, xpath_ctx);

						if (xpathObj2 != NULL) {
							int size2, m;
							nodes2 = xpathObj2->nodesetval;
							size2 = (nodes2)?nodes2->nodeNr:0;

							if (size2 == 1) {
								args = string_array_add(args, nodes2->nodeTab[0]->content);
							} else {
								oc_writelog("Missing argument '%s' for action '%s'; action not connected to event.\n", action->args[k], action->name);
								register_event = 0;
							}
						}

						k++;
					}
					
					if (register_event) {
						event_register(plugin_events[i].event_name, action->action, args);
					}
				}
			}
			i++;
		}
	}

	free_xmldoc(doc);
	return 0;
}

/* debug function */
void plugin_show(void) {
	struct plugin *list;
	list = plugins;
	
	printf ("\n\nLoaded plugins:\n");
	
	while (list) {
		struct cmd_table *commands = NULL;
		int i = 0;
		printf("Name: %s\nDescription: %s\nExported commands: ", 
			list->name, list->description);
		commands = list->commands;
		while (commands[i].cmd) {
			printf("%s ", commands[i].cmd);
			i++;
		}
		printf ("\n");
		list = list->next;
	}
	
}
