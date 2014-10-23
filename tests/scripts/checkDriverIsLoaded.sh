#!/bin/bash

if [ ! -e /dev/mtcadummys0 ] ; then
    echo Could not find device file. Make sure the mtcadummy driver is loaded.
    exit -1
fi
