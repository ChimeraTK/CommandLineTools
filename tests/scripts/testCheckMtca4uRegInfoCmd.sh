#!/bin/bash

./mtca4u "register_info" "DUMMY1" "WORD_FIRMWARE" > output_mtca4uRegInfo.txt 2>&1  
if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    exit -1
fi 
./mtca4u "register_info" "DUMMY1" "AREA_DMAABLE_FIXEDPOINT16_3" >> output_mtca4uRegInfo.txt 2>&1  
if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    exit -1
fi 
./mtca4u "register_info" "DUMMY1"  >> output_mtca4uRegInfo.txt 2>&1
# This command is not successful; Error message gets printed to stderr, and
# consequently return type is not 0
if [ $? -ne 1 ] ; then 
    exit -1
fi 

diff output_mtca4uRegInfo.txt referenceMtca4uRegInfo.txt

