CC=gcc
CFLAGS=-Iinclude
LFLAGS=-lpaho-mqtt3c
ifeq ($(OS), Windows_NT) # Windows
	DELETE=del
	DELPATH=bin\\**
else
	DELETE=rm
	DELPATH=build/**
	ifeq ($(UNAME_S), Darwin) # MacOS 
		CFLAGS += -D OSX
	else # Linux
	endif
endif

all: ticTacToe daemon
ticTacToe:
	$(CC) $(CFLAGS) src/*.c $(LFLAGS) -o bin/tictactoe
daemon:
	$(CC) $(CFLAGS) src/daemon/*.c $(LFLAGS) -o bin/tictactoe-daemon
clean:
	$(DELETE) $(DELPATH)