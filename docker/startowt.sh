#!/bin/bash

# change workdir
cd /home/owt

mongourl=localhost/owtdb
LOG=/dev/null

service mongodb start &
service rabbitmq-server start &

while ! mongo --quiet --eval "db.adminCommand('listDatabases')" >>$LOG 2>&1
do
    echo mongo is not ready. Waiting
    sleep 1
done

while ! rabbitmqctl -q status >> $LOG 2>&1
do
    echo RabbitMQ is not ready. Waiting...
    sleep 1
done

# format the parameters
set -- $(getopt -u -l rabbit:,mongo:,hostname:,externalip:,network_interface:: -- -- "$@")
# get the parameters
while [ -n "$1" ]
do
    case "$1" in
        --rabbit ) rabbitmqip=$2; shift; shift ;;
        --mongo ) mongourl=$2; shift; shift ;;
        --hostname ) hostname=$2; shift; shift ;;
        --externalip ) externalip=$2; shift; shift ;;
        --network_interface ) networkinterface=$2; shift; shift ;;
        * ) break;;
    esac
done

alltoml=$(find . -maxdepth 2 -name "*.toml")
echo ${mongourl}
echo ${rabbitmqip}
for toml in $alltoml; do
  if [ ! -z "${mongourl}" ];then
    sed -i "/^dataBaseURL = /c \dataBaseURL = \"${mongourl}\"" $toml  
  fi

  if [ ! -z "${rabbitmqip}" ];then
    if [[ $toml == *"management_console"* ]]; then
     echo "Do not modify management_console"
    else
      sed -i "/^host = /c \host = \"${rabbitmqip}\"" $toml
    fi
 
  fi
done

if [ ! -z "${hostname}" ];then
    echo ${hostname}
    sed -i "/^hostname = /c \hostname = \"${hostname}\"" portal/portal.toml  
fi

if [ ! -z "${externalip}" ] && [ ! -z "{network_interface}" ];then
    echo ${externalip}
    sed -i "/^network_interfaces =/c \network_interfaces = [{name = \"${networkinterface}\", replaced_ip_address = \"${externalip}\"}]" webrtc_agent/agent.toml
    sed -i "/^ip_address = /c \ip_address =  \"${externalip}\"" portal/portal.toml  
fi

./management_api/init.sh --dburl=${mongourl}

./video_agent/install_openh264.sh

./bin/start-all.sh

sleep infinity
