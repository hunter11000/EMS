CC = gcc
MAINC = ems_tcpserver.c
EXEC = ems_tcpserver

CFLAGS = -pthread -lsqlite3
main:
	$(CC) $(MAINC) -o $(EXEC) $(CFLAGS) 
clean:
	rm $(EXEC) -rf