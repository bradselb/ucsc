#!/bin/sh

# This is used to create device nodes when dynamic major number is requested. 
# Not needed when using static major numbers or sysfs interface (recommended) 
# where udev can automatically create a device file when this module is 
# loaded

module="poll_dev"
device="poll_dev"
mode="664"

if grep '^staff:' /etc/group > /dev/null; then
    group="staff"
else
    group="wheel"
fi

major=`cat /proc/devices | awk "\\$2==\"$module\" {print \\$1}"`

# Remove stale nodes and replace them, then give gid and perms

mknod /dev/${device} c $major 0
chgrp $group /dev/${device} 
chmod $mode  /dev/${device}
