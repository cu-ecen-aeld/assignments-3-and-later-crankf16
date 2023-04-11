#! /bin/sh

# Reference https://www.tutorialspoint.com/unix/case-esac-statement.htm#:~:text=esac%20statement%20is%20to%20give,default%20condition%20will%20be%20used.

case "$1" in
	start)
		start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
		;;

	stop)
		start-stop-daemon -K -n aesdsocket
		;;

	*)
		exit 1;
		;;

esac

exit 0
