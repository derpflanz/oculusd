#include "action.h"

int action_alarm_master(const char **args, void *extra) {
	oc_writelog("[not implemented yet] action_alarm_master: 1->%s, 2->%s, 3->%s\n", args[0], args[1], args[2]);

	return OK;
}

int action_send_mail(const char **args, void *extra) {
	oc_writelog("[not implemented yet] action_send_master: 1->%s, 2->%s\n", args[0], args[1]);

	return OK;
}

int action_set_alarm(const char **args, void *extra) {

	return OK;
}



const char *args_alarm_master[] = { "host", "port", "alarm_uid", NULL };
const char *args_set_alarm[]    = { "alarm_uid", NULL };
const char *args_send_mail[]    = { "recipient", "subject", NULL };

struct action action_map[] = {
	{ "action_alarm_master", args_alarm_master, action_alarm_master },
	{ "action_set_alarm", args_set_alarm, action_set_alarm },
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
	struct action *ret;

	while (action_map[i].name) {
		if (!strcmp(action_map[i].name, action_name)) {
			ret = &(action_map[i]);
			break;
		}
		i++;
	}

	return ret;
}
