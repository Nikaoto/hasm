# Makefile
CC:= gcc
CFLAGS:= -Wall -Wpedantic -std=c99 -O2

all: hasm

hasm: hasm.c file.c
	$(CC) $(CFLAGS) $^ -o hasm

hasm_g: hasm.c file.c
	$(CC) $(CFLAGS) $^ -g -o hasm_g

test: hasm
	cd test && luajit test.lua

bench: hasm
	time -p ./bench.sh 1000 "./hasm test/sandbox/Pong.asm -o /dev/null > /dev/null"

.PHONY: all test bench
