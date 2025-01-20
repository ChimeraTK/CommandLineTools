#!/bin/bash

# NOTE: Paths specified below, assume the working directory is the build
# directory
mtca4u_executable=./mtca4u
actual_console_output="./output_without_version.txt"
expected_console_output="./referenceTexts/referenceNoParameters.txt"

mtca4u_command="$mtca4u_executable"

$mtca4u_command &> output.txt # Both stdout and stderr
                       # are piped to file for now. Reason being the output of
                       # this command right now prints to both streams. TODO:
                       # Revisit this piping once code is revised.
if [ $? -ne 1 ] ; then
    exit -1
fi

# we cannot pipe directly onto seed because otherwise the $? would give
# the result of sed, not of mtca4u.
cat output.txt | sed "{s/version [0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]/version \
IRRELEVANT/}" > $actual_console_output

#drop out if an error occurs
set -e

grep -v "gcda:Merge mismatch" $actual_console_output > ${actual_console_output}-filtered
diff ${actual_console_output}-filtered $expected_console_output
