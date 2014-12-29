#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

usage="Usage: daemon.sh (start|stop|status) (nuve|mcu|app)"

# if no args specified, show usage
if [ $# -le 1 ]; then
  echo $usage
  exit 1
fi

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`
ROOT=`cd "${bin}/.."; pwd`

# get arguments
startStop=$1
shift

command=$1 #nuve, mcu, app
shift

umask 0000

rotate_log ()
{
  local logfile=$1;
  local num=5;
  if [ -n "$2" ]; then
  num=$2
  fi
  if [ -f "$logfile" ]; then # rotate logs
  while [ $num -gt 1 ]; do
    prev=`expr $num - 1`
    [ -f "$logfile.$prev" ] && mv "$logfile.$prev" "$logfile.$num"
    num=$prev
  done
  mv "$logfile" "$logfile.$num";
  fi
}

[[ -z ${WOOGEEN_HOME} ]] && WOOGEEN_HOME="${ROOT}/build"
[[ ! -d ${WOOGEEN_HOME} ]] && WOOGEEN_HOME="${ROOT}"
export WOOGEEN_HOME=${WOOGEEN_HOME}

forced=$1
FORCEDKILL=false
shopt -s extglob
if [ ! -z ${forced} ]; then
    case ${forced} in
        *(-)f )
            FORCEDKILL=true
            ;;
        *(-)force )
            FORCEDKILL=true
            ;;
    esac
fi

# create log directory
LogDir="${WOOGEEN_HOME}/logs"
mkdir -p "$LogDir"

stdout=${LogDir}/woogeen-${command}.stdout
pid=${LogDir}/woogeen-${command}.pid

# Set default scheduling priority
if [ "$WOOGEEN_NICENESS" = "" ]; then
    export WOOGEEN_NICENESS=0
fi

case $startStop in

  (start)
    if [ -f $pid ]; then
      if ps -p `cat $pid` > /dev/null 2>&1; then
        echo $command running as process `cat $pid`.
        exit 1
      fi
    fi

    rotate_log $stdout
    [[ $command != "mcu" ]] && echo "starting $command, stdout -> $stdout"
    case ${command} in
      nuve )
        if ! pgrep -f rabbitmq; then
          sudo echo
          sudo rabbitmq-server > ${LogDir}/rabbit.log &
        fi
        cd ${WOOGEEN_HOME}/nuve/nuveAPI
        nohup nice -n ${WOOGEEN_NICENESS} node nuve.js \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        sleep 5
        ;;
      controller )
        cd ${WOOGEEN_HOME}/mcu/erizoController
        nohup nice -n ${WOOGEEN_NICENESS} node erizoController.js \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      agent)
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${WOOGEEN_HOME}/lib
        export LOG4CXX_CONFIGURATION=${WOOGEEN_HOME}/etc/mcu/log4cxx.properties
        cd ${WOOGEEN_HOME}/mcu/erizoAgent
        nohup nice -n ${WOOGEEN_NICENESS} node erizoAgent.js \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      mcu )
        ${bin}/daemon.sh start controller;
        ${bin}/daemon.sh start agent;
        ;;
      app )
        cd ${WOOGEEN_HOME}/extras/basic_example/
        nohup nice -n ${WOOGEEN_NICENESS} node basicServer.js \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      * )
        echo $usage
        exit 1
        ;;
    esac

    sleep 1; [[ -f ${stdout} ]] && head "$stdout"
    ;;

  (stop)
    if [[ "$command" == "mcu" ]]; then
      ${bin}/daemon.sh stop controller;
      ${bin}/daemon.sh stop agent;
      exit 0;
    fi
    if [ -f $pid ]; then
      if ps -p `cat $pid` > /dev/null 2>&1; then
        if ! kill -0 `cat $pid` > /dev/null 2>&1; then
          echo cannot stop $command with pid `cat $pid` - permission denied
        elif ${FORCEDKILL}; then
          kill -9 `cat $pid` > /dev/null 2>&1;
          sleep 1;
          echo $command killed
        else
          echo -n stopping $command
          kill `cat $pid` > /dev/null 2>&1
          while ps -p `cat $pid` > /dev/null 2>&1; do
            echo -n "."
            sleep 1;
          done
          echo
        fi
        if ! ps -p `cat $pid` > /dev/null 2>&1; then
          rm $pid
        fi
      else
        echo no $command to stop
      fi
    else
      echo no $command to stop
    fi
    ;;

  (status)
    if [ -f $pid ]; then
      if ps -p `cat $pid` > /dev/null 2>&1; then
        echo $command running as process `cat $pid`.
        if [[ "$command" = "gateway" && -f $GW_COUNTER_OUTPUT ]]; then
          cat $GW_COUNTER_OUTPUT
        fi
      else
        echo $command not running.
      fi
    else
      echo $command not running.
    fi
    ;;

  (*)
    echo $usage
    exit 1
    ;;

esac
