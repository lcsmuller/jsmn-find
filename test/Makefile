TOP = ..
CC ?= gcc

JSMN_FIND = $(TOP)/jsmn-find.h

PARSING_DIR   = test_parsing
TRANSFORM_DIR = test_transform

EXE_PARSER    = parser
EXE_FUNCTIONS = functions

EXES = $(EXE_PARSER) $(EXE_FUNCTIONS)

CFLAGS += -Wall -Wextra -Wpedantic -std=c89 -g -I. -I$(TOP)

all: $(EXES)

$(EXES): %: %.c $(JSMN_FIND)
	$(CC) $(CFLAGS) $(XFLAGS) -o $@ $<

$(JSMN_FIND): ;

test: test_parse test_transform test_functions

test_functions: $(EXE_FUNCTIONS)
	- ./$< -e
test_parse: $(EXE_PARSER) $(PARSING_DIR)
	- ./$< -e -s json_parsing -- $(PARSING_DIR)/*
test_transform: $(EXE_PARSER) $(TRANSFORM_DIR)
	- ./$< -e -s json_transform -- $(TRANSFORM_DIR)/*

clean:
	rm -rf $(EXES) *.gcno *.gcda

.PHONY : all clean
