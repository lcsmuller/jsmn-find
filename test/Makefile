TOP = ..
CC ?= gcc

PARSING_DIR   := test_parsing
TRANSFORM_DIR := test_transform
OBJDIR        := $(TOP)/obj

TEST_PARSER := parser test

EXES := $(TEST_PARSER)

CFLAGS += -std=c99 -g -I. -I$(TOP)

all: $(EXES)

$(EXES): %: %.c
	$(CC) $(CFLAGS) $(XFLAGS) -o $@ $^

test_parse: $(EXES) $(PARSING_DIR)
	- ./$(TEST_PARSER) -e -s json_parsing -- $(PARSING_DIR)/*

test_transform: $(EXES) $(TRANSFORM_DIR)
	- ./$(TEST_PARSER) -e -s json_transform -- $(TRANSFORM_DIR)/*

clean:
	rm -rf $(EXES) *.gcno *.gcda

.PHONY : all clean
