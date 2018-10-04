#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uDeviceInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uDeviceInfo.txt"


( flock 9 # lock for mtcadummys0

  # print out device info for a card without modules
  echo "Device Info for card without modules" > $actual_console_output  2>&1
  $mtca4u_executable device_info DUMMY1 > $actual_console_output  2>&1 # redirect
                                                 # both stderr and stdout to file 
  if [ $? -ne 0 ] ; then # 0 is the exit 
                         # status for a successful command
      exit -1
  fi 

) 9>/var/run/lock/mtcadummy/mtcadummys0


( flock 9 # lock for mtcadummys1

  # print out device info for a card with modules
  echo "Device Info for card with modules" >> $actual_console_output  2>&1
  $mtca4u_executable device_info DUMMY2 >> $actual_console_output  2>&1

) 9>/var/run/lock/mtcadummy/mtcadummys1


# Bad command structure
echo "Device Info command called with incorrect number of parameters" >> $actual_console_output  2>&1
$mtca4u_executable device_info >> $actual_console_output  2>&1 
if [ $? -ne 1 ] ; then 
    exit -1
fi 



diff $actual_console_output $expected_console_output

