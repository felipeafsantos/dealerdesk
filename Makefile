# DealerDesk build
#
#   make          build ./dealerdesk  (equivalent to: gcc -o dealerdesk *.c -lm)
#   make test     build and run the unit tests (tests/test_calc.c) and the
#                 end-to-end screen tests (tests/e2e/run.sh)
#   make clean
#
# On Windows with the WinLibs/MinGW toolchain use mingw32-make.

# plain "=" because make's built-in default CC is "cc", which Windows lacks;
# "make CC=clang" on the command line still overrides it.
CC       = gcc
CFLAGS  ?= -Wall -Wextra -std=c99
LDLIBS   = -lm

ifeq ($(OS),Windows_NT)
EXE = .exe
else
EXE =
endif

SRC  = main.c calc.c dbfile.c screen.c
HDR  = globals.h calc.h dbfile.h screen.h
BIN  = dealerdesk$(EXE)
TEST = tests/test_calc$(EXE)

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

# The unit tests link only calc.c: pricing has no dependency on the rest.
$(TEST): tests/test_calc.c calc.c calc.h
	$(CC) $(CFLAGS) -I. -o $@ tests/test_calc.c calc.c $(LDLIBS)

test: $(TEST) $(BIN)
	./$(TEST)
	sh tests/e2e/run.sh ./$(BIN)

clean:
	rm -f dealerdesk dealerdesk.exe tests/test_calc tests/test_calc.exe

.PHONY: all test clean
