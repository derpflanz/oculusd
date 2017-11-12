/*
 * Oculusd plugin support
 */
 
#include <dirent.h>
#include "plugindata.h"

#ifndef __PLUGIN_H
#define __PLUGIN_H

#define PU_NONAME	1
#define PU_NOHAND	3
#define PU_EMPTY	5

struct plugin {
	void *handle;			/* as opened by dlopen() */
	char *name;
	char *description;
	char *version;
	struct cmd_table *commands;
	struct mon_table *monitors;
	struct plugin *next;
};

int plugin_init();
int match_libocp (const struct dirent *d);
int register_plugin(const char *plugin);
int register_commands(struct cmd_table *plugin_commands, void *handle);
struct plugin *plugin_add(void *handle);
int plugin_unload(void *handle);
int plugin_register_events(void *handle);
int plugin_validate(void * handle, const char * plugin);
#endif
