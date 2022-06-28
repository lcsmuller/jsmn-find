TOP = ..
CC ?= gcc

PARSING_DIR   = test_parsing
TRANSFORM_DIR = test_transform

EXE_PARSER    = parser
EXE_FUNCTIONS = functions
EXES          = $(EXE_PARSER) $(EXE_FUNCTIONS)

CFLAGS += -Wall -Wextra -Wpedantic -g -I$(TOP) -std=c89

all: $(EXES)

test: test_parse test_transform test_functions

test_functions: $(EXE_FUNCTIONS)
	./$< -e
test_parse: $(EXE_PARSER) $(PARSING_DIR)
	./$< -e -s json_parsing -- $(PARSING_DIR)/*
test_transform: $(EXE_PARSER) $(TRANSFORM_DIR)
	./$< -e -s json_transform -- $(TRANSFORM_DIR)/*

clean:
	rm -f $(EXES)

.PHONY : all clean
