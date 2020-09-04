#!/bin/sh

printf "Installing pwswd++...\n"

killall pwswdpp &> /dev/null

cp S92pwswdpp.sh /usr/local/etc/init.d
cp pwswdpp /usr/local/sbin

chmod +x /usr/local/etc/init.d/S92pwswdpp.sh /usr/local/sbin/pwswdpp

printf "Done! Rebooting...\n"
sleep 2
reboot