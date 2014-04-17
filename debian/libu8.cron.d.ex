#
# Regular cron jobs for the libu8 package
#
0 4	* * *	root	[ -x /usr/bin/libu8_maintenance ] && /usr/bin/libu8_maintenance
