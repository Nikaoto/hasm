# Makefile
CC:= gcc
CFLAGS:= -Wall -pedantic -std=c99 -O2

all: hasm

hasm: hasm.c file.c
	$(CC) $(CFLAGS) $^ -o hasm

hasm_g: hasm.c file.c
	$(CC) $(CFLAGS) $^ -g -o hasm_g

test:
	cd test && luajit test.lua

.PHONY: test all
