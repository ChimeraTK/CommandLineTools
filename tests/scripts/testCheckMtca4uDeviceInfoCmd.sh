
#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uDeviceInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uDeviceInfo.txt"

mtca4u_command="$mtca4u_executable device_info DUMMY1"

$mtca4u_command > $actual_console_output  2>&1 # redirect
                                               # both stderr and stdout to file 

if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    exit -1
fi 

diff $actual_console_output $expected_console_output

