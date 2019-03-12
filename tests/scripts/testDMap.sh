#!/bin/bash


mtca4u_executable=${PWD}/mtca4u
actual_console_output=${PWD}/output_testDMap.txt
expected_console_output=${PWD}/referenceTexts/referenceTestDMap.txt
TEST_BASE_DIR=${PWD}

mkdir -p /var/run/lock/mtcadummy
( flock 9 # lock for mtcadummys0

  rm ${actual_console_output}

  #for TESTDIR in testNoDmapFile testTwoDmapFilesBroken testTwoDmapFilesOk; do
  for TESTDIR in testNoDmapFile testTwoDmapFilesBroken; do
      #test in a directory without dmap file
      cd ${TEST_BASE_DIR}/${TESTDIR}
      echo "*** Tests in ${TESTDIR} ***" >> ${actual_console_output}
      
      #using cdd works
      echo Testing CDD >> ${actual_console_output}
      ${mtca4u_executable} read "(pci:mtcadummys0?map=mtcadummy.map)" ADC WORD_CLK_DUMMY 0 0 hex >> ${actual_console_output} 2>&1
      #using sdm (still) works
      echo Testing sdm >> ${actual_console_output}
      ${mtca4u_executable} read "sdm://./pci:mtcadummys0=mtcadummy.map" ADC WORD_CLK_DUMMY 0 0 hex >> ${actual_console_output} 2>&1
      #alias alias does not work
      echo Testing alias >> ${actual_console_output}
      ${mtca4u_executable} read DUMMY0 "" WORD_CLK_DUMMY 0 0 hex >> ${actual_console_output} 2>&1
      #info does not work
      echo Testing info >> ${actual_console_output}
      ${mtca4u_executable} info >> ${actual_console_output} 2>&1

      echo . >> ${actual_console_output}
  done

) 9>/var/run/lock/mtcadummy/mtcadummys0

cat $actual_console_output
diff $actual_console_output $expected_console_output
