CC = g++
CFLAGS = -Wall -Wextra
LDFLAG = -pthread

TARGETS = client server
OBJ0 = client.o
OBJ1 = server.o
DIR = plugins
all: $(TARGETS)

# countLine.o : countLine.cpp countLine.h
# 	$(CC) $(CFLAGS) -c $< -o $@

client : clientmain.cpp $(OBJ0)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAG)

server : servermain.cpp $(OBJ1)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAG)
	make clean

client.o : client.cpp ../include/client.h
	$(CC) $(CFLAGS) -c $< -o $@

server.o : server.cpp ../include/server.h
	$(CC) $(CFLAGS) -c $< -o $@

clean : 
	rm -f *.o