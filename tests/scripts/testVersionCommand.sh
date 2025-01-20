#!/bin/bash -e

# NOTE: Paths specified below, assume the working directory is the build directory
mtca4u_executable=./mtca4u
actual_console_output="./output_versionCommand.txt"
expected_console_output="./referenceTexts/referenceVersionCommand.txt"

$mtca4u_executable version &> $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
