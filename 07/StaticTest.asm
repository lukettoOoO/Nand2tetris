//initialize the stack pointer to 256
@256
D=A
@SP
M=D
//initialize the LCL segment to 300
@300
D=A
@LCL
M=D
//initialize the ARG segment to 400
@400
D=A
@ARG
M=D
//initialize the THIS segment to 500
@3000
D=A
@THIS
M=D
//initialize the THAT segment to 600
@3010
D=A
@THAT
M=D
//StaticTest.vm
//push constant 111
@111
D=A
@SP
A=M
M=D
@SP
M=M+1
//push constant 333
@333
D=A
@SP
A=M
M=D
@SP
M=M+1
//push constant 888
@888
D=A
@SP
A=M
M=D
@SP
M=M+1
//pop static 8
@SP
M=M-1
A=M
D=M
@StaticTest.8
M=D
//pop static 3
@SP
M=M-1
A=M
D=M
@StaticTest.3
M=D
//pop static 1
@SP
M=M-1
A=M
D=M
@StaticTest.1
M=D
//push static 3
@StaticTest.3
D=M
@SP
A=M
M=D
@SP
M=M+1
//push static 1
@StaticTest.1
D=M
@SP
A=M
M=D
@SP
M=M+1
//sub
@SP
M=M-1
A=M
D=M
@R13
M=D
@SP
M=M-1
A=M
D=M
@R13
D=D-M
@SP
A=M
M=D
@SP
M=M+1
//push static 8
@StaticTest.8
D=M
@SP
A=M
M=D
@SP
M=M+1
//add
@SP
M=M-1
A=M
D=M
@R13
M=D
@SP
M=M-1
A=M
D=M
@R13
D=D+M
@SP
A=M
M=D
@SP
M=M+1
