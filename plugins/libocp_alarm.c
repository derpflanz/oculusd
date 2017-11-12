/*
 * implements alarm handling
 */

#include "libocp_alarm.h"
#include "../src/liboculus.h"
#include <string.h>
#include "../src/event.h"

char plugin_name[] = "alarm";
char plugin_description[] = "Alarm handling, use the ALLS, ALST and ALRM to list, set or remove an alarm, respectively.";
char plugin_version[] = "1.0";

char *alarm_file = NULL;
struct alarm *alarms = NULL;

/* commands supported by this plugin */
struct cmd_table plugin_commands[] =  {
	{"ALLS", "alarm_list", "Shows all set alarms."},
	{"ALST", "alarm_set", "Set an alarm."},
	{"ALRM", "alarm_remove", "Remove an alarm."},
	{NULL, NULL, NULL}									/* needed for end-of-array checking */
};

struct evt_table plugin_events[] = {
	{"alarm_set" },
	{ NULL }
};

/* plugin initialising */
int plugin_initialise(void) {
	int length;
	char *homedir;

	homedir = getenv("HOME");
	alarms = NULL;

	/* +2 for \0 and a '/' */
	length = strlen(runtimedir) + strlen(DEFAULT_ALARMFILE) + 2;
	alarm_file = (char *)malloc(length * sizeof(char));
	snprintf(alarm_file, length, "%s/%s", runtimedir, DEFAULT_ALARMFILE);

	load_alarms(alarm_file);

	return 0;
}

/* called on plugin unload (by HUP or UNLD) */
int plugin_unload(void) {
	struct alarm *al = alarms, *alm;

	while (al) {
		alm = al;
		al = al->next;

		free(alm->message);
		free(alm);
	}

	oc_writelog("[alarm] alarm plugin unloaded.\n");
}

void save_alarms(const char *alarm_fn) {
	FILE *fp;
	fp = fopen(alarm_fn, "w");
	if (fp) {
		struct alarm *al = alarms;
		while (al) {
			fprintf(fp, "%ld %s %s\n", al->t_set, al->uid, al->message);

			al = al->next;
		}

		fclose(fp);
	} else {
		oc_writelog("[alarm] couldn't open %s for writing. Alarms are not saved between sessions.\n", alarm_fn);
	}
}

void load_alarms(const char *alarm_fn) {
	FILE *f;
	f = fopen(alarm_fn, "r");
	if (f) {
		char line[2048];
		struct alarm *al;
		int ptr = 0;

		while (fgets(line, 2048, f)) {
			char **fields = string_array_create(line);

			if (string_array_size(fields) == 3) {
				al = (struct alarm *)malloc(sizeof(struct alarm));
				al->message = strdup(fields[2]);
				al->uid = strdup(fields[1]);
				al->t_set = atol(fields[0]);
				al->next = alarms;
				alarms = al;

				string_array_free(fields);
			}
		}
		fclose(f);
	}

}

struct reply *alarm_list(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	struct alarm *a;

	rpl = malloc (sizeof(struct reply));
	rpl->code = OK;

	rpl->result = buffer_setup();

	a = alarms;
	while (a != NULL) {
		char *ts = create_timestring(a->t_set, "%Y-%m-%d %H:%M:%S");
		rpl->result = buffer_addf(rpl->result, "alarm %s, set at %s: %s\n", a->uid, ts, a->message);
		free(ts);
				
		a = a->next;
	}

	return rpl;
}

struct alarm *alarm_create(char **cmdline, const struct connection *conn) {
	struct alarm *ret = (struct alarm *)malloc(sizeof(struct alarm));

	ret->message = strdup(cmdline[2]);
	ret->uid = strdup(cmdline[1]);
	ret->t_set = time(NULL);
	ret->next = NULL;

	return ret;
}

void alarm_free(struct alarm *al) {
	if (al->uid) free(al->uid);
	if (al->message) free(al->message);
	free(al);
}

struct alarm *alarm_fetch(char *uid) {
	struct alarm *al = alarms;

	while (al != NULL && strcmp(al->uid, uid)) {
		al = al->next;
	}

	return al;
}

struct reply *alarm_set(char **cmdline, const struct connection *conn) {
	struct reply *rpl;
	int i = 0;

	rpl = malloc(sizeof(struct reply));
	rpl->result = buffer_setup();

	if (string_array_size(cmdline) < 3) {
		rpl->code = WRN_WRONGPARAM;
		rpl->result = buffer_addf(rpl->result, "You have to supply an alarm message: ALST <UID> \"<message>\".\n");
	} else {
		/* alarm message was ok, create */
		struct alarm *al = alarm_create(cmdline, conn), *al_c;

		al_c = alarm_fetch(al->uid);
		/* check if it is set */
		if (al_c) {
			rpl->code = ERR_FAILED;
			rpl->result = buffer_addf(rpl->result, "Alarm id %s already set.\n", al->uid);

			alarm_free(al);
		} else {
			/* alarm is legit, add to LL */
			al->next = alarms;
			alarms = al;

			rpl->code = OK;
			rpl->result = buffer_addf(rpl->result, "Alarm set; uid: %s, message: %s\n", al->uid, al->message);

			save_alarms(alarm_file);

			// and fire our 'alarm_set' event
			event_fire("alarm_set", al->message);
		}
	}

	return rpl;

}

struct reply *alarm_remove(char **cmdline, const struct connection *conn) {
	struct reply *rpl;

	rpl = malloc (sizeof(struct reply));
	rpl->result = buffer_setup();

	if (cmdline[1] == NULL) {
		rpl->code = WRN_WRONGPARAM;
		rpl->result = buffer_addf(rpl->result, "No alarm id given.\n");
	} else {
		char *uid = strdup(cmdline[1]);
		struct alarm *al = alarms;
		struct alarm *prev = NULL;
		char foundit = 0;

		while (al != NULL) {
			if (!strcmp(al->uid,  uid)) {
				if (prev == NULL) {
					alarms = al->next;
				} else {
					prev->next = al->next;
				}
				alarm_free(al);
				
				/* found it, we're done */
				foundit = 1;
				break;
			} else {
				prev = al;
				al = al->next;
			}
		}

		if (foundit == 0) {
			rpl->code = WRN_WRONGPARAM;
			rpl->result = buffer_addf(rpl->result, "Alarm '%s' is not a valid alarm id.\n", cmdline[1]);
		} else {
			rpl->code = OK;
			rpl->result = buffer_addf(rpl->result, "Alarm %s removed.\n", uid);

			save_alarms(alarm_file);
		}
		free(uid);
	}

	return rpl;
}
