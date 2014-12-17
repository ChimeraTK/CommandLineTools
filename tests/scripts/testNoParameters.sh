#!/bin/bash

./mtca4u &> output.txt # Both stdout and stderr
                       # are piped to file for now. Reason being the output of
                       # this command right now prints to both streams. TODO:
                       # Revisit this piping once code is revised.
if [ $? -ne 1 ] ; then
    exit -1
fi 

#drop out if an error occurs
set -e

diff output.txt referenceNoParameters.txt
