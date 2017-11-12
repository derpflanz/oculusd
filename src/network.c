/*
 * Oculus Network Monitor (C) Bart Friederichs
 *
 * oculusd is the server part of oculus
 * network.c - all networking code is in here
 */

#include "network.h"
#include "sighndlr.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "config.h"
#include "options.h"
#include "liboculus.h"
#include <pthread.h>
#include "command.h"
#include "plugindata.h"

int sockfd;
struct sockaddr_in my_addr;

int handle_connection(struct connection *conn)  {
	unsigned char connected = 1;
	int ret = 0, result = 0;
	
	send_reply(conn->socket, WELCOME);
	
	while (connected) {
		char buffer[1024];
		char **cmdline;
		ssize_t len;

		len = recv(conn->socket, buffer, 1023, 0);
		
		/* buffer is filled with crap, cut off */
		buffer[len] = '\0';
		
		if (len == 0) {
			/* client disconnected */
			connected = 0;
			ret = -1;
		} else {
			cmdline = string_array_create(buffer);		/* allocates memory for cmdline */
			if (cmdline) {
				result = handle_command(cmdline, conn);
				if (len >=2 && buffer[len-2] == '\r') buffer[len-2] = '\0';
				else if (len >= 1 && buffer[len-1] == '\n') buffer[len-1] = '\0';

				oc_writelog("host: %s, command: %s, result %d\n", inet_ntoa(conn->peer_address.sin_addr), buffer, result);
				if (result == QUIT) {
					connected = 0;
					ret = -1;
				}
				if (result == KILL_SERVER) {
					pid_t ppid;
					connected = 0;
					ret = -1;
					ppid = getpid();
					kill (ppid, SIGTERM);
				}
				if (result == RHSH_SERVER) {
					pid_t ppid;
					ppid = getpid();
					kill (ppid, SIGHUP);
				}
				string_array_free(cmdline);						/* frees entire string array */
			} else {
				oc_writelog("Couldn't parse commandline. Problem seems memory related.\n");
			}
		}
	}
	
	return ret;
}

int oc_start_daemon(void) {
	extern char *oc_host;
	extern int oc_port;
	struct sockaddr_in oldname;
	socklen_t len = sizeof(struct sockaddr);
	int so_reuseaddr = 1;
	
	/* setup network connection (the listener) */
	getsockname(sockfd, (struct sockaddr *)&oldname, &len);

	if (oc_port == ntohs(oldname.sin_port) && !strcmp(inet_ntoa(oldname.sin_addr), oc_host)) {
		/* If the new network settings are the same as the old, don't re-bind
		 */
		return 0;
	}
	
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
		perror("socket");
		return (-1);
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(oc_port);
	inet_aton(oc_host, &(my_addr.sin_addr));
	memset(&(my_addr.sin_zero), '\0', 8);
	
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return (-1);
	}
	
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		return (-1);
	}
	
	return(0);
}

int is_allowed(const struct in_addr *ip_address) {
	int is_allowed = 0, i = 0;
	extern struct in_addr *allowed_hosts;
	
	if (allowed_hosts != NULL) 
		while (allowed_hosts[i].s_addr) {
			if (allowed_hosts[i].s_addr == ip_address->s_addr) {
				is_allowed = 1;
				break;
			}
			i++;
		}
	
	return is_allowed;
}

void main_loop(void) {
	struct sockaddr_in his_addr;
	extern int sockfd;
	int new_fd;
	int sin_size;
	int conn_ok = 0;
	extern char quiet, caughthup;

	while (1) {
        struct connection *new_conn = NULL;
		sin_size = sizeof(struct sockaddr_in);
			
		/* accept() blocks the app until someone connects */
		if ((new_fd = accept(sockfd, (struct sockaddr *)&his_addr, &sin_size)) == -1) {
			if (errno == EAGAIN && caughthup == 1) {
				oc_writelog("Caught SIGHUP, reloading configfiles.\n");	
				fcntl(sockfd, F_SETFL, !O_NONBLOCK);
				caughthup = 0;
				oc_cleanup();
				oc_start();
				continue;
			} else {
				perror("accept");
				oc_cleanup();
				oc_exit(-1);
			}
		}
		
		setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
		/* see if this IP is allowed */
		if (!is_allowed(&his_addr.sin_addr)) {
			if (!quiet) oc_writelog("connect from %s: DENIED\n", inet_ntoa(his_addr.sin_addr));
			close(new_fd);
			continue;
		}


        new_conn = register_connection(new_fd, his_addr);
        if (new_conn != NULL) {
			if ((conn_ok = create_connection(new_conn)) != 0) {
				oc_writelog("Could not create thread to handle connection: %d\n", conn_ok);
				free_connection(new_conn);
			}
        } else {
            oc_writelog("Could not create connection struct!\n");
        }
	}
}

struct connection *register_connection(int fd, struct sockaddr_in peer_addr) {
    struct connection *new_connection = NULL;

	/* create new list element */
    new_connection = (struct connection *)malloc(sizeof(struct connection));
	if (new_connection != NULL) {
		new_connection->next = new_connection->prev = NULL;
		new_connection->socket = fd;
		new_connection->peer_address = peer_addr;
		new_connection->logon = time(NULL);

		new_connection->next = connection_list;
		if (new_connection->next != NULL) {
			/* there is an element in the list, make the backref */
			new_connection->next->prev = new_connection;
		}
		connection_list = new_connection;
	}

	return new_connection;
}

int create_connection(struct connection *conn) {
	int r = -1;
	
	/* first, try to create a new thread */
	if (conn != NULL) {
		r = pthread_create(&(conn->thread), NULL, connection_thread, conn);
	}
	return r;
}


void *connection_thread(void *args) {
	struct connection *c = (struct connection *)args;
	handle_connection(c);
	free_connection(c);

	return NULL;
}

void free_connection(struct connection *conn) {
	/* let's close the connection first */
	close(conn->socket);

	/* first, fix connection list, if we delete the first item */
	if (conn == connection_list) {
		connection_list = conn->next;
	}
	/* fix connection_list pointer, if entry was last in list */
	if (conn->next == NULL && conn->prev == NULL) {
		connection_list = NULL;
	}
	
	/* remove entry */
	if (conn->next != NULL) {
		conn->next->prev = conn->prev;		
	}
	if (conn->prev != NULL) {
		conn->prev->next = conn->next;
	}
	
	/* finally, delete the entry */
	free(conn);
}

