Oculusd is a simple server monitoring daemon for Linux

A little history
----------------
I started writing it as a exercise in C. I wanted to create a plugin system, networking tool, etc. And I was a little frustrated 
by how complex all those "simple" network and server monitoring tools were (SNMP, Nagios).

I started the project in 2003 and used / developed it on and off during that period. It was hosted on FreshMeat (now FreeCode and 
dead) for some time and I actually have built RPMs as well. When I revived my blog again in november 2017 (blog.friesoft.nl) I saw 
it again and wanting to do some C coding again, I thought why not work on it a little again.

I created the Github repository on november 12, 2017, with oculusd version 0.18.

How does it work?
-----------------
oculusd is the daemon that runs on a server you want to monitor. It is a TCP-server that by default listens on port 1976. When
you connect to oculusd on this port, you will be greeted SMTP-style and you can provide commands. A command is a four-letter
word (abbreviation) in all caps. When you run the command, it will reply with the result, or an error message. Any reply
will always have a result code (HTTP-style) and a human readable result. 

Commands are either built-in or come from a plugin. Built-in commands are only related to server administration and have no
monitoring capabilities. Monitoring commands are all placed in plugins. When oculusd starts, it will look for plugins in its 
plugin directory and load them if they are oculus plugins.

Configuration is done via XML files. You can control which commands are allowed and from where.

See the manual pages for more information.

Note that oculusd is still in development and not ready for production servers yet (if ever).
