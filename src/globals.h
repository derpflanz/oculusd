/* All globals are declared in liboculus.c
 * liboculus.h includes this file
 */

/* defaults */
#define DEFAULT_CONFIGFILE	"/etc/oculusd/oculusd.conf"
#define DEFAULT_RUNTIMEDIR	".oculusd"

extern struct plugin *plugins;
extern char **allowed_commands;
extern struct in_addr *allowed_hosts;
extern int oc_port, backlog, master_port;
extern char *logfile_name, *plugindir, *master_host, *oc_host, *runtimedir;
extern char **allowed_commands;
extern struct in_addr *allowed_hosts;
extern FILE *logfile;
extern char *config_file;
extern char help, quiet, verbose, runasdaemon;
extern int cmdline_port;
