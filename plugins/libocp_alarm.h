#include "../src/plugindata.h"
#include "../src/network.h"
#include <time.h>

#define DEFAULT_ALARMFILE	"alarms"

struct reply *alarm_list(char **cmdline, const struct connection *conn);
struct reply *alarm_set(char **cmdline, const struct connection *conn);
struct reply *alarm_remove(char **cmdline, const struct connection *conn);

void save_alarms(const char *alarm_fn);
void load_alarms(const char *alarm_fn);

/* LL helpers */
struct alarm *alarm_create(char **cmdline, const struct connection *conn);
char *alarm_create_uid(char *uid, struct sockaddr_in peer);

struct alarm {
	char *uid;			// alarm_create_uid() generates this (hexcoded IP+AID given in ALST)
	time_t t_set;
	char *message;
	struct alarm *next;
};

void alarm_free(struct alarm *al);
struct alarm *alarm_fetch(char *uid);