# Design Name: Tester.py
# Project Name: Compiler Tester
# Description: Makefile for bison + flex compiler
# Authors:
#  - Leonardo Kauer Leffa - 00333399
#  - Luis Eduardo Pereira Mendes - 00333936

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address
LEX = flex
BISON = bison

# Source files
LEX_FILE = scanner.l
BISON_FILE = parser.y
# Updated C_SOURCES to include table.c and valor_t.c
C_SOURCES = main.c asd.c table.c valor_t.c type.c label.c code.c iloc.c
GENERATED_SOURCES = lex.yy.c parser.tab.c
GENERATED_HEADERS = parser.tab.h
SOURCES = $(C_SOURCES) $(GENERATED_SOURCES)
OBJ = $(SOURCES:.c=.o)

# Output binary
TARGET = etapa5

# Default target
all: $(TARGET) 

# Main build rule
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) -lfl

# Flex rule
lex.yy.c: $(LEX_FILE)
	$(LEX) -o $@ $<

# Bison rule (generates both .c and .h)
parser.tab.c parser.tab.h: $(BISON_FILE)
	$(BISON) -d -o parser.tab.c $<

# Special rule for main.o since it depends on generated header
main.o: main.c $(GENERATED_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ 

# Pattern rule for other object files
# This rule will correctly compile asd.c, table.c, and valor_t.c into their respective .o files.
# If any of these files also directly include parser.tab.h, you might consider
# adding specific rules for them similar to main.o, or ensure parser.tab.h
# is a prerequisite for a target that depends on these .o files.
# However, for typical structures, this pattern rule is often sufficient.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -f $(OBJ) $(GENERATED_SOURCES) $(GENERATED_HEADERS) $(TARGET)

# Phony targets
.PHONY: all clean
