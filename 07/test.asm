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
@500
D=A
@THIS
M=D
//initialize the THAT segment to 600
@600
D=A
@THAT
M=D
//test.vm
//push constant 2
@2
D=A
@SP
A=M
M=D
@SP
M=M+1
//push constant 3
@3
D=A
@SP
A=M
M=D
@SP
M=M+1
//pop static 10
@SP
M=M-1
A=M
D=M
@test.10
M=D
