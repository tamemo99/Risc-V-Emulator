#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>


enum opcode_decode { R = 0x33, I = 0x13, S = 0x23, L = 0x03, B = 0x63, JALR = 0x67, JAL = 0x6F, AUIPC = 0x17, LUI = 0x37 };

typedef struct
{
	size_t data_mem_size_;
	uint32_t regfile_[32];
	uint32_t pc_;
	uint8_t* instr_mem_;
	uint8_t* data_mem_;
} CPU;

void CPU_open_instruction_mem(CPU* cpu, const char* filename);
void CPU_load_data_mem(CPU* cpu, const char* filename);

CPU* CPU_init(const char* path_to_inst_mem, const char* path_to_data_mem) {
	CPU* cpu = (CPU*)malloc(sizeof(CPU));
	cpu->data_mem_size_ = 0x400000;
	cpu->pc_ = 0x0; //program counter has register 0
	CPU_open_instruction_mem(cpu, path_to_inst_mem);
	CPU_load_data_mem(cpu, path_to_data_mem);
	return cpu;
}

void CPU_open_instruction_mem(CPU* cpu, const char* filename) {
	uint32_t  instr_mem_size;
	FILE* input_file = fopen(filename, "r");
	if (!input_file) {
		printf("no input\n");
		exit(EXIT_FAILURE);
	}
	struct stat sb;
	if (stat(filename, &sb) == -1) {
		printf("error stat\n");
		perror("stat");
		exit(EXIT_FAILURE);
	}
	printf("size of instruction memory: %d Byte\n\n", sb.st_size);
	instr_mem_size = sb.st_size;
	cpu->instr_mem_ = malloc(instr_mem_size);
	fread(cpu->instr_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}

void CPU_load_data_mem(CPU* cpu, const char* filename) {
	FILE* input_file = fopen(filename, "r");
	if (!input_file) {
		printf("no input\n");
		exit(EXIT_FAILURE);
	}
	struct stat sb;
	if (stat(filename, &sb) == -1) {
		printf("error stat\n");
		perror("stat");
		exit(EXIT_FAILURE);
	}
	printf("read data for data memory: %d Byte\n\n", sb.st_size);

	cpu->data_mem_ = malloc(cpu->data_mem_size_);
	fread(cpu->data_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}


/**
 * Instruction fetch Instruction decode, Execute, Memory access, Write back
 */
uint8_t rd(uint32_t instruction) {
	return (0x1F) & (uint8_t)(instruction >> 7); 
}
uint8_t rs1(uint32_t instruction) {
	return (0x1F) & (uint8_t)(instruction >> 15);
}
uint8_t rs2(uint32_t instruction) {
	return (0x1F) & (uint8_t)(instruction >> 20);
}
uint32_t immediateIType(uint32_t instruction) {
	return (0xFFFF0000) & (instruction >> 20);
}
uint32_t immediateSType(uint32_t instruction) {
	return (0xFE000000) & (instruction >> 25) | (0x1F) & (uint8_t)(instruction >> 7);
}
uint32_t immediateBType(uint32_t instruction) {
	return (0x80000000) & (instruction >> 25) | (0x000007E0) & (instruction >> 25) | (0x1E) & (uint8_t)(instruction >> 7) | (0x80) & (instruction >> 25);
}
uint32_t immediateUType(uint32_t instruction) {
	return 0xFFFFF000 & ((uint32_t)(instruction>>12));
}
uint32_t immediateJType(uint32_t instruction) {
	return (0x000FF000) & (instruction >> 12) | (0x0000007FE) & (instruction >> 12) | (0x00000800) & (instruction >> 12) | (0x00100000) & (instruction >> 12);
}

void execute_lui(CPU* cpu, uint32_t instruction) { //überprüft
	cpu->regfile_[rd(instruction)] = immediateUType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_AUIPC(CPU* cpu, uint32_t instruction) { //richtig
	cpu->regfile_[rd(instruction)] = cpu->pc_ + immediateUType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_JAL(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->pc_ + 4;
	cpu->pc_ = cpu->pc_ + immediateJType(instruction);
}
void execute_JALR(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->pc_ + 4;
	cpu->pc_ = cpu->regfile_[rs1(instruction)] + (int32_t)immediateJType(instruction);
}
void execute_BEQ(CPU* cpu, uint32_t instruction) {
	if (cpu->regfile_[rs1(instruction)] == cpu->regfile_[rs2(instruction)]) {
		cpu->pc_ = cpu->pc_ + (int32_t)immediateBType(instruction);
	}
	else cpu->pc_ = cpu->pc_ + 4;
}
void execute_BNE(CPU* cpu, uint32_t instruction) {
	if (cpu->regfile_[rs1(instruction)] != cpu->regfile_[rs2(instruction)]) {
		cpu->pc_ = cpu->pc_ + (int32_t)immediateBType(instruction);
	}
	else cpu->pc_ = cpu->pc_ + 4;
}
void execute_BLT(CPU* cpu, uint32_t instruction) { //CHECK WITH PROF
	if (cpu->regfile_[rs1(instruction)] < cpu->regfile_[rs2(instruction)]) {
		cpu->pc_ = cpu->pc_ + (int32_t)immediateBType(instruction);
	}
	else cpu->pc_ = cpu->pc_ + 4;
}
void execute_BGE(CPU* cpu, uint32_t instruction) { //CHECK WITH PROF
	if (cpu->regfile_[rs1(instruction)] >= cpu->regfile_[rs2(instruction)]) {
		cpu->pc_ = cpu->pc_ + (int32_t)immediateBType(instruction);
	}
	else cpu->pc_ = cpu->pc_ + 4;
}
void execute_BLTU(CPU* cpu, uint32_t instruction) { //CHECK WITH PROF
	if (cpu->regfile_[rs1(instruction)] < cpu->regfile_[rs2(instruction)]) {
		cpu->pc_ = cpu->pc_ + (int32_t)immediateBType(instruction);
	}
	else cpu->pc_ = cpu->pc_ + 4;
}
void execute_BGEU(CPU* cpu, uint32_t instruction) { // CHECK WITH PROF
	if (cpu->regfile_[rs1(instruction)] >= cpu->regfile_[rs2(instruction)]) {
		cpu->pc_ = cpu->pc_ + (int32_t)immediateBType(instruction);
	}
	else cpu->pc_ = cpu->pc_ + 4;
}
void execute_LB(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->data_mem_[cpu->regfile_[rs1(instruction) + immediateIType(instruction)]];
	cpu->pc_ = cpu->pc_ + 4;

}
void execute_LH(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = *(uint16_t*)(cpu->regfile_[rs1(instruction)] + immediateIType(instruction) + cpu->data_mem_);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_LW(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = *(uint32_t*)(cpu->regfile_[rs2(instruction)] + immediateIType(instruction) + cpu->data_mem_);
}
void execute_LBU(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->data_mem_[cpu->regfile_[rs1(instruction)]] + immediateIType(instruction);
}
void execute_LHU(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = *(uint16_t*)(cpu->regfile_[rs2(instruction)] + immediateIType(instruction) + cpu->data_mem_);
}
void execute_SB(CPU* cpu, uint32_t instruction) {
	cpu->data_mem_[cpu->regfile_[rs1(instruction)] + (int32_t)immediateIType(instruction)] = (uint8_t) cpu->regfile_[rs2(instruction)];
}
void execute_SH(CPU* cpu, uint32_t instruction) {
	*(uint16_t*)(cpu->data_mem_ + cpu->regfile_[rs1(instruction)] + immediateIType(instruction)) = (uint16_t)cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu -> pc_ + 4;
}
void execute_SW(CPU* cpu, uint32_t instruction) {
	*(uint32_t*)(cpu->data_mem_ + cpu->regfile_[rs1(instruction)] + immediateIType(instruction)) = (uint32_t)cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_ADDI(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] + immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SLTI(CPU* cpu, uint32_t instruction) { //check mit prof
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] < immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SLTIU(CPU* cpu, uint32_t instruction) { //check mit prof
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] < immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_XORI(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] ^ immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_ORI(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] | immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_ANDI(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] & immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SLLI(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] << immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SRLI(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] >> immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SRAI(CPU* cpu, uint32_t instruction) { //check mit prof
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] >> immediateIType(instruction);
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_ADD(CPU* cpu, uint32_t instruction) { //check mit prof
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] + cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SUB(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] - cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SLL(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] << cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SLT(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = (int32_t)cpu->regfile_[rs1(instruction)] < (int32_t)cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SLTU(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] < cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_XOR(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] - cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 2;
}
void execute_SRL(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] >> cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_SRA(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = (int32_t)cpu->regfile_[rs1(instruction)] >> cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_OR(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] | cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}
void execute_AND(CPU* cpu, uint32_t instruction) {
	cpu->regfile_[rd(instruction)] = cpu->regfile_[rs1(instruction)] & cpu->regfile_[rs2(instruction)];
	cpu->pc_ = cpu->pc_ + 4;
}





void CPU_execute(CPU* cpu) {


	cpu->regfile_[0] = 0;

	uint32_t instruction = *(uint32_t*)(cpu->instr_mem_ + (cpu->pc_ & 0xFFFFF));
	uint32_t funct3 = (instruction >> 12) & 0x7;
	uint32_t funct7 = (instruction >> 25) & 0x7F;
	uint8_t opcode; //existiert immer egal in welcher TYP  


	opcode = (0x1F) & (uint8_t)(instruction >> 0);

	switch (opcode)
	{
	case LUI: //binary: 0110111
		
		execute_lui(cpu, instruction);
		printf("Lui Operation\n");
		break;
	case AUIPC: //binary: 0010111
		printf("AUIPC Operation\n");
		execute_AUIPC(cpu, instruction);
		break;
	case JAL: //binary: 1101111
		printf("JAL Operation\n");
		execute_JAL(cpu, instruction);
		break;
	case JALR: // binary: 1100111
		printf("JALR Operation\n");
		execute_JALR(cpu, instruction);
		break;
	case B: //binary: 1100011 
		switch (funct3) {
		case 0x0: //BEQ
			printf("BEQ Operation\n");
			execute_BEQ(cpu, instruction);
			break;
		case 0x01: //BNE
			printf("BEQ Operation\n");
			execute_BNE(cpu, instruction);
			break;
		case 0x4: //BLT
			printf("BLT Operation\n");
			execute_BLT(cpu, instruction);
			break;
		case 0x5: //BGE
			printf("BGE Operation\n");
			execute_BGE(cpu, instruction);
			break;
		case 0x6: //BLTU
			printf("BLTU Operation\n");
			execute_BLTU(cpu, instruction);
			break;
		case 0x7: //BGEU
			printf("BGEU Operation\n");
			execute_BGEU(cpu, instruction);
			break;
		}
		break;
	case L: //binary 0000011
		switch (funct3)
		{
		case 0x0: //LB
			printf("LB Operation\n");
			execute_LB(cpu, instruction);
			break;
		case 0x1://LH
			printf("LH Operation\n");
			execute_LH(cpu, instruction);
			break;
		case 0x2: //LW
			printf("LW Operation\n");
			execute_LW(cpu, instruction);
			break;
		case 0x4: //LBU
			printf("LBU Operation\n");
			execute_LBU(cpu, instruction);
			break;
		case 0x5: //LHU
			printf("LHU Operation\n");
			execute_LHU(cpu, instruction);
			break;
		}
		break;
	case S: //bianry: 0100011
		switch (funct3) {
		case 0x0: //SB
			printf("SB Operation\n");
			execute_SB(cpu, instruction);
			break;
		case 0x1: //SH
			printf("SH Operation\n");
			execute_SH(cpu, instruction);
			break;
		case 0x2: //SW
			printf("SW Operation\n");
			execute_SW(cpu, instruction);
			break;
		}
		break;
	case I: //binary 0010111
		switch (funct3) {
		case 0x0: //ADDI
			printf("ADDI Operation\n");
			execute_ADDI(cpu, instruction);
			break;
		case 0x1: //SLLI
			printf("SLLI Operation\n");
			execute_SLLI(cpu, instruction);
			break;
		case 0x2: //SLTI
			printf("SLTI Operation\n");
			execute_SLTI(cpu, instruction);
			break;
		case 0x3: //SLTIU
			printf("SLTUI Operation\n");
			execute_SLTIU(cpu, instruction);
			break;
		case 0x4: //XORI
			printf("XORI Operation\n");
			execute_XORI(cpu, instruction);
			break;
		case 0x5:
			if (immediateIType(instruction) == 0x20) { //SRAI
				printf("SRAI Operation\n");
				execute_SRAI(cpu, instruction);
			}
			else if (immediateIType(instruction) == 0x00) { //SRLI
				printf("SRAI Operation\n");
				execute_SRLI(cpu, instruction);
			}
			else;
			break;
		case 0x6://ORI
			printf("ORI Operation\n");
			execute_ORI(cpu, instruction);
			break;
		case 0x7: //ANDI
			printf("ANDI Operation\n");
			execute_ANDI(cpu, instruction);
			break;
		}
		break;
	case R: //binary: 0110011
		switch (funct3) {
		case 0x0:
			if (funct7 == 0x00) {//add
				printf("add Operation\n");
				execute_ADD(cpu, instruction);
			}
			else if (funct7 == 0x20) { //sub
				printf("sub Operation\n");
				execute_SUB(cpu, instruction);
			}
			else;
			break;
		case 0x1: //SLL
			printf("sll Operation\n");
			execute_SLL(cpu, instruction);
			break;
		case 0x2: //SLT
			printf("slt Operation\n");
			execute_SLT(cpu, instruction);
			break;
		case 0x3: //SLTU
			printf("sltu Operation\n");
			execute_SLTU(cpu, instruction);
			break;
		case 0x4: //XOR
			printf("xor Operation\n");
			execute_XOR(cpu, instruction);
			break;
		case 0x5:
			if (funct7 == 0x00) {//SRL
				printf("Lu Operation\n");
				execute_SRL(cpu, instruction);
			}
			else if (funct7 == 0x20) { //SRA
				printf("sra Operation\n");
				execute_SRA(cpu, instruction);
			}
			else;
			break;
		case 0x6: //OR
			printf("or Operation\n");
			execute_OR(cpu, instruction);
			break;
		case 0x7: // AND
			printf("and Operation\n");
			execute_AND(cpu, instruction);
			break;
		}
		break;

	}

}

int main(int argc, char* argv[]) {
	printf("C Praktikum\nHU Risc-V  Emulator 2022\n");

	CPU* cpu_inst;

	cpu_inst = CPU_init(argv[1], argv[2]);
	for (uint32_t i = 0; i < 1000000; i++) { // run 70000 cycles
		CPU_execute(cpu_inst);
	}

	printf("\n-----------------------RISC-V program terminate------------------------\nRegfile values:\n");

	//output Regfile
	for (uint32_t i = 0; i <= 31; i++) {
		printf("%d: %X\n", i, cpu_inst->regfile_[i]);
	}
	fflush(stdout);

	return 0;
}