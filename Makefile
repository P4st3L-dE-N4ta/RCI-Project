#########################################################################
# Makefile
#
# Author: Ricardo Rodrigues
#
# Usage:
# -> make - compiles the program
# -> make clean - deletes executables and .o files
#
#########################################################################


CC = gcc
CFLAGS = -Wall -g

target = ndn
objects = main.o ndn_maintenance.o ndn_objects.o CommonFunc.o
source = main.c ndn_maintenance.c ndn_objectts.c CommonFunc.c

$(target): $(objects)
	$(CC) $(CFLAGS) $^ -o $(target)

$(objects): %.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

clean:
	rm -f *.o ndn 
