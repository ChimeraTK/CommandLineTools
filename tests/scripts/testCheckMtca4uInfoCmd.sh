#!/bin/bash -e

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_mtca4uInfo.txt"
expected_console_output="./referenceTexts/referenceMtca4uInfo.txt"

mtca4u_command="$mtca4u_executable info"

$mtca4u_command &> $actual_console_output

sed -r "{s|\t/.*/|\t./|}" -i $actual_console_output
grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
