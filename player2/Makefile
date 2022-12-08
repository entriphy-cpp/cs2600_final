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

all: ticTacToe
ticTacToe:
	$(CC) $(CFLAGS) src/*.c $(LFLAGS) -o bin/tictactoe
clean:
	$(DELETE) $(DELPATH)