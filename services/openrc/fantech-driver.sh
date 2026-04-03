#!/sbin/openrc-run

description="Fantech X9 Thor Mouse Driver Service"
command="/usr/local/bin/fantech-driver"
command_args="daemon --auto /home/$USER/.config/fantech-x9-thor/config.conf"
command_user="$USER"
command_background="yes"
pidfile="/run/fantech-driver-$USER.pid"
output_log="/var/log/fantech-driver-$USER.log"
error_log="/var/log/fantech-driver-$USER.err"

depend() {
    need dbus
    after xdm
}

start_pre() {
    checkpath --directory --mode=0755 /var/log
    checkpath --file --path=$pidfile
}

stop_pre() {
    if [ -f "$pidfile" ]; then
        kill $(cat $pidfile) 2>/dev/null
        rm -f $pidfile
    fi
}
