
#!/bin/bash

./mtca4u "device_info" "DUMMY1" > output_mtca4uDeviceInfo.txt  2>&1 # redirect
# all output to file 

if [ $? -ne 0 ] ; then # 0 is the exit status for a successful command
    exit -1
fi 

diff output_mtca4uDeviceInfo.txt referenceMtca4uDeviceInfo.txt

