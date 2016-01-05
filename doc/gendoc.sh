#!/bin/bash

#GENERATE
rm -rf html
doxygen doxygen_server.conf

#MOVE RESOURCE
mkdir html/pic
cp ServerPic/* html/pic

