// Initialize
@0
D=M           // D = R0 (first operand)
@2
M=0           // R2 = 0 (initialize result)

// Loop label
(LOOP)
@1
D=M           // D = R1 (second operand)
@END
D;JEQ         // if R1 == 0, end the loop

@0
D=M           // D = R0 (first operand)
@2
M=D+M         // R2 = R2 + R0 (add R0 to the result)

@1
MD=M-1        // R1 = R1 - 1 (decrement R1)
@LOOP
0;JMP         // repeat the loop

// End label
(END)
@END
0;JMP         // infinite loop to end the program