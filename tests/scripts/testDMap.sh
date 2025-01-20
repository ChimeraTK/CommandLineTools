#!/bin/bash -e


mtca4u_executable="${PWD}/mtca4u"
actual_console_output="${PWD}/output_testDMap.txt"
expected_console_output="${PWD}/referenceTexts/referenceTestDMap.txt"
TEST_BASE_DIR="${PWD}"

mkdir -p /var/run/lock/mtcadummy
( flock 9 # lock for mtcadummys0

  {
    for TESTDIR in testNoDmapFile testTwoDmapFilesBroken testTwoDmapFilesOk; do
        #test in a directory without dmap file
        cd "${TEST_BASE_DIR}/${TESTDIR}"
        echo "*** Tests in ${TESTDIR} ***"

        #using cdd works
        echo Testing CDD
        "${mtca4u_executable}" read "(pci:mtcadummys0?map=mtcadummy.map)" ADC WORD_CLK_DUMMY 0 0 hex
        #alias alias does not work ("!" is used to invert the return code)
        echo Testing alias
        ! "${mtca4u_executable}" read DUMMY1 "" WORD_CLK_DUMMY 0 0 hex
        #info does not work
        echo Testing info
        ! ${mtca4u_executable} info

        echo .
    done
  } &> "${actual_console_output}"

) 9>/var/run/lock/mtcadummy/mtcadummys0

grep -v "gcda:Merge mismatch" "$actual_console_output" > "${actual_console_output}-filtered"
diff "${actual_console_output}-filtered" "$expected_console_output"
