# **Nand2Tetris – From NAND Gates to Tetris** :large_blue_circle:

This repository contains my complete implementation of the Nand2Tetris course (*The Elements of Computing Systems by Nisan & Schocken*).
The project begins with building logic gates and CPU from a single NAND gate and then continues with writing an assembler, VM translator, compiler and an operating system and running high-level language programs on a computer I built from scratch. Each module is
fully tested on the course website using the tools and test files provided by the authors.

This project uses its own assembly language called *Hack* and its own programming language called *Jack* both created by the authors of the book in the scope of testing its modules.

The programming language that I chose to do this project in is **C**.

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
![Final result](https://github.com/lukettoOoO/Nand2tetris/blob/cf2ccb020b9d42672832c4fdd7f5646bf63e91cc/final_result.gif)

*A Pong game played entirely on the built computer, the Jack code written for it is compiled into Virtual Machine code -> assembly code -> binary machine code -> CPU*

## **⌾ Chapters:**

### 1. Boolean logic
This project introduces the foundation of digital hardware: logic gates. Starting from a single primitive gate (NAND), all other Boolean logic gates are constructed. Using HDL (Hardware Description Language), we specify chips as a composition of other chips.
Each gate is verified using the Hardware Simulator with test scripts.

**Basic gates:**
```
	•	Not.hdl – logical negation
	•	And.hdl – logical conjunction
	•	Or.hdl – logical disjunction
	•	Xor.hdl – exclusive or
```
**Selectors:**
```
	•	Mux.hdl – 2-way multiplexor
	•	DMux.hdl – 2-way demultiplexor
```

**Multi-bit Gates:**
```
	•	Not16.hdl – bitwise NOT on 16-bit input
	•	And16.hdl – bitwise AND on 16-bit input
	•	Or16.hdl – bitwise OR on 16-bit input
	•	Mux16.hdl – selects between two 16-bit inputs
```

**Multi-way Gates:**
```
	•	Or8Way.hdl – OR across 8 input bits
	•	Mux4Way16.hdl – selects among 4 inputs (16-bit each)
	•	Mux8Way16.hdl – selects among 8 inputs (16-bit each)
	•	DMux4Way.hdl – routes input into 1 of 4 outputs
	•	DMux8Way.hdl – routes input into 1 of 8 outputs
```
*Example: Xor.hdl - implentation & testing using the Hardware Emulator*

![Xor gate implementation](https://github.com/lukettoOoO/Nand2tetris/blob/1d9557234294cab04a8a0113b08135ccb4fc7ea1/xor.png)

![Xor gate testing](https://github.com/lukettoOoO/Nand2tetris/blob/627de22d0c837eb314a7d63dda1a0a7d337e706a/xor.gif)

### 2. Boolean Arithmetic
Chapter 2 builds arithmetic from the logic made in Chapter 1 including binary addition (bitwise + carry propagation), 2's complement representation for signed integers, multi-bit adders from half- and full adders, ALU.
```
	•	HalfAdder.hdl — adds two single bits → produces sum and carry.
	•	FullAdder.hdl — adds three bits (a, b, carryIn) → sum, carryOut.
	•	Add16.hdl (or Adder.hdl) — 16-bit ripple adder (wired from 16 FullAdders).
	•	Inc16.hdl — incrementer (adds 1 to a 16-bit word).
	•	ALU.hdl — 16-bit Arithmetic-Logic Unit.
```
*ALU functions (examples): 0, 1, -1, x, y, !x, !y, -x, -y, x+1, y+1, x-1, y-1, x+y, x-y, y-x, x&y, x|y — selectable by the control bits.*

*Implementation and testing is done using the provided Hardware Emulator like in the example above.*

### 3. Sequential Logic
In this chapter, we move from combinational logic (stateless) to sequential logic (stateful) like Time & clocked circuits (a data flip-flop stores a bit until the next clock tick), registers (collections of flip-flops used to store multi-bit values), RAM hierarchy (memory is built recursively: RAM8 → RAM64 → RAM512 → RAM4K → RAM16K), Program counter (a sequential chip that increments, resets, or loads values depending on control inputs)

```
	•	Bit.hdl — 1-bit register (stores a single bit).
	•	Register.hdl — 16-bit register.
	•	RAM8.hdl, RAM64.hdl, RAM512.hdl, RAM4K.hdl, RAM16K.hdl — scalable RAM hierarchy.
	•	PC.hdl — Program Counter (increments by 1, loads a value, or resets to 0).
```

*Implementation and testing is done using the provided Hardware Emulator like in the example above.*

### 4. Machine Language
This chapter defines the **Hack machine language**, the instruction set for the computer we’ll finish building in the following chapters.
It’s a simple 16-bit instruction set that supports:

	•	A-instructions — load a constant/address into the A register.
  
	•	C-instructions — perform computations with the ALU, store results, and optionally jump.
  
	•	Labels/variables (symbols) — introduced later when using assembly (next chapter).

Programs are written in Hack assembly (.asm) and then assembled into binary (.hack).
```
	•	Mult.asm — multiplies two numbers.
```

The Machine Language specification is provided in the book.

*Example:*

```
0000 0000 0000 0111    // @7 -> A-instruction that loads the constant 7 into the A register
1111 1101 1101 1000    // MD=M+1 -> C-instruction that looks at RAM[7], adds 1 to it and stores the result in both the D register and back into RAM[7]
```
*.hack files are tested using the CPU emulator*


### 5. Computer Architecture
In this chapter, we assemble everything from Chapters 1–3 (ALU, registers, memory, ROM, RAM) into a complete computer called Hack.
This computer can run the machine language programs from Chapter 4.
```
	•	Memory.hdl — integrates RAM16K, Screen, and Keyboard into the address space.
	•	CPU.hdl — the Hack CPU:
    	•	Inputs: current instruction (inM, instruction, reset).
    	•	Outputs: memory write signal, data to memory, next PC address.
	•	Executes A- and C-instructions using the ALU and registers (A, D, PC).
	•	Computer.hdl — top-level chip:
    	•	Integrates CPU, Memory, and ROM32K.
    	•	Can run full Hack machine language programs.
```
*Implementation and testing is done using the provided Hardware Emulator like in the example above.*

### 6. Assembler
This project implements an assembler that translates Hack assembly language (.asm) into Hack machine code (.hack). It handles A- and C-instructions, labels (L-commands), and manages a symbol table for predefined and user-defined variables. The assembler works in two passes: the first pass registers label addresses, and the second pass converts instructions to binary for execution on the Hack CPU.

```
• File reading (open_asm) — loads the .asm file into memory (asmb array), ignoring comments and empty lines.
• Command classification (commandType) — determines if a line is an A_COMMAND, C_COMMAND, or L_COMMAND.
• Symbol handling (symbol, Constructor, addEntry, contains, GetAddress) — manages predefined symbols, labels, and variables with memory addresses.
• Instruction parsing (dest, comp, jump) — extracts dest, comp, and jump parts for C-instructions.
• Binary translation (valueb, destb, compb, jumpb) — converts A- and C-instructions into 16-bit Hack binary.
• Two-pass assembly (write_hack) — first pass records label addresses; second pass generates the .hack file.
```

*Note: It does what it's supposed to do, the only things I did not manage to implement
was whitespace removal, so the user can't use indentation in the .asm files, and dynamic line sizes (the only thing
that is dynamic is the symbol table). Except that, the program works with all the provided test files by nand2tetris.*

**Usage:**
`./Assembler *.asm` (input: *.asm -> output: *.hack)


*Example: translated .asm Pong code using the Assembler into Pong .hack code running on the CPU Emulator*

![Assembler](https://github.com/lukettoOoO/Nand2tetris/blob/8242e8a2f702fd3b61db5936f6de466c9a6af74a/assembler.gif)

![CPU Emulator](https://github.com/lukettoOoO/Nand2tetris/blob/831102490150773216d35b5b605ddc113b53213f/cpu_emulator.gif)

