#!/bin/bash
rm -rf purehtml
mkdir purehtml

cp confSverHtml/index.html purehtml/conf.html
cp sipHtml/index.html purehtml/sip.html

mkdir purehtml/pic
cp ServerPic/* purehtml/pic
cp confSverHtml/intel_logo.png purehtml/
cp confSverHtml/customdoxygen.css purehtml/

