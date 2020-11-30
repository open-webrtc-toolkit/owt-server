#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

usage="Usage: daemon.sh (start|stop|status) (management-api|{purpose}-agent|app)"

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

command=$1 #management-api, conference-agent, ...
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

[[ -z ${OWT_HOME} ]] && OWT_HOME="${ROOT}/build"
[[ ! -d ${OWT_HOME} ]] && OWT_HOME="${ROOT}"
export OWT_HOME=${OWT_HOME}

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
LogDir="${OWT_HOME}/logs"
mkdir -p "$LogDir"

stdout=${LogDir}/owt-${command}.stdout
pid=${LogDir}/owt-${command}.pid

# Set default scheduling priority
if [ "$OWT_NICENESS" = "" ]; then
    export OWT_NICENESS=0
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
      management-api )
        cd ${OWT_HOME}/management_api
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Management-API \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      cluster-manager )
        cd ${OWT_HOME}/cluster_manager
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Cluster-Manager \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      portal )
        cd ${OWT_HOME}/portal
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Portal \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      conference-agent )
        cd ${OWT_HOME}/conference_agent
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Conference-Controller . -U conference\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      webrtc-agent )
        cd ${OWT_HOME}/webrtc_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U webrtc\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      streaming-agent )
        cd ${OWT_HOME}/streaming_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U streaming\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      recording-agent )
        cd ${OWT_HOME}/recording_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U recording\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      sip-agent )
        cd ${OWT_HOME}/sip_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U sip\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      sip-portal )
        cd ${OWT_HOME}/sip_portal
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-SIP-Portal \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      audio-agent )
        cd ${OWT_HOME}/audio_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U audio\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      video-agent )
        cd ${OWT_HOME}/video_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        export PATH=./bin:/opt/intel/mediasdk/bin:${PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U video\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      quic-agent )
        cd ${OWT_HOME}/quic_agent
        export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
        nohup nice -n ${OWT_NICENESS} ./OWT-MCU-Agent . -U quic\
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      management-console )
        cd ${OWT_HOME}/management_console
        nohup nice -n ${OWT_NICENESS} node . \
          > "${stdout}" 2>&1 </dev/null &
        echo $! > ${pid}
        ;;
      app )
        cd ${OWT_HOME}/apps/current_app/
        nohup nice -n ${OWT_NICENESS} node . \
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
