/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.7
 *
 * oculusd is the server part of oculus
 * command.h
 */

#include "plugindata.h"
#include "network.h"

/* API functions */
int handle_command (char **commandline, const struct connection *conn);	/* returns the reply code */
int cmd_is_allowed(const char *command);
int cmd_is_supported(const char *command);	/* return command number if supported, -1 otherwise */
int send_reply(int socket, int code);
int register_builtin_cmds (void);		/* register default commands, build supported commands array */

/* Builtin command handlers */
struct reply * handle_help(char **cmdline, const struct connection *conn);
struct reply * handle_quit(char **cmdline, const struct connection *conn);
struct reply * handle_kill(char **cmdline, const struct connection *conn);
struct reply * handle_reload(char **cmdline, const struct connection *conn);
struct reply * handle_unload(char **cmdline, const struct connection *conn);
struct reply * handle_showplugins(char **cmdline, const struct connection *conn);
struct reply * handle_rehash(char **cmdline, const struct connection *conn);
struct reply * handle_event(char **cmdline, const struct connection *conn);

/* Get the handler of the command 'command' */
struct reply *(*get_cmd_handler(const char *command))(char **cmdline, const struct connection *conn);

/* Get helptext of command */
char *get_help_text(const char *command);

struct builtin_cmd_table {
	char *cmd;
	struct reply * (*handler)(char **cmdline, const struct connection *conn);
	char *helptext;
};
