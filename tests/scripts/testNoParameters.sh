#!/bin/bash -e

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_without_version.txt"
expected_console_output="./referenceTexts/referenceNoParameters.txt"

# test without parameters ("!" is used to invert the return code)
! $mtca4u_executable &> $actual_console_output

# we cannot pipe directly into sed because otherwise the $? would give the result of sed, not of mtca4u.
sed "{s/version [0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]/version IRRELEVANT/}" -i $actual_console_output

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
