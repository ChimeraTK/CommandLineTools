#!/bin/bash

./mtca4u 2> output.txt
if [ $? -ne 1 ] ; then
    exit -1
fi 

#drop out if an error occurs
set -e

diff output.txt referenceNoParameters.txt
