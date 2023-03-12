
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>


enum opcode_decode {R = 0x33, I = 0x13, S = 0x23, L = 0x03, B = 0x63, JALR = 0x67, JAL = 0x6F, AUIPC = 0x17, LUI = 0x37};

typedef struct {
    size_t data_mem_size_;
    uint32_t regfile_[32];
    uint32_t pc_;
    uint8_t* instr_mem_;
    uint8_t* data_mem_;
} CPU;

void CPU_open_instruction_mem(CPU* cpu, const char* filename);
void CPU_load_data_mem(CPU* cpu, const char* filename);

CPU* CPU_init(const char* path_to_inst_mem, const char* path_to_data_mem) {
	CPU* cpu = (CPU*) malloc(sizeof(CPU));
	cpu->data_mem_size_ = 0x400000;
    cpu->pc_ = 0x0;
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
	printf("size of instruction memory: %d Byte\n\n",sb.st_size);
	instr_mem_size =  sb.st_size;
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
	printf("read data for data memory: %d Byte\n\n",sb.st_size);

    cpu->data_mem_ = malloc(cpu->data_mem_size_);
	fread(cpu->data_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}


/**
 * Instruction fetch Instruction decode, Execute, Memory access, Write back
 */

/*Vorbereitung (Opcode, Func3, Func7, Registeroperanten und Immediates extrahieren)*/
 uint8_t get_opcode(uint32_t instruction){
	 uint32_t instructioncode = (instruction & 0x0000007F);
	 return instructioncode;
 }

 uint8_t get_rd(uint32_t instruction) {
	 uint32_t vorbereitung = (instruction >>7);
	 uint32_t rd = (vorbereitung & 0x0000001f);
	 return (uint8_t) rd;
 }

 uint8_t get_func3(uint32_t instruction) {
	 uint32_t vorbereitung = (instruction >>12);
	 uint32_t func3 = (vorbereitung & 0x00000007);
	 return (uint8_t) func3;
 }

 uint8_t get_rs1(uint32_t instruction) {
	 uint32_t vorbereitung = (instruction >> 15);
	 uint32_t rs1 = (vorbereitung & 0x0000001f);
	 return (uint8_t) rs1;
 }

 uint8_t get_rs2(uint32_t instruction) {
	 uint32_t vorbereitung = (instruction >>20);
	 uint32_t rs2 = (vorbereitung & 0x0000001f);
	 return (uint8_t) rs2;
 }

 uint8_t get_func7(uint32_t instruction) {
	 uint32_t vorbereitung = (instruction >>25);
	 uint32_t func7 = (vorbereitung & 0x0000007f);
	 return (uint8_t) func7;
 }


 uint32_t immediateITyp(uint32_t instruction) {
	 uint32_t immediate = (instruction >> 20);
	 if (immediate & 0x800) {
		 immediate = 0xFFFFF000 | immediate;
	 }
	 else;
	 //printf("Immediate I typ %X\n", immediate);
	 return immediate;
 }

 uint8_t shiftimmediate(uint32_t instruction) {
	 uint32_t immediate = immediateITyp(instruction);
	 uint8_t shamt = (uint8_t) (immediate & 0x1f);
	 return shamt;
 }

 uint32_t immediateStyp(uint32_t instruction) {
	 uint32_t vorbereitung1 = (instruction & 0x00000F80); //alle bits außer [4:0] sind 0
	 uint32_t ersterteil = (vorbereitung1 >> 7); //verschiebung um die richtige Position
	 uint32_t vorbereitung2 = (instruction & 0xFE000000); //alle bits außer [11:5] sind 0
	 uint32_t zweiterteil = (vorbereitung2 >> 20); //Verschiebung um die richtige Position >>25 aber danach <<5 d.h >>20
	 uint32_t immediate = (ersterteil | zweiterteil);
	
	 if (immediate & 0x800) {
		 immediate = 0xFFFFF000 | immediate;
	 }
	 else;
	// printf("Immediate S Typ :  %X\n", immediate);
	 return immediate;
 }

int32_t immediateBtyp(uint32_t instruction) {
		 int32_t imm = (0x1E & (uint16_t)(instruction >> 7))
			 | (0x7E0 & (instruction >> 20))
			 | (0x800 & (instruction << 4))
			 | (0x1000 & (instruction >> 19));

		 if (imm & 0x1000) imm = imm | 0xFFFFE000;

		 // printf("Immediate B Typ :  %X\n", immediate);
		 return imm;
}

 uint32_t immediateUTyp(uint32_t instruction) {
	 uint32_t immediate = (instruction & 0xFFFFF000);
	// printf("Immediate U Typ :  %X\n", immediate);
	 return immediate;
 }

 uint32_t immediateJTyp(uint32_t instruction) {
	 uint32_t vorbereitung1 = (instruction & 0x7FE00000); //alle bits außer [10:1] sind 0
	 uint32_t vorbereitung2 = (vorbereitung1 >> 20); //verschiebung der [10:1] bits um die richtige Position
	 uint32_t vorbereitung3 = (instruction & 0x00100000); //alle bits außer [11] sind 0
	 uint32_t vorbereitung4 = (vorbereitung3 >> 9); //verschiebung des [11] bits um die richtige Position
	 uint32_t vorbereitung5 = (instruction & 0x000FF000); //alle bits außer [19:12] sind 0
	 uint32_t vorbereitung6 = (instruction & 0x80000000); //alle bits außer [20] sind 0
	 uint32_t vorbereitung7 = vorbereitung6 >> 11; //verschiebung um die richtige Position
	 uint32_t erstesteil = (vorbereitung4 | vorbereitung2); //[11|10:1]
	 uint32_t immediate = (vorbereitung7 | vorbereitung5 | erstesteil); //[20|19:12|11|10:1]

	 if (immediate & 0x800) {
		 immediate = 0xFFF00000 | immediate;
	 }
	 else;
	// printf("Immediate J Typ :  %X\n", immediate);
	 return immediate;
 }

 /*Vorbereitung Ende*/

 /*Instruktionen*/
 void lui(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = immediateUTyp(instruction);
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void auipc(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->pc_ + immediateUTyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }


 void jal(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->pc_ + 4);
	 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateJTyp(instruction)));
 }

 void jalr(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->pc_ + 4);
	 cpu->pc_ = (cpu->regfile_[get_rs1(instruction)] + ((int32_t)immediateITyp(instruction)));
 }

 void beq(CPU* cpu, uint32_t instruction) {
	 if (cpu->regfile_[get_rs1(instruction)] == cpu->regfile_[get_rs2(instruction)]) {
		 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateBtyp(instruction)));
	 }
	 else {
		 cpu->pc_ = (cpu->pc_ + 4);
	 }
 }

 void bne(CPU* cpu, uint32_t instruction) {
	 if (cpu->regfile_[get_rs1(instruction)] != cpu->regfile_[get_rs2(instruction)]) {
		 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateBtyp(instruction)));
	 }
	 else {
		 cpu->pc_ = (cpu->pc_ + 4);
	 }
 }

 void blt(CPU* cpu, uint32_t instruction) {
	 if (cpu->regfile_[get_rs1(instruction)] < cpu->regfile_[get_rs2(instruction)]) {
		 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateBtyp(instruction)));
	 }
	 else {
		 cpu->pc_ = (cpu->pc_ + 4);
	 }
 }

 void bge(CPU* cpu, uint32_t instruction) {
	 if ((int32_t)cpu->regfile_[get_rs1(instruction)] >=(int32_t) cpu->regfile_[get_rs2(instruction)]) {
		 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateBtyp(instruction)));
	 }
	 else {
		 cpu->pc_ = (cpu->pc_ + 4);
	 }
 }


 void bltu(CPU* cpu, uint32_t instruction) {
	 if (cpu->regfile_[get_rs1(instruction)] < cpu->regfile_[get_rs2(instruction)]) {
		 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateBtyp(instruction)));
	 }
	 else {
		 cpu->pc_ = (cpu->pc_ + 4);
	 }
 }


 void bgeu(CPU* cpu, uint32_t instruction) {
	 if (cpu->regfile_[get_rs1(instruction)] >= cpu->regfile_[get_rs2(instruction)]) {
		 cpu->pc_ = (cpu->pc_ + ((int32_t)immediateBtyp(instruction)));
	 }
	 else {
		 cpu->pc_ = (cpu->pc_ + 4);
	 }
 }

 void lb(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = cpu->data_mem_[(cpu->regfile_[get_rs1(instruction)]) + immediateITyp(instruction)];

	 if (cpu->regfile_[get_rd(instruction)] & 0x80) {
		 cpu->regfile_[get_rd(instruction)] = 0xFFFFFF00 | cpu->regfile_[get_rd(instruction)];
	 }
	 else;
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void lh(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (*(uint16_t*)((cpu->regfile_[get_rs1(instruction)]) + immediateITyp(instruction) + (cpu->data_mem_)));
	 if (cpu->regfile_[get_rd(instruction)] & 0x80) {
		 cpu->regfile_[get_rd(instruction)] = 0xFFFFFF00 | cpu->regfile_[get_rd(instruction)];
	 }
	 else;
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void lw(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (*(uint32_t*)((cpu->regfile_[get_rs1(instruction)]) + immediateITyp(instruction) + (cpu->data_mem_)));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void lbu(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->data_mem_[(cpu->regfile_[get_rs1(instruction)] + immediateITyp(instruction))]);
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void lhu(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (*(uint16_t*)(cpu->regfile_[get_rs1(instruction)] + immediateITyp(instruction) + cpu->data_mem_));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void sb(CPU* cpu, uint32_t instruction) {
	 if (cpu->regfile_[get_rs1(instruction)] + (immediateStyp(instruction)) == 0x5000) {
		 putchar(cpu->regfile_[get_rs2(instruction)]);

	 }
	 else {
		 cpu->data_mem_[cpu->regfile_[get_rs1(instruction)] + ((int32_t)(immediateStyp(instruction)))] = ((uint8_t)(cpu->regfile_[get_rs2(instruction)]));
	 }
	 cpu->pc_ = (cpu->pc_ + 4);

 }

 void sh(CPU* cpu, uint32_t instruction) {
	 (*(uint16_t*)(cpu->data_mem_ + cpu->regfile_[get_rs1(instruction)] + immediateStyp(instruction))) = ((uint16_t)(cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void sw(CPU* cpu, uint32_t instruction) {
	 *(uint32_t*)(cpu->data_mem_ + cpu->regfile_[get_rs1(instruction)] + (int32_t)immediateStyp(instruction)) = ((uint32_t)(cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void addi(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] + immediateITyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void slti(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] < immediateITyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }


 void sltiu(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] < immediateITyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void xori(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] ^ immediateITyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void ori(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] | immediateITyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void andi(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] & immediateITyp(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void slli(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] << shiftimmediate(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void srli(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (cpu->regfile_[get_rs1(instruction)] >> shiftimmediate(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }


 void srai(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = (int8_t)(cpu->regfile_[get_rs1(instruction)] >> (int8_t)shiftimmediate(instruction));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void add(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) + (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void sub(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) - (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void sll(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) << (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void slt(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((int32_t)(cpu->regfile_[get_rs1(instruction)]) < ((int32_t)(cpu->regfile_[get_rs2(instruction)])));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void sltu(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) < (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }


 void xorOperation(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) ^ (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void srl(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) >> (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void sra(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((int32_t)(cpu->regfile_[get_rs1(instruction)]) >> (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void orOperation(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) | (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }

 void andOparation(CPU* cpu, uint32_t instruction) {
	 cpu->regfile_[get_rd(instruction)] = ((cpu->regfile_[get_rs1(instruction)]) & (cpu->regfile_[get_rs2(instruction)]));
	 cpu->pc_ = (cpu->pc_ + 4);
 }
 
 /*Ende Instruktionen*/

void CPU_execute(CPU* cpu) {

	uint32_t instruction = *(uint32_t*)(cpu->instr_mem_ + (cpu->pc_  & 0xFFFFF)); //line given by professor

	uint32_t opcode = get_opcode(instruction);
	uint32_t function3 = get_func3(instruction);
	uint32_t function7 = get_func7(instruction);

	switch (opcode)
	{
	case LUI: //binary: 0110111
	//	printf("LUI Operation\n");
		lui(cpu, instruction);
		break;
	case AUIPC: //binary: 0010111
		//printf("AUIPC Operation\n");
		auipc(cpu, instruction);
		break;
	case JAL: //binary: 1101111
		//printf("JAL Opeartion\n");
		jal(cpu, instruction);
		break;
	case JALR: // binary: 1100111
	//	printf("JALR Operation\n");
		jalr(cpu, instruction);
		break;
	case B: //binary: 1100011 
		switch (function3) {
		case 0x0: //BEQ
			//printf("BEQ Operation\n");
			beq(cpu, instruction);
			break;
		case 0x1: //BNE
			//printf("BNE Operation\n");
			bne(cpu, instruction);
			break;
		case 0x4: //BLT
			//printf("BLT Operation\n");
			blt(cpu, instruction);
			break;
		case 0x5://BGE
			//printf("BGE Operation\n");
			bge(cpu, instruction);
			break;
		case 0x6://BLTU
			//printf("BLTU Operation\n");
			bltu(cpu, instruction);
			break;
		case 0x7://BGEU
			//printf("BGEU Operation\n");
			bgeu(cpu, instruction);
			break;
		}
		break;
	case L: //binary 0000011
		switch (function3) {
		case 0x0: //LB
			//printf("LB Operation\n");
			lb(cpu, instruction);
			break;
		case 0x1: //LH
			//printf("LH Operation\n");
			lh(cpu, instruction);
			break;
		case 0x2: //LW 
			//printf("LW Operation\n");
			lw(cpu,instruction);
			break;
		case 0x4: //LBU
			//printf("LBU Operation\n");
			lbu(cpu, instruction);
			break;
		case 0x5: //LHU
			//printf("LHU Operation\n");
			lhu(cpu, instruction);
			break;
		}
		break;
	case S: //bianry: 0100011
		switch (function3)
		{
		case 0x0: //SB
			//printf("SB Operation\n");
			sb(cpu, instruction);
			break;
		case 0x1: //SH
			//printf("SH Operation\n");
			sh(cpu, instruction);
			break;
		case 0x2: //SW
			//printf("SW Operation\n");
			sw(cpu, instruction);
			break;
		}
		break;
	case I: //binary 0010011
		switch (function3)
		{
		case 0x0: //ADDI
			//printf("ADDI Operation\n");
			addi(cpu, instruction);
			break;
		case 0x2: //SLTI
			//printf("SLTI Operation\n");
			slti(cpu, instruction);
			break;
		case 0x3: //SLTIU
			//printf("SLTIU Operation\n");
			sltiu(cpu, instruction);
			break;
		case 0x4: // XORI 
			//printf("XORI Operation\n");
			xori(cpu, instruction);
			break;
		case 0x6: //ORI
			//printf("ORI Operation\n");
			ori(cpu, instruction);
			break;
		case 0x7: //ANDI
			//printf("ANDI Operation\n");
			andi(cpu, instruction);
			break;
		case 0x1: //SLLI
			//printf("SLLI Operation\n");
			slli(cpu, instruction);
			break;
		case 0x5:
			/*TODO*/
			if ((immediateITyp(instruction)&0xFF0) == 0x000) { //SLRI
				//printf("Operation SRLI\n");
				srli(cpu, instruction);
			}
			else {
				//printf("Operation SRAI\n");
				srai(cpu, instruction);
			}
			break;
		}
		break;
	case R: //binary: 0110011
		switch (function3)
		{
		case 0x0:
			if (function7 == 0x00) { //ADD 
				//printf("ADD Operation\n");
				add(cpu, instruction);
			}
			else if (function7 == 0x20) { //SUB
				//printf("SUB Operation\n");
				sub(cpu, instruction);
			}
			else;
			break;
		case 0x1: //SLL
			//printf("SLL Operation\n");
			sll(cpu, instruction);
			break;
		case 0x2: //SLT
			//printf("SLT Operation\n");
			slt(cpu, instruction);
			break;
		case 0x3: //SLTU
			//printf("SLTU Operation\n");
			sltu(cpu, instruction);
			break;
		case 0x4: //XOR
			//printf("XOR Operation\n");
			xorOperation(cpu, instruction);
			break;
		case 0x5:
			if (function7 == 0x00) { //SRL
				//printf("SRL Operation\n");
				srl(cpu, instruction);
			}
			else if (function7 == 0x20) { // SRA
				//printf("SRA Operation\n");
				sra(cpu, instruction);
			}
			else;
			break;
		case 0x6: //OR
		//	printf("OR Operation\n");
			orOperation(cpu, instruction);
			break;
		case 0x7: //AND
		//	printf("AND Operation\n");
			andOparation(cpu, instruction);
			break;
		}
		break;

	}
	cpu->regfile_[0] = 0;
	/*ends here*/
	// TODO

}

int main(int argc, char* argv[]) {
	printf("C Praktikum\nHU Risc-V  Emulator 2022\n");

	CPU* cpu_inst;

	cpu_inst = CPU_init(argv[1], argv[2]);
	
    for(uint32_t i = 0; i < 1000000; i++) { // run 70000 cycles //was i<1000000
    	CPU_execute(cpu_inst);
		//printf("i = %d\n", i);
		//output Regfile
		/*for (uint32_t j = 0; j <= 31; j++) {
			printf("%d: %X\n", j, cpu_inst->regfile_[j]);
		}
		printf("\n");
		printf("\n");
		*/
	}

	printf("\n-----------------------RISC-V program terminate------------------------\nRegfile values:\n");

	//output Regfile
	for(uint32_t i = 0; i <= 31; i++) {
    	printf("%d: %X\n",i,cpu_inst->regfile_[i]);
    }
    fflush(stdout);

	return 0;
}
