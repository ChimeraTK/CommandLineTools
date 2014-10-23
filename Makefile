CFLAGS=-Wall -Wextra -Wshadow -pedantic -Wuninitialized -std=c++0x
CXXFLAGS=-Wall -Wextra -Wshadow -pedantic -Wuninitialized -std=c++0x
LDFLAGS=-w -std=c++0x

MTCA_INCLUDE_FLAG = -I/usr/include/mtca4u/
MTCA_LIB_FLAG = -L/usr/lib/mtca4u/

DIR = $(CURDIR)

all:
	gcc $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -I./include $(MTCA_INCLUDE_FLAG) $(MTCA_LIB_FLAG) -o ./bin/mtca4u ./src/mtca4u_cmd.cpp -lstdc++ -lMtcaMappedDevice

.PHONY: clean install install_local

clean:	
	rm -rf ./bin/*

install:
	cp ./bin/mtca4u ~/bin 

install_local:
	cp ./bin/mtca4u /usr/bin/mtca4u
