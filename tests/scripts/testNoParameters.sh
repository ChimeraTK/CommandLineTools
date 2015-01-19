#!/bin/bash

./mtca4u &> output.txt # Both stdout and stderr
                       # are piped to file for now. Reason being the output of
                       # this command right now prints to both streams. TODO:
                       # Revisit this piping once code is revised.
if [ $? -ne 1 ] ; then
    exit -1
fi 

# we cannot pipe directly onto seed because otherwise the $? would give
# the result of sed, not of mtca4u.
cat output.txt | sed "{s/version [0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]/version IRRELEVANT/}" > output_without_version.txt

#drop out if an error occurs
set -e

diff output_without_version.txt referenceNoParameters.txt
