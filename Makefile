# Makefile for Lab 4: Client-Server Using Fork
# Run "make" to build everything, or build one target at a time.
# Run "make clean" to remove all compiled binaries.

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra
CXXFLAGS = -Wall -Wextra

all: c/server c/server_basic c/client cpp/server_cpp cpp/client_cpp dos_sim/dos_sim_local

c/server: c/server.c
	$(CC) $(CFLAGS) c/server.c -o c/server

c/server_basic: c/server_basic.c
	$(CC) $(CFLAGS) c/server_basic.c -o c/server_basic

c/client: c/client.c
	$(CC) $(CFLAGS) c/client.c -o c/client

cpp/server_cpp: cpp/server.cpp
	$(CXX) $(CXXFLAGS) cpp/server.cpp -o cpp/server_cpp

cpp/client_cpp: cpp/client.cpp
	$(CXX) $(CXXFLAGS) cpp/client.cpp -o cpp/client_cpp

dos_sim/dos_sim_local: dos_sim/dos_sim_local.c
	$(CC) $(CFLAGS) dos_sim/dos_sim_local.c -o dos_sim/dos_sim_local

clean:
	rm -f c/server c/server_basic c/client cpp/server_cpp cpp/client_cpp dos_sim/dos_sim_local

.PHONY: all clean
