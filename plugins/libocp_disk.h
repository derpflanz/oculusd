/*
 * libocp_disk.h
 * 
 * Monitors disk usage and registers a thread function to do so.
 */

#include "../src/plugindata.h"
#include "../src/monitor.h"
#include "../src/network.h"

struct reply *handle_disk(char **cmdline, const struct connection *conn);
int monitor_disk();
int monitor_free();

