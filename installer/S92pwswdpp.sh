#!/bin/sh

case "$1" in
start)
        echo "Stopping original pwswd"
        start-stop-daemon -K -q -p /var/run/pwswd.pid
        rm -f /var/run/pwswd.pid

        echo "Starting pwswd++"
        /usr/bin/env HOME=`cat /etc/passwd |head -1 |cut -d':' -f 6`    \
                /sbin/start-stop-daemon -S -b -m -p /var/run/pwswdpp.pid \
                -x /usr/local/sbin/pwswdpp
        ;;
stop)
        echo "Stopping pwswd++"
        start-stop-daemon -K -q -p /var/run/pwswdpp.pid
        rm -f /var/run/pwswdpp.pid
        ;;
status)
        RET=1
        if [ -r /var/run/pwswdpp.pid ] ; then
                kill -0 `cat /var/run/pwswdpp.pid` 2>&1 >/dev/null
                RET=$?
        fi
        if [ $RET -eq 0 ] ; then
                echo "pwswd++ is running"
        else
                echo "pwswd++ is NOT running"
        fi
        exit $RET
        ;;
*)
        echo "Usage: $0 {start|stop|status}"
        exit 1
esac

exit $?