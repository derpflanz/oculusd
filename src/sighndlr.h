/*
 * Oculus Network Monitor (C) 2003 Bart Friederichs
 * oculusd v0.1
 *
 * oculusd is the server part of oculus
 * oc_signal.h
 */

int initialise_signals();
void sigchild_handler(int s);
void sigterm_handler(int s);
void sighup_handler(int s);
