/*
 * Oculus default plugin
 * This plugin services the standard oculus commands
 * UPTI, PROC, UNAM
 */
 
#include "../src/plugindata.h"
#include "../src/network.h"
#include <signal.h>

/* main handlers */
struct reply *handle_plst(char **cmdline, const struct connection *conn);
struct reply *handle_plog(char **cmdline, const struct connection *conn);
struct reply *handle_psig(char **cmdline, const struct connection *conn);

/* sub handlers */
void handle_proc_list(struct reply *rpl, int human, const char *process);

int plugin_initialise(void);
pid_t getpidfromname(char *processname);

/* Returns a file handler to /proc/pid/stat */
FILE * getstatusfile(pid_t pid);

/* Get status (ZSDTR) from process */
char getstatus(pid_t pid);
