CFLAGS=-Wall -Wextra -Wshadow -pedantic -Wuninitialized -std=c++0x
CXXFLAGS=-Wall -Wextra -Wshadow -pedantic -Wuninitialized -std=c++0x
LDFLAGS=-w -std=c++0x

#Adapt the path to point to your mtca4u configuration.
#For local/private installations it might be something like
#include $(HOME)/mtca4u/00.14.00/MTCA4U.CONFIG
#
#/usr/share/mtca4u/ is the path for installations from Ubuntu packages
include /usr/share/mtca4u/MTCA4U.CONFIG

DIR = $(CURDIR)

all:
	gcc $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -I./include $(MtcaMappedDevice_INCLUDE_FLAGS) $(MtcaMappedDevice_LIB_FLAGS) $(MtcaMappedDevice_RUNPATH_FLAGS) -o ./bin/mtca4u ./src/mtca4u_cmd.cpp -lstdc++

.PHONY: clean install install_local

clean:	
	rm -rf ./bin/*

install:
	cp ./bin/mtca4u ~/bin 

install_local:
	cp ./bin/mtca4u /usr/bin/mtca4u
