#!/bin/sh
#  Set permissions so there's no way to extract any useful information
# from this directory, but:
#    - sandserv executable can be run
#    - sandserv can still create directories in run/
#    - sandserv.sh can keep the server running
echo "Installing netrun on `date`" > sandserv.log
mkdir run
chown root:nobody run sandserv sandserv.log
chmod 110 sandserv
chmod 330 run
chmod 220 sandserv.log
