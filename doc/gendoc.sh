#!/bin/bash
#GENERATE
rm -rf confSverHtml
rm -rf sipHtml

doxygen doxygen_cfrc.conf
doxygen doxygen_sip.conf

#MOVE RESOURCE
mkdir confSverHtml/pic
cp ServerPic/* confSverHtml/pic

#KEEP PURE SERVER DOC
./mvhtml.sh
