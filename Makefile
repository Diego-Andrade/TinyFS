CC = gcc
FLAGS = -Wall -g
PROG = tinyFSDemo
OBJS = tinyFSDemo.o libTinyFS.o libDisk.o linkedList.o fileTableList.o

$(PROG): $(OBJS) 
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

tinyFSDemo.o: tinyFSDemo.c libTinyFS.o 
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c 
	$(CC) $(CFLAGS) -c -o $@ $<

linkedList.o: linkedList.c
	$(CC) $(CFLAGS) -c -o $@ $<

fileTableList.o: fileTableList.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(PROG) 