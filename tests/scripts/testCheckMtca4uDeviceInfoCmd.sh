#!/bin/bash -e

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uDeviceInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uDeviceInfo.txt"

{

  mkdir -p /var/run/lock/mtcadummy
  ( flock 9 # lock for mtcadummys0

    # print out device info for a card without modules
    echo "Device Info for card without modules"
    $mtca4u_executable device_info DUMMY1

  ) 9>/var/run/lock/mtcadummy/mtcadummys0


  ( flock 9 # lock for mtcadummys1

    # print out device info for a card with modules
    echo "Device Info for card with modules"
    $mtca4u_executable device_info DUMMY2

  ) 9>/var/run/lock/mtcadummy/mtcadummys1


  # Bad command structure ("!" is used to invert the return code)
  echo "Device Info command called with incorrect number of parameters"
  ! $mtca4u_executable device_info

} &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
