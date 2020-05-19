// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)

    // R2 = 0
    @R2 
    M=0

    // if R0 == 0 goto END
    @R0
    D=M
    @END
    D;JEQ

    // if R1 == 0 goto END
    @R1
    D=M
    @END
    D;JEQ

    // a = R0
    @R0
    D=M
    @a
    M=D
    // b = R1
    @R1
    D=M
    @b
    M=D
    // sign = b
    @sign
    M=D
    // sign = sign & a
    @a
    D=M
    @sign
    M=M&D
    // sign = !sign
    M=!M
    // nota = !a
    @a
    D=M
    @nota
    M=!D
    // nota_and_notb = !b
    @b
    D=M
    @nota_and_notb
    M=!D
    // nota_and_notb &= nota
    @nota
    D=M
    @nota_and_notb
    M=M&D
    // sign &= nota_and_notb
    D=M // D = nota_and_notb
    @sign
    M=M&D
    // sign &= 2^15
    @32768
    D=A
    @sign
    M=M&D

    // if sign < 0 goto NEGATIVE
    D=M
    @NEGATIVE
    D;JLT

// OPTIM:
// compare a and b
// write smaller into b and larger into a
// this reduces the number of additions in LOOP

(POSITIVE)
    // delta = a
    @a
    D=M
    @delta
    M=D
    // idelta = -1
    @idelta
    M=-1
    // goto LOOP
    @LOOP
    0;JMP

(NEGATIVE)
    // delta = -a
    @a
    D=M
    @delta
    M=-D
    // idelta = 1
    @idelta
    M=1
    // goto LOOP
    @LOOP
    0;JMP

(LOOP)
    // if b == 0 goto END
    @b
    D=M
    @END
    D;JEQ
    // R2 += delta
    @delta
    D=M
    @R2
    M=M+D
    // b += idelta
    @idelta
    D=M
    @b
    M=M+D
    // goto LOOP
    @LOOP
    0;JMP

(END)
    @END
    0;JMP