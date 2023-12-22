CC = g++
CFLAGS = -Wall -Wextra

TARGETS = task1 task2 function1.so function2.so
DIR = plugins
DIR_RUN = run
all: $(TARGETS)

task1 : task1.cpp
	$(CC) $(CFLAGS) -o $(DIR_RUN)/$@ $^ -ldl

task2 : task2.cpp
	$(CC) $(CFLAGS) -o $(DIR_RUN)/$@ $^ -ldl

function2.so : function2.o
	$(CC) $(CFLAGS) -shared -o $(DIR)/$@ $^
	make clean

function1.so : function1.o
	$(CC) $(CFLAGS) -shared -o $(DIR)/$@ $^

function2.o : function2.cpp ./function2.h
	$(CC) $(CFLAGS) -fpic -c $< -o $@

function1.o : function1.cpp ./function1.h
	$(CC) $(CFLAGS) -fpic -c $< -o $@

clean : 
	rm -f *.o