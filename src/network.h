/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.1
 *
 * oculusd is the server part of oculus
 * network.h
 */

#ifndef __NETWORK_H
#define __NETWORK_H

#include <arpa/inet.h>


/* start the daemon, handles configfile scanning, opening the socket etc */
int oc_start_daemon(void);

/* the main daemon loop, does the low level connection handling
		(e.g. IP checks, connects, etc */
void main_loop(void);

/* check if an IP is allowed to connect
   0 if not, 1 otherwise */
int is_allowed(const struct in_addr *ip_address);

/* create a new connection, this creates a thread and starts it 
 * return 0 on success, or an error number (from pthread_create)
 */
struct connection {
	int socket;
	struct sockaddr_in peer_address;
	pthread_t thread;
	struct connection *next;
	struct connection *prev;
	time_t logon;
};

/* handle an onmp connection, this handles an entire onmp session */
int handle_connection(struct connection *conn);

struct connection *register_connection(int fd, struct sockaddr_in peer_addr);
int create_connection(struct connection *c_info);
pthread_t *fetch_thread_ptr();
void connection_thread(void *args);
void free_connection(struct connection *conn);
void print_threads();

#endif