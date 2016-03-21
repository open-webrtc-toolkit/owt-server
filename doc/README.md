Package Description
====================

1. gendoc.sh
--------------
Description: Run this script to generate doxygen online document.
It will output 2 documents
+ Package 'confSverHtml': open 'index.html' in that package to access the conference user guide index.
+ Package 'sipHtml': open 'index.html' in that package to access the sip gateway user guide index.
Example Usage: ./gendoc.sh

Note: before running this script, you should install doxygen first.
See instruction at https://www.stack.nl/~dimitri/doxygen/manual/install.html
For Ubuntu, you can directly run the following commands to install doxygen under version 1.8.6.
    sudo apt-get update
    sudo apt-get install doxygen
2. servermd
--------------
Description: This package contains markdown files for conference user guide and sip gateway user guide.

3. doxygen_cfrc.conf
--------------
The configure file to generate conference user guide document.

4. doxygen_sip.conf
--------------
The configure file to generate sip gateway user guide document.

5. ServerPic
--------------
The pictures needed in the server html pages.

6. doxygen
--------------
related doxygen resources.

7. mvhtml.sh
--------------
Generate pure html server guides in package 'purehtml'

