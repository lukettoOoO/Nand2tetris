# **Nand2Tetris – From NAND Gates to Tetris** :large_blue_circle:

This repository contains my complete implementation of the Nand2Tetris course (*The Elements of Computing Systems by Nisan & Schocken*).
The project begins with building logic gates and CPU from a single NAND gate and then continues with writing an assembler, VM translator, compiler and an operating system and running high-level language programs on a computer I built from scratch. Each module is
fully tested on the course website using the tools and test files provided by the authors.

This project uses its own assembly language called *Hack* and its own programming language called *Jack* both created by the authors of the book in the scope of testing its modules.

Course website: https://www.nand2tetris.org/

## **⌾ Project structure:**
```
.
├── 01/   # Boolean logic (gates: And, Or, Not, Xor, Mux, DMux)
├── 02/   # Boolean arithmetic (HalfAdder, FullAdder, ALU parts)
├── 03/   # Sequential logic (Flip-Flops, Registers, RAM)
├── 04/   # Machine language (Hack assembly examples)
├── 05/   # Computer architecture (CPU, Memory, Computer.hdl)
├── 06/   # Assembler (translates .asm → .hack)
├── 07/   # VM Translator I (stack arithmetic, memory segments)
├── 08/   # VM Translator II (program flow, functions, calls)
├── 09/   # High-level Jack programs (apps like Pong, Hangman)
├── 10/   # Compiler I (Jack → VM, syntax analysis)
├── 11/   # Compiler II (Jack → VM, code generation)
└── 12/   # Operating System (Math, String, Memory, Array, Sys)
```
Each project includes
```
*.hdl – Hardware Description Language files
*.asm – Hack assembly programs
*.hack – Binary machine code
*.vm – Virtual Machine code
*.jack – High-level programs
*.tst, *.cmp, *.out – Provided test scripts and outputs
```

### **⌾ Final result:**
