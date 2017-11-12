/*
 * Oculus Network Monitor (C) 2004-2010 Bart Friederichs
 *
 * oculusd is the server part of oculus
 * config.h
 * all configuration is in here:
 * - configfile parsing
 * - global config variables
 * - configuration variable handling
 *
 * Oculus uses XML-style configuration files and thus
 * libxml is needed for it to run 
 */
 
/* config file syntax:
 * <oculusd>
 *  <server>
 *   [server-options]
 *  </server>
 *  <onmp>
 *    [onmp-options]
 *  </onmp>
 * </oculusd>
 *
 * Options must be in XML style, for example <port>1976</port>
 *
 * [server-options]
 * host 				IP where to listen on (0.0.0.0)
 * port					port to listen on (1976)
 * hosts-allow			newline delimited list of IPs that are
 *						allowed to access the daemon (note that there
 *						is no 'hosts-deny' option. When an IP is not in 
 *						the allow list, it gets a 'connection refused') (127.0.0.1) 
 * logfile				file where to log errors and notices (oculus.log)
 *
 * [onmp-options]
 * allowed-commands	newline delimited list of commands that are 
 *						allowed (UPTI UNAM)
 */

/* hard-coded defaults */
#define OC_PORT 1976
#define BACKLOG 10
#define LOGFILE "oculus.log"
#define PLUGINDIR "/usr/local/lib/oculusd"
#define VERSION "0.18"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

void use_defaults(void);
int parse_config_file(const char *filename);
int read_configuration(xmlDocPtr doc);
void print_config();
