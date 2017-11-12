/*
 * Oculus Network Monitor (C) Bart Friederichs
 * oculusd 
 *
 * oculusd is the server part of oculus
 * liboculus.h
 */
 
  /* Generic messages
 * A generic reply starts with a status message:
 * [+-](reply code): (reply message)
 * + means success, - means error
 * Reply codes are made of 3 numbers
 * first number:
 * 1: OK, 2:Warning, 3: Error
 * 
 * Reply codes:
 * 2xx OK
 * 200 Request handled successfully
 * 214 Help message
 * 220 Welcome message
 * 4xx ERROR
 * 404 Command not supported
 * 405 Command not allowed
 * 5xx WARNING 
 * 501 Wrong parameters for command
 * 6xx Server messages
 * 601 Connection closed
 * 602 Server killed
 */

#define OK 					200
#define HELP				214
#define WELCOME				220
#define ERR_UNKNOWN			400
#define ERR_NOSUPPORT 		404
#define ERR_NOALLOW			405
#define ERR_FAILED			406
#define WRN_PARAMNOALLOW	502
#define WRN_WRONGPARAM		501
#define WRN_NOCMD			500
#define RHSH_SERVER       	603
#define KILL_SERVER			602
#define QUIT				601
 
#include <libxml/xpath.h>
#include "globals.h"
#include "event.h"
#include "action.h"
#include "plugindata.h"
 
/* exit nicely */
void oc_exit(int errcode);

/* start oculus */
void oc_start(void);

void oc_cleanup(void);

/* write in the current logfile (which might be stdout)
 * in the following format: Sep 03 13:11:55 <entry>
 * <entry> can be a printf-style string
 */
void oc_writelog(char const *entry,  ...);

/* Remove leading and trailing whitespace from the the NULL terminated str
 */
void strip(char *str);

/* String array functions. Usage:
 * char **str_arr;
 * str_arr = setup_string_array();
 * str_arr = string_array_add(str_arr, "String to add");
 * free_string_array(str_arr);
 */

/* search, return -1 or string index */
int string_array_search(char **array, const char *string);

/* free a previously built string array */
void string_array_free(char **array);

/* create an empty string array, with only one element: NULL */
char **string_array_setup();

/* add a string to the array, a copy will be made of the string (NULL terminated) 
 * return 1 on success, 0 otherwise
 */
char **string_array_add(char **array, char *string);

/* print a string array to stdout */
void string_array_print(char **array);

/* create a NULL terminated array of strings from a whitespace
 * delimited buffer
 */
char **string_array_create(char *string);

/* count strings in array
 */
int string_array_size(char **string_array);

/* Dynamic buffer functions. Usage:
 * char *buffer;
 * buffer = buffer_setup();
 * buffer = buffer_addf(buffer, "Add this number: %d.\n", somenumber);
 * buffer_free(buffer);
 */

/* Setup a dynamic buffer */
char * buffer_setup();

/* Add a string to a buffer, only for internal use.
 * Use buffer_addf(char *buffer, char *data, ...) in your program
 */
char * buffer_add(char * buffer, const char *data);

/* Add a printf-style formatted string to the buffer 
 * Maximum length of the string to be added is fixed
 */
char * buffer_addf(char * buffer, const char *data, ...);

/* Free the buffer */
void buffer_free(char * buffer);

/* Get a char * array of XPath results, return NULL if none found
 */
char **xpath_execute (xmlXPathContextPtr xpathCtx, xmlChar *XPathExpr);

xmlDocPtr get_xmldoc(const char * filename);
void free_xmldoc(xmlDocPtr doc);
const char *xml_get_attr(xmlNodePtr node, const char *attr_name);

void free_result(struct reply *rpl);

/* create a time string from a time_t, has to be free with free() */
char *create_timestring(time_t t, const char *format);

extern struct connection *connection_list;
