set -x
#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */

TEST_FOLDER=`dirname "$0"`
TEST=`cd "$TESTFOLDER"; pwd`
mode=$1
if [ $# == 2 ];then
    DIST=$2
else
    DIST="$TEST/../dist"
fi
echo $DIST
### Hardware or softwre ###
if [ "$mode" == "hardware" ];then
$DIST/bin/init.sh --hardware> tempInitResult.txt
else
$DIST/bin/init.sh > tempInitResult.txt
fi
### STOP PREVIOUS MCU ##
$DIST/bin/stop-all.sh
### start MCU ##
cp -rf ./init_superservice.sh $DIST/bin/
export DISPLAY=:0.0
$DIST/bin/start-all.sh
$DIST/bin/init_superservice.sh
############## check if start up successfully ################
# we use these way to check if the package/build is all right.
#SAMPLE_PID=`ps aux | grep basicServerAPITest.js | grep -v "grep" | awk '{print $2}'`
WORKER_PID=`ps aux | grep erizo | grep -v "grep" | awk '{print $2}'`
MANAGER_PID=`ps aux | grep nuve | grep -v "grep" | awk '{print $2}'`
if [ -z "${WORKER_PID}" -o -z "${MANAGER_PID}" ];then
  echo "There are some errors in the packages!!!"
  exit 1
fi
### run Test cases ###
/usr/lib/node_modules/jasmine-node-karma/bin/jasmine-node-karma --junitreport nuve-api-spec.js >nuve_test.txt
npm test >testlog$mode.txt

### Clear env ###
$DIST/bin/stop-all.sh
