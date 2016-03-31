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

check_node_version()
{
  if ! hash node 2>/dev/null; then
    echo >&2 "Error: node not found. Please install node ${NODE_VERSION} first."
    return 1
  fi
  local NODE_VERSION=v$(node -e "process.stdout.write(require('${ROOT}/package.json').engine.node)")
  NODE_VERSION=$(echo ${NODE_VERSION} | cut -d '.' -f 1)
  local NODE_VERSION_USE=$(node --version | cut -d '.' -f 1)
  [[ ${NODE_VERSION} == ${NODE_VERSION_USE} ]] && return 0 || (echo "node version not match. Please use node ${NODE_VERSION}"; return 1;)
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
    check_node_version || exit 1
    rotate_log $stdout
    echo "starting $command, stdout -> $stdout"
    case ${command} in
      nuve )
        if ! pgrep -f rabbitmq >/dev/null; then
          sudo echo
          sudo rabbitmq-server > ${LogDir}/rabbit.log &
        fi
        cd ${WOOGEEN_HOME}/nuve
        nohup nice -n ${WOOGEEN_NICENESS} node nuve.js \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        sleep 5
        ;;
      cluster-manager )
        cd ${WOOGEEN_HOME}/cluster_manager
        nohup nice -n ${WOOGEEN_NICENESS} node . \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      portal )
        cd ${WOOGEEN_HOME}/controller
        nohup nice -n ${WOOGEEN_NICENESS} node erizoController.js \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      webrtc-agent )
        cd ${WOOGEEN_HOME}/access_agent
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib
        export NODE_PATH=./node_modules
        nohup nice -n ${WOOGEEN_NICENESS} node . -U webrtc\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      rtsp-agent )
        cd ${WOOGEEN_HOME}/access_agent
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib
        export NODE_PATH=./node_modules
        nohup nice -n ${WOOGEEN_NICENESS} node . -U rtsp\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      recording-agent )
        cd ${WOOGEEN_HOME}/access_agent
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib
        export NODE_PATH=./node_modules
        nohup nice -n ${WOOGEEN_NICENESS} node . -U file\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      audio-agent )
        cd ${WOOGEEN_HOME}/audio_agent
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib
        export NODE_PATH=./node_modules
        nohup nice -n ${WOOGEEN_NICENESS} node . -U audio\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      video-agent )
        cd ${WOOGEEN_HOME}/video_agent
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib
        export NODE_PATH=./node_modules
        nohup nice -n ${WOOGEEN_NICENESS} node . -U video\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
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
