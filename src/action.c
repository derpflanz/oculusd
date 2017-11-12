#include "action.h"
#include <string.h>
#include "liboculus.h"
#include <errno.h>

int action_command(const char **args, void *extra) {
	char *cmd;
	int port, maxlen = strlen(args[2]) + strlen(extra);

	port = atoi(args[1]);
	cmd = malloc(maxlen + 1);
	snprintf(cmd, maxlen, args[2], extra);

	/* this action is meant to send a command to another oculus server */
	int result = oc_send_command(args[0], port, cmd);

	if (result == OK) {
		oc_writelog("Sent '%s' to %s:%d\n", cmd, args[0], port);
	}

	return OK;
}

int action_send_mail(const char **args, void *extra) {
	oc_writelog("[not implemented yet] action_send_mail: 1->%s, 2->%s\n", args[0], args[1]);

	return OK;
}

const char *args_command[] = { "host", "port", "command", NULL };
const char *args_send_mail[]    = { "recipient", "subject", NULL };

struct action action_map[] = {
	{ "action_command", args_command, action_command },
	{ "action_send_mail", args_send_mail, action_send_mail },
	{ NULL, NULL, NULL }
};

int (*action_fetch_handler(const char *action_name)) (const char **args, void *extra) {
	int (*action)(const char **args, void *extra) = NULL;
	int i = 0;

	while (action_map[i].name) {
		if (!strcmp(action_map[i].name, action_name)) {
			action = action_map[i].action;
			break;
		}
		i++;
	}
	return action;
}

struct action *action_lookup(const char *action_name) {
	int i = 0;
	struct action *ret = NULL;

	while (action_map[i].name) {
		if (!strcmp(action_map[i].name, action_name)) {
			ret = &(action_map[i]);
			break;
		}
		i++;
	}

	return ret;
}
