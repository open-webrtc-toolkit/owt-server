#!/bin/bash -e

service mongodb start &
service rabbitmq-server start &
while ! mongo --quiet --eval "db.adminCommand('listDatabases')"
do
    sleep 1
done


cd /home/owt
sed -i '/analytics-agent/d' ./bin/start-all.sh
./management_api/init.sh && ./bin/start-all.sh 
