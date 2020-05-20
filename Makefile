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

bench:
	$(shell time -p ./bench.sh 1000 "./hasm test/sandbox/PongL.asm -o /dev/null > /dev/null")
	@echo "Ran './hasm test/sandbox/PongL.asm -o /dev/null > /dev/null' 1000 times."

.PHONY: all test bench
