// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input.
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel;
// the screen should remain fully black as long as the key is pressed. 
// When no key is pressed, the program clears the screen, i.e. writes
// "white" in every pixel;
// the screen should remain fully clear as long as no key is pressed.

    // prev_kbd = 0
    @prev_kbd
    M=0

    // endp = 24576
    @24576
    D=A
    @endp
    M=D

    // black = -1
    @black
    M=-1
    // white = 0
    @white
    M=0

(MAIN_LOOP)
    // if prev_kbd == KBD goto MAIN_LOOP
    @KBD
    D=M
    @temp // temp = KBD
    M=D
    @prev_kbd
    D=M
    @temp
    M=M-D  // temp = temp - prev_kbd
    D=M
    @MAIN_LOOP // goto MAIN_LOOP if temp == 0
    D;JEQ
    // prev_kbd = KBD
    @KBD
    D=M
    @prev_kbd
    M=D
    // if KBD == 0 goto WHITE_FILL
    @WHITE_FILL
    D;JEQ
    // goto BLACK_FILL
    @BLACK_FILL
    0;JMP

(WHITE_FILL)
    // pattern = white
    @white
    D=M
    @pattern
    M=D
    // goto FILL
    @FILL
    0;JMP

(BLACK_FILL)
    // pattern = black
    @black
    D=M
    @pattern
    M=D
    // goto FILL
    @FILL
    0;JMP    

(FILL) // fill screen with @pattern and goto MAIN_LOOP
    // p = SCREEN
    @SCREEN
    D=A
    @p
    M=D
(FILL_LOOP)
    // temp = endp
    @endp
    D=M
    @temp
    M=D
    // temp = endp - p
    @p
    D=M
    @temp
    M=M-D
    // if temp == 0 goto END
    D=M
    @MAIN_LOOP
    D;JEQ

    // RAM [p] = pattern
    @pattern
    D=M
    @p
    A=M
    M=D

    // p++
    @p
    M=M+1
    @FILL_LOOP
    0;JMP
