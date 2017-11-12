/*
 * Oculus default plugin
 * This plugin services the standard oculus commands
 * UPTI, PROC, UNAM
 */
 
#include "../src/plugindata.h"
#include "../src/network.h"

/* command handlers */
struct reply *handle_upti(char **cmdline, const struct connection *conn);
struct reply *handle_date(char **cmdline, const struct connection *conn);
struct reply *handle_unam(char **cmdline, const struct connection *conn);
struct reply *handle_conn(char **cmdline, const struct connection *conn);
