#!/bin/bash

# Steps for testing the SNMP counters (on Ubuntu):
#
#        ## On the SNMP server (i.e., the Gateway server) to start the SNMP service
#
#        # Install the snmpd service on Ubuntu
#        sudo apt-get install snmpd
#        # Install the snmpd service on CentOS
#        sudo yum install net-snmp
#
#        # Install and download SNMP MIBs on Ubuntu
#        sudo apt-get install snmp-mibs-downloader
#        # On CentOS this step is not needed
#
#        # Configure the snmpd service
#        sudo vim /etc/snmp/snmpd.conf
#         # Enable full access from the ip address that we want to allow SNMP lookups from
#         # for example 1.2.3.4
#         rocommunity public  1.2.3.4
#         # Extend SNMP with the Gateway counters retrieval script.
#         # Note that we need to modify the path if the script is in another place.
#         extend gateway-packetsReceived /bin/bash /opt/webrtc-gateway/bin/retrieveGatewayCounters.sh packetsReceived
#         extend gateway-packetsLost /bin/bash /opt/webrtc-gateway/bin/retrieveGatewayCounters.sh packetsLost
#         extend gateway-averageRTT /bin/bash /opt/webrtc-gateway/bin/retrieveGatewayCounters.sh averageRTT
#         extend gateway-connections /bin/bash /opt/webrtc-gateway/bin/retrieveGatewayCounters.sh connections
#
#        # Restart the snmpd service
#        sudo service snmpd restart
#
#
#        ## On the client machine (e.g., Ubuntu) to query the SNMP counters
#
#        # Install snmp client tools
#        sudo apt-get install snmp
#
#        # Configure the snmp client tools
#        sudo vim /etc/snmp/snmp.conf
#        # Enable loading of the MIBs by commenting out the following line:
#         # mibs:
#
#        # Use snmpwalk to test the Gateway counters  (assuming the SNMP server ip is 5.6.7.8)
#        snmpwalk -v1 -c public 5.6.7.8 NET-SNMP-EXTEND-MIB::nsExtendObjects
#
#        # You should see the output like below;
#        # currently the output is a JSON formatted string
#        yxian@yxian-desk3:/opt/webrtc-gateway/scripts$ snmpwalk -v1 -c public 5.6.7.8 NET-SNMP-EXTEND-MIB::nsExtendObjects
#        NET-SNMP-EXTEND-MIB::nsExtendNumEntries.0 = INTEGER: 1
#        NET-SNMP-EXTEND-MIB::nsExtendCommand."gateway" = STRING: /bin/sh
#        NET-SNMP-EXTEND-MIB::nsExtendArgs."gateway" = STRING: /opt/webrtc-gateway/scripts/retrieveGatewayCounters.sh
#        NET-SNMP-EXTEND-MIB::nsExtendInput."gateway" = STRING:
#        NET-SNMP-EXTEND-MIB::nsExtendCacheTime."gateway" = INTEGER: 5
#        NET-SNMP-EXTEND-MIB::nsExtendExecType."gateway" = INTEGER: exec(1)
#        NET-SNMP-EXTEND-MIB::nsExtendRunType."gateway" = INTEGER: run-on-read(1)
#        NET-SNMP-EXTEND-MIB::nsExtendStorage."gateway" = INTEGER: permanent(4)
#        NET-SNMP-EXTEND-MIB::nsExtendStatus."gateway" = INTEGER: active(1)
#        NET-SNMP-EXTEND-MIB::nsExtendOutput1Line."gateway" = STRING: {"packetsReceived":3164,"packetsLost":3,"averageRTT":7,"connections":2}
#        NET-SNMP-EXTEND-MIB::nsExtendOutputFull."gateway" = STRING: {"packetsReceived":3164,"packetsLost":3,"averageRTT":7,"connections":2}
#        NET-SNMP-EXTEND-MIB::nsExtendOutNumLines."gateway" = INTEGER: 1
#        NET-SNMP-EXTEND-MIB::nsExtendResult."gateway" = INTEGER: 0
#        NET-SNMP-EXTEND-MIB::nsExtendOutLine."gateway".1 = STRING: {"packetsReceived":3164,"packetsLost":3,"averageRTT":7,"connections":2}

usage() {
  echo
  echo "WebRTC ooVoo Gateway counters retrieval script"
  echo "Usage:"
  echo "  retrieveGatewayCounters.sh counter"
  echo "    get the value of the specified counter"
  echo
  echo "    Valid counters are: "
  echo
  echo "    packetsReceived                     accumulated number of received packets"
  echo "    packetsLost                         accumulated number of lost packets"
  echo "    averageRTT                          average value of RTT (round-trip-time)"
  echo "    connections                         accumulated number of connections"
  echo "    chrome_windows                      accumulated number of connections from Chrome on Windows"
  echo "    chrome_linux                        accumulated number of connections from Chrome on Linux"
  echo "    chrome_mac os x                     accumulated number of connections from Chrome on Mac OS X"
  echo "    chrome_android                      accumulated number of connections from Chrome on Android"
  echo "    chrome_others                       accumulated number of connections from Chrome on other OSes"
  echo "    firefox_windows                     accumulated number of connections from Firefox on Windows"
  echo "    firefox_linux                       accumulated number of connections from Firefox on Linux"
  echo "    firefox_mac os x                    accumulated number of connections from Firefox on Mac OS X"
  echo "    firefox_android                     accumulated number of connections from Firefox on Android"
  echo "    firefox_others                      accumulated number of connections from Firefox on other OSes"
  echo "    opera_windows                       accumulated number of connections from Opera on Windows"
  echo "    opera_linux                         accumulated number of connections from Opera on Linux"
  echo "    opera_mac os x                      accumulated number of connections from Opera on Mac OS X"
  echo "    opera_android                       accumulated number of connections from Opera on Android"
  echo "    opera_others                        accumulated number of connections from Opera on other OSes"
  echo "    others_windows                      accumulated number of connections from other browsers on Windows"
  echo "    others_linux                        accumulated number of connections from other browsers on Linux"
  echo "    others_mac os x                     accumulated number of connections from other browsers on Mac OS X"
  echo "    others_android                      accumulated number of connections from other browsers on Android"
  echo "    othres_others                       accumulated number of connections from other browsers on other OSes"
  echo
}

if [[ $# -eq 0 ]];then
  usage
  exit 0
fi

COUNTER_FILE=/tmp/webrtc_gateway_counters.json

shopt -s extglob
if [[ $# -gt 0 ]]; then
  case $1 in
    packetsReceived|packetsLost|averageRTT|connections|chrome_windows|chrome_linux|chrome_mac\ os\ x|chrome_android|chrome_others|firefox_windows|firefox_linux|firefox_mac\ os\ x|firefox_android|firefox_others|opera_windows|opera_linux|opera_mac\ os\ x|opera_android|opera_others|others_windows|others_linux|others_mac\ os\ x|others_android|others_others )
      if [[ -s $COUNTER_FILE ]]; then
        value=`grep "$1" $COUNTER_FILE | awk '{print $2}'`
        if [[ -z $value ]]; then
          value=0
        fi
        echo $value
        exit $value
      else
        echo -e "\x1b[33mCounter file doesn't exist\x1b[0m: $1"
      fi
      ;;
    *(-)help )
      usage
      ;;
    * )
      echo -e "\x1b[33mUnknown argument\x1b[0m: $1"
      ;;
  esac
fi

exit 0
