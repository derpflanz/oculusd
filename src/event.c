#include "event.h"
#include <stdlib.h>
#include <string.h>

struct event *event_list = NULL;


struct event *event_create(const char *name, int (*action) (const char **args, void *extra), char **args) {
	struct event *ev = NULL;
	ev = (struct event *)malloc(sizeof(struct event));

	if (ev) {
		ev->event_name = strdup(name);
		ev->action = action;
		ev->args = (const char **)args;
		ev->next = NULL;
	}

	return ev;
}

int event_register(const char *name, int (*action) (const char **args, void *extra), char **args) {
	struct event *ev;
	ev = event_create(name, action, args);
	if (ev) {
		ev->next = event_list;
		event_list = ev;
	}
}

int event_fire(const char *name, void *extra) {
	struct event *ev;
	int result = 0;
	ev = event_list;

	while (ev) {
		if (strcmp(name, ev->event_name) == 0) {
			/* found the event, fire its action */
			result = (ev->action)(ev->args, extra);

			/* we don't break. this way, we can register more actions to one event */
		}

		ev = ev->next;
	}

	return result;
}

int event_cleanup() {
	struct event *ev = event_list, *evn;

	while (ev) {
		evn = ev;
		ev = ev->next;

		free(evn->event_name);
		string_array_free((char **)evn->args);
		free(evn);
	}
	oc_writelog("[event] disconnected all events.\n");

	return 0;
}
