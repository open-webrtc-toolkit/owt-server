#!/bin/bash
#
# webrtc-gateway        WebRTC Gateway to ooVoo server
#
# chkconfig: 2345 99 01
# description: This is a Gateway to connect the WebRTC clients to the ooVoo server
# processname: webrtc_gateway
#
# Put me as /etc/init.d/webrtc-gateway
#

# Source function library.
. /etc/init.d/functions

RETVAL=0
prog="webrtc-gateway"

WEBRTC_GATEWAY_DIR=/opt/webrtc-gateway
#LOCKFILE=/var/lock/subsys/$prog
LOCKFILE=$WEBRTC_GATEWAY_DIR/$prog.lock
version=`grep '"version":' ${WEBRTC_GATEWAY_DIR}/etc/heartbeat_config.json | awk '{print $2}'`
USERCONFIG=${WEBRTC_GATEWAY_DIR}/etc/linux_service_start_user
WEBRTC_USER=
[ -s ${USERCONFIG} ] && WEBRTC_USER=`cat ${USERCONFIG}`

checkUser() {
        if [[ $WEBRTC_USER ]]; then
          ! getent passwd $WEBRTC_USER > /dev/null 2>&1 && WEBRTC_USER=`whoami`
        else
          WEBRTC_USER=`whoami`
        fi
}

start() {
        checkUser
        echo "Starting $prog as $WEBRTC_USER: "
        apply_sysctl
        setcap 'cap_net_bind_service=+ep' $WEBRTC_GATEWAY_DIR/sbin/webrtc_gateway
        if [[ $WEBRTC_USER != `whoami` ]]; then
          daemon --user $WEBRTC_USER $WEBRTC_GATEWAY_DIR/bin/daemon.sh start gateway
        else
          $WEBRTC_GATEWAY_DIR/bin/daemon.sh start gateway
        fi
        touch $LOCKFILE
        return $RETVAL
}        

stop() {
        checkUser
        echo "Shutting down $prog as $WEBRTC_USER: "
        if [[ $WEBRTC_USER != `whoami` ]]; then
          daemon --user $WEBRTC_USER $WEBRTC_GATEWAY_DIR/bin/daemon.sh stop gateway
        else
          $WEBRTC_GATEWAY_DIR/bin/daemon.sh stop gateway
        fi
        rm -f $LOCKFILE
        return $RETVAL
}

status() {
        checkUser
        echo -n "Checking $prog status as $WEBRTC_USER: "
        echo "Version $version"
        if [[ $WEBRTC_USER != `whoami` ]]; then
          daemon --user $WEBRTC_USER $WEBRTC_GATEWAY_DIR/bin/daemon.sh status gateway
        else
          $WEBRTC_GATEWAY_DIR/bin/daemon.sh status gateway
        fi
        echo
        RETVAL=$?
        return $RETVAL
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    status)
        status
        ;;
    restart)
        stop
        start
        ;;
    reload)
        killall -HUP webrtc_gateway
        ;;
    condrestart)
        [ -f $LOCKFILE ] && (stop && start) || :
        ;;
    # probe)
        # <optional.  If it exists, then it should determine whether
        # or not the service needs to be restarted or reloaded (or
        # whatever) in order to activate any changes in the configuration
        # scripts.  It should print out a list of commands to give to
        # $0; see the description under the probe tag below.>
        # ;;
    *)
        echo "Usage: $prog {start|stop|status|reload|restart"
        exit 1
        ;;
esac
exit $?

