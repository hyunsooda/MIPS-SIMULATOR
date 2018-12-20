// ref : https://opencores.org/project/plasma/opcodes

#include <stdio.h>
#include <string.h>
//some definitions
#define FALSE 0
#define TRUE 1
#define WORD 32

// R-type: add, jr
// I-type(also branch): addi, lw, sw, beq, slti
// J-type: jal, j
// 10진수로표시햇음
// R-format
#define add 0
#define jr 0

// I-format
#define addi 8
#define lw 35
#define sw 43
#define beq 4
#define slti 10

// J-format
#define j 2
#define jal 3

int funct[6];
int opcode[6];
int rs_arr[5], rt_arr[5], rd_arr[5], constant_arr[16];
int addr_arr[26];
int op, rs, rt, rd, ft, constant, addr;
int msb4bit[4];
int* temp;
int* temp2;

//clock cycles
long long cycles;

int ALU_result;
int* write_register = NULL;

// registers
int regs[32];

// program counter
int pc;

// memory
#define INST_MEM_SIZE 32*1024
#define DATA_MEM_SIZE 32*1024
int inst_mem[INST_MEM_SIZE]; //instruction memory
int data_mem[DATA_MEM_SIZE]; //data memory

//misc. function
void init();

void print_reg() {
	char hex[32][32];
	for(int i=0; i<WORD; i++) 
		sprintf(&hex[i], "%x", regs[i]);

	printf("R0  [r0] = %s\n", hex[0]);
	printf("R1  [at] = %s\n", hex[1]);
	printf("R2  [v0] = %s\n", hex[2]);
	printf("R3  [v1] = %s\n", hex[3]);
	printf("R4  [a0] = %s\n", hex[4]);
	printf("R5  [a1] = %s\n", hex[5]);
	printf("R6  [a2] = %s\n", hex[6]);
	printf("R7  [a3] = %s\n", hex[7]);
	printf("R8  [t0] = %s\n", hex[8]);
	printf("R9  [t1] = %s\n", hex[9]);
	printf("R10 [t2] = %s\n", hex[10]);
	printf("R11 [t3] = %s\n", hex[11]);
	printf("R12 [t4] = %s\n", hex[12]);
	printf("R13 [t5] = %s\n", hex[13]);
	printf("R14 [t6] = %s\n", hex[14]);
	printf("R15 [t7] = %s\n", hex[15]);
	printf("R16 [s0] = %s\n", hex[16]);
	printf("R17 [s1] = %s\n", hex[17]);
	printf("R18 [s2] = %s\n", hex[18]);
	printf("R19 [s3] = %s\n", hex[19]);
	printf("R20 [s4] = %s\n", hex[20]);
	printf("R21 [s5] = %s\n", hex[21]);
	printf("R22 [s6] = %s\n", hex[22]);
	printf("R23 [s7] = %s\n", hex[23]);
	printf("R24 [t8] = %s\n", hex[24]);
	printf("R25 [t9] = %s\n", hex[25]);
	printf("R26 [k0] = %s\n", hex[26]);
	printf("R27 [k1] = %s\n", hex[27]);
	printf("R28 [gp] = %s\n", hex[28]);
	printf("R29 [sp] = %s\n", hex[29]);
	printf("R30 [s8] = %s\n", hex[30]);
	printf("R31 [ra] = %s\n", hex[31]);
	printf("----------------------------\n");
}
void print_cycles() {
	printf("----------------------------\n");
	printf("Clock cycles = %d", cycles);
}
void print_pc() {
	char hex[32];
	sprintf(hex, "%x", pc);
	printf("\npc         = %s\n\n\n", hex);
}
long fetch() { 
	return inst_mem[pc++];
}
int* hex2bin(long n)
{
	int i,k,mask,cnt=0;
	int* arr = (int*)malloc(sizeof(int)*32);

	for(i=sizeof(int)*8-1;i>=0;i--)
	{
		mask = 1<<i;
		k = n&mask;
		if(!k)
			arr[cnt++] = 0;
		else 
			arr[cnt++] = 1;
		// k==0?printf("0"):printf("1");
	}
	return arr;
}
int bin2dec(int* bin, int len, int isPossibleNegative)
{
	int sum = 0, pow2 = 1;
	int* collection;
    for(int i=len-1; i>=0; i--) {
		if(i != len-1) 
			pow2 *= 2;
		if(bin[i]) 
			sum += pow2;
	}

	if(bin[0] && isPossibleNegative) { // 음수처리 
		return -(65535 - sum + 1);
	} 
	return sum;
}
int* dec2bin(int n) {
	int c, k, cnt = 0;
	int* arr = (int*)malloc(sizeof(int)*32);
	for (c = 31; c >= 0; c--) {
		k = n >> c;
	
		if (k & 1)
			arr[cnt++] = 1;
		else
			arr[cnt++] = 0;
  	}
	return arr;
}
int decode(long inst) {
	int* word = hex2bin(inst);

	memmove(opcode, word, sizeof(int)*6);
	memmove(funct, word+26, sizeof(int)*6);
	op = bin2dec(opcode, 6, 0);
	ft = bin2dec(funct, 6, 0);
	
	switch(op) {
		case 0:
			memmove(rs_arr, word+6, sizeof(int)*5);
			memmove(rt_arr, word+11, sizeof(int)*5);
			memmove(rd_arr, word+16, sizeof(int)*5);
			rs = bin2dec(rs_arr, 5, 0);
			rt = bin2dec(rt_arr, 5, 0);
			rd = bin2dec(rd_arr, 5, 0);

			return 0;
		case addi:
			memmove(rs_arr, word+6, sizeof(int)*5);
			memmove(rt_arr, word+11, sizeof(int)*5);
			memmove(constant_arr, word+16, sizeof(int)*16);
			rs = bin2dec(rs_arr, 5, 0);
			rt = bin2dec(rt_arr, 5, 0);
			constant = bin2dec(constant_arr, 16, 1);

			/*
			실제 memory address에서 음수는 존재하지않는다. 그래서 스택포인터에 마이너스하는
			연산은 플러스로바꿔서 생각한다. 또한 우리프로그램에서의 메모리최소단위는 4바이트기때문에
			-4는 +1이되고 -8은 2로 컨버팅되야한다.
			*/

			return addi;
		case lw:
			memmove(rs_arr, word+6, sizeof(int)*5);
			memmove(rt_arr, word+11, sizeof(int)*5);
			memmove(constant_arr, word+16, sizeof(int)*16);
			rs = bin2dec(rs_arr, 5, 0);
			rt = bin2dec(rt_arr, 5, 0);
			constant = bin2dec(constant_arr, 16, 0);

			return lw;
		case sw:
			memmove(rs_arr, word+6, sizeof(int)*5);
			memmove(rt_arr, word+11, sizeof(int)*5);
			memmove(constant_arr, word+16, sizeof(int)*16);
			rs = bin2dec(rs_arr, 5, 0);
			rt = bin2dec(rt_arr, 5, 0);
			constant = bin2dec(constant_arr, 16, 0);

			return sw;
		case beq:
			memmove(rs_arr, word+6, sizeof(int)*5);
			memmove(rt_arr, word+11, sizeof(int)*5);
			memmove(constant_arr, word+16, sizeof(int)*16);
			rs = bin2dec(rs_arr, 5, 0);
			rt = bin2dec(rt_arr, 5, 0);
			constant = bin2dec(constant_arr, 16, 0);

			return beq;
		case slti:
			memmove(rs_arr, word+6, sizeof(int)*5);
			memmove(rt_arr, word+11, sizeof(int)*5);
			memmove(constant_arr, word+16, sizeof(int)*16);
			rs = bin2dec(rs_arr, 5, 0);
			rt = bin2dec(rt_arr, 5, 0);
			constant = bin2dec(constant_arr, 16, 0);

			return slti;
		case j:
			memmove(addr_arr, word+6, sizeof(int) * 26); 
			addr = bin2dec(addr_arr, 26, 0); // target address
			temp = dec2bin(pc);
			memmove(msb4bit, temp, sizeof(int) * 4);

			return j;
		case jal:
			memmove(addr_arr, word+6, sizeof(int) * 26); 
			addr = bin2dec(addr_arr, 26, 0); // target address

			return jal;
	}
}
void exe(int which_operation) {
	switch(which_operation) {
		case 0:
			if(ft == 32) { // add
				if(!rs) {
					ALU_result = regs[rt];
					write_register = regs + rd;	
				} else 
					ALU_result = regs[rs] + regs[rt];
					write_register = regs + rd;
			} else { // jr
				pc = regs[rs];
			}
		break;
		case addi:
			if(rs == 29) {
				if(!rs) {
					ALU_result = constant / 4;
					write_register = regs + rt;
				} else {
					ALU_result = regs[rs] + constant / 4;	
					write_register = regs + rt;
				}
			} else {
				if(!rs) {
					ALU_result = constant;
					write_register = regs + rt;
				} else {
					ALU_result = regs[rs] + constant;
					write_register = regs + rt;
				}
			}
		break;
		case lw:
			if(rs == 29) {
				ALU_result = constant / 4 + regs[rs];
				write_register = regs + rt;
			} else {
				ALU_result = regs[rs] + constant;
				write_register = regs + rt;
			}
		break;
		case sw:
			if(rs == 29)  
				ALU_result = constant / 4 + regs[rs];
			else 
				ALU_result = regs[rs] + constant;
		break;
		case beq:
			if(!rs) {
				if(rs == rt)
					pc += constant * 1;  
			} else {
				if(regs[rs] == rt) 
					pc += constant * 1;  
			}	
		break;
		case slti:
			ALU_result = regs[rs] < constant ? 1 : 0;
			write_register = regs + rt;
		break;
		case j:
			pc = bin2dec(msb4bit, 4, 0) | (addr * 1);
		break;
		case jal:
			regs[31] = pc;
			pc = addr * 1;
		break;
	}
}
void mem(int which_operation) {
	switch(which_operation) {
		case sw:
			data_mem[ALU_result] = regs[rt];
		break;
		case lw:
			ALU_result = data_mem[ALU_result];
		break;
	}
}
void wb() {
	if(write_register) {
		*write_register = ALU_result;
		write_register = NULL;
	}
}

//main
int main(int argc, char* argv[])
{
	long inst;
	int which_operation;
	char* mode = argv[1];
	char done=FALSE;
	init();
	
	while(!done) {
		inst = fetch();
		which_operation = decode(inst);
		exe(which_operation);
		mem(which_operation);
		wb();

		cycles++;

		if(!strcmp(mode, "0")) {
			print_cycles();
			print_pc();		
			print_reg(); 
		}

		if(regs[9] == 10)
			done = TRUE;
	}
	if(!strcmp(mode, "1")) {
		print_cycles();
		print_pc();		
		print_reg(); 
	}
	return 0;
}


/* initialize all datapat elements
//fill the instruction and data memory
//reset the registers
*/
void init()
{
	FILE* fp = fopen("runme.hex","r");
	int i;
	long inst;

	if(fp == NULL)
	{
		fprintf(stderr,"Error opening file.\n");
		exit(2);
	}

	/* fill instruction memory */
	i=0;
	while(fscanf(fp, "%x", &inst)==1)
	{
		inst_mem[i++]=inst;
	}

	fclose(fp);

	/*reset the registers*/
	for(i=0;i<32;i++)
	{
		regs[i]=0;
	}

	/*reset pc*/
	pc=0;
}

