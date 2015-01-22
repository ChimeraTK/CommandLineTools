#!/bin/bash

./mtca4u "info" > output_mtca4uInfo.txt 2>&1

if [ $? -ne 0 ] ; then 
    exit -1
fi 

diff output_mtca4uInfo.txt referenceMtca4uInfo.txt

