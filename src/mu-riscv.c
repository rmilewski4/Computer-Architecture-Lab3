#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-riscv.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-RISCV Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("forwarding <0-1>\t-- turn data forwarding on/off\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate RISCV for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {

	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */
/***************************************************************/
void rdump() {
	int i;
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < RISCV_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */
/***************************************************************/
void handle_command() {
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-RISCV SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll();
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-RISCV! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value;
			NEXT_STATE.HI = hi_reg_value;
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program();
			break;
		case 'f':
		case 'F':
			if(scanf("%d",&ENABLE_FORWARDING) != 1) {
				break;
			}
		ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n");
		break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {
	int i;
	/*reset registers*/
	for (i = 0; i < RISCV_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;

	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}

	/*load program*/
	load_program();

	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/

	WB();
	MEM();
	EX();
	ID();
	IF();
	//To stop execution when no syscalls are in the program. We assume the program has finished excution when the pipeline registers are completely flushed.
	if(IF_ID.IR == 0 && MEM_WB.IR == 0 && ID_EX.IR == 0 && EX_MEM.IR == 0) {
		printf("All pipeline registers empty, program execution complete!\n");
		RUN_FLAG = FALSE;
	}
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */
/************************************************************/

void WB(){
	if(MEM_WB.RegWrite == FALSE) {
		//increment instruction count
		INSTRUCTION_COUNT++;
		return;
	}
	//extract current instruction from pipeline reg.
	uint32_t instruction = MEM_WB.IR;
	uint32_t opcode = instruction & 127;
	uint32_t rd = (instruction & 4095) >> 7;
	if(instruction){
		switch(opcode){
			case(3): //load
				//write back result to both current and next, as the results might be used earlier in the pipeline as well
				NEXT_STATE.REGS[rd] = MEM_WB.LMD;
				CURRENT_STATE.REGS[rd] = MEM_WB.LMD;
				break;
			case(19): //register-immediate
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				CURRENT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
			case(51): //register-register
			case(103): //jal, jalr
			case(111):
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				CURRENT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
		}
		//increment instruction count
		INSTRUCTION_COUNT++;
	}
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */
/************************************************************/

void MEM_load(uint32_t instruction){
	uint32_t funct3 = (instruction & 28672) >> 12;
	//need to pull out funct3 so we know if it is a lb, lh, or lw
	switch(funct3){
		case(0): //lb - 8 bits
			//read from memory based on address calculated, only pull out first byte.
			MEM_WB.LMD = mem_read_32(MEM_WB.ALUOutput) & 255;
			break;

		case(1): //lh - 16 bits
			MEM_WB.LMD = mem_read_32(MEM_WB.ALUOutput) & 65535;
			break;

		case(2): //lw - 32 bits
			MEM_WB.LMD = mem_read_32(MEM_WB.ALUOutput);
			break;
		
		case(4): //lbu - 8 bits
			MEM_WB.LMD = mem_read_32(MEM_WB.ALUOutput) & 255;
			break;

		case(5): //lhu - 16 bits
			MEM_WB.LMD = mem_read_32(MEM_WB.ALUOutput) & 65535;
			break;
	}
}

void MEM_store(uint32_t instruction){
	uint32_t funct3 = (instruction & 28672) >> 12;

	switch(funct3){
		case(0): //sb - 8 bits
			//write data stored in Temp B into the address calculated .
			mem_write_32(EX_MEM.ALUOutput, EX_MEM.B & 255);
			break;
		case(1): //sh - 16 bits
			mem_write_32(EX_MEM.ALUOutput, EX_MEM.B & 65535);
			break;
		case(2): //sw - 32 bits
			mem_write_32(EX_MEM.ALUOutput, EX_MEM.B);
			break;
		
	}
}

void MEM(){
	uint32_t instruction = EX_MEM.IR;
	uint32_t opcode = instruction & 127;
	//Update pipeline regs.
	MEM_WB.PC = EX_MEM.PC;
	MEM_WB.RegWrite = EX_MEM.RegWrite;
	MEM_WB.IR = EX_MEM.IR;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	MEM_WB.B = EX_MEM.B;
	switch(opcode){
		case(3): //load
			MEM_load(instruction);
			break;
		case(35): //store
			MEM_store(instruction);
			break;
	}
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */
/************************************************************/

//Processing any R instructions in execution stage.
void EX_R_Processing(uint32_t instruction) {
	uint32_t funct3 = (instruction & 28672) >> 12;
	uint32_t funct7 = (instruction & 4261412864) >> 25;
	switch(funct3){
		case 0:
			switch(funct7){
				case 0:		//add
				//for each operation, store in ALU Output pipeline reg. and do the given operation.
					EX_MEM.ALUOutput = ID_EX.A + ID_EX.B;
					break;
				case 32:	//sub
					EX_MEM.ALUOutput = ID_EX.A - ID_EX.B;
					break;
				default:
					RUN_FLAG = FALSE;
					break;
				}	
			break;
		case 6: 			//or
			EX_MEM.ALUOutput = ID_EX.A | ID_EX.B;
			break;
		case 7:				//and
			EX_MEM.ALUOutput = ID_EX.A & ID_EX.B;
			break;
		case 4:		//xor
			EX_MEM.ALUOutput = ID_EX.A ^ ID_EX.B;
			break;
		case 1: //sll
			EX_MEM.ALUOutput = ID_EX.A << ID_EX.B;
			break;
		case 5: 
			switch(funct7) {
				case 0: //srl
					EX_MEM.ALUOutput = ID_EX.A >> ID_EX.B;
					break;
				case 32: //sra
					EX_MEM.ALUOutput = ID_EX.A >> ID_EX.B;
					break;
			}
			break;
		default:
			RUN_FLAG = FALSE;
			break;
	//b-type
	case(99):
		//need to recombine imm
		uint32_t imm4_1and11 = (instruction & 3968) >> 7;
		uint32_t imm12and10_5 = (instruction & 4261412864) >> 25;
		uint32_t bit11 = (imm4_1and11 & 1) << 11;
		uint32_t bits4_1 = imm4_1and11 & 30;
		uint32_t bit12 = (imm12and10_5 & 64) << 6;
		uint32_t bits10_5 = (imm12and10_5 & 63) << 5;
		int32_t immediate = bit11 | bits4_1 | bit12 | bits10_5;
		//if true, number is negative, so fill bits 13-31 with 1s
		if((immediate & 4096) >> 12 == 1) {
			immediate |= 4294959104;
		}
		switch(funct3) {
			case 0: //beq
				if(ID_EX.A == ID_EX.B){
					//if these are the same, then jump will be done. Update the following cycle's PC with the new old PC + the data in immediate
					IF_ID.jumpDetected = TRUE;
					NEXT_STATE.PC = ID_EX.PC + immediate;
				}
				//Since we need to stall the following 2 instructions in every case for just 1 cycle, this will always be set to one, if the branch is taken or not
				IF_ID.jumpStallCount = 1;
			break;
			case 1: //bne
			//The following instructions are identical to beq besides the condition for comparing A & B
				if(ID_EX.A != ID_EX.B){
					IF_ID.jumpDetected = TRUE;
					NEXT_STATE.PC = ID_EX.PC + immediate;
				}
				IF_ID.jumpStallCount = 1;
			break;
			case 4: //blt
				if(ID_EX.A < ID_EX.B){
					IF_ID.jumpDetected = TRUE;
					NEXT_STATE.PC = ID_EX.PC + immediate;

				}
				IF_ID.jumpStallCount = 1;
			break;
			case 5: //bge
				if(ID_EX.A >= ID_EX.B){
					IF_ID.jumpDetected = TRUE;
					NEXT_STATE.PC = ID_EX.PC + immediate;
				}
				IF_ID.jumpStallCount = 1;
			break;
			case 6: //bltu
				if(ID_EX.A < ID_EX.B){
					IF_ID.jumpDetected = TRUE;
					NEXT_STATE.PC = ID_EX.PC + immediate;
				}
				IF_ID.jumpStallCount = 1;
			break;
			case 7: //bgtu
				if(ID_EX.A >= ID_EX.B){
					IF_ID.jumpDetected = TRUE;
					NEXT_STATE.PC = ID_EX.PC + immediate;
				}
				IF_ID.jumpStallCount = 1;
			break;
			default:
				printf("Invalid instruction");
				RUN_FLAG = FALSE;
			break;
		}
	}
}
void EX_Iimm_Processing(uint32_t instruction) {
	uint32_t funct3 = (instruction & 28672) >> 12;
	uint32_t imm5_11 = (instruction & 0xFE000000) >> 25;
	switch (funct3)
	{
	case 0: //addi
	//Combine reg. A with Imm when processing I-Imm.
		EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;
		break;

	case 4: //xori
		EX_MEM.ALUOutput = ID_EX.A ^ ID_EX.imm;
		break;

	case 6: //ori
		EX_MEM.ALUOutput = ID_EX.A | ID_EX.imm;
		break;

	case 7: //andi
		EX_MEM.ALUOutput = ID_EX.A & ID_EX.imm;
		break;

	case 1: //slli
		EX_MEM.ALUOutput = ID_EX.A << ID_EX.imm;
		break;

	case 5: //srli and srai
		switch (imm5_11)
		{
		case 0: //srli
			EX_MEM.ALUOutput = ID_EX.A >> ID_EX.imm;
			break;

		case 32: //srai
			EX_MEM.ALUOutput = ID_EX.A >> ID_EX.imm;
			break;

		default:
			RUN_FLAG = FALSE;
			break;
		}
		break;
	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void EX()
{
	//flushing previous instruction
	if(IF_ID.jumpDetected == TRUE) {
		//stall detected!
		ID_EX.A = 0;
		ID_EX.B = 0;
		ID_EX.ALUOutput = 0;
		ID_EX.PC = 0;
		ID_EX.imm = 0;
		ID_EX.IR = 0;
		ID_EX.RegWrite = 0;
		ID_EX.LMD = 0;
	}
	//Set appropriate registers
	uint32_t instruction = ID_EX.IR;
	EX_MEM.PC = ID_EX.PC;
	EX_MEM.IR = ID_EX.IR;
	EX_MEM.RegWrite = ID_EX.RegWrite;
	uint32_t opcode = instruction & 127;
	//Memory reference, so calculate address jump and store in ALU output
	if(opcode == 3 || opcode == 35) {
		EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;
		EX_MEM.B = ID_EX.B;
	}
	//jal
	else if(opcode == 111) {
		//Store old PC+4 in the WB register so program can return if needed
		EX_MEM.ALUOutput = ID_EX.PC + 4;
		//update PC += imm, flush previous instruction.
		NEXT_STATE.PC = ID_EX.PC + ID_EX.imm;
		//since this is a jump, the jump will always occur, so we tell the later instructions that a jump was detected so they know to stall/not proceed.
		IF_ID.jumpStallCount = 1;
		IF_ID.jumpDetected = TRUE;
	}
	//jalr
	else if(opcode == 103) {
		//store old PC+4 to rd
		EX_MEM.ALUOutput = ID_EX.PC + 4;
		//update pc = rs1 + imm, so program can get
		NEXT_STATE.PC = ID_EX.A + ID_EX.imm;
		IF_ID.jumpStallCount = 1;
		IF_ID.jumpDetected = TRUE;
	}
	//register-immediate, so go to functions above.
	else if(opcode == 19) {
		EX_Iimm_Processing(instruction);
	}
	//register-register
	else if(opcode == 51) {
		EX_R_Processing(instruction);
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */
/************************************************************/
void detect_hazard(uint32_t rs, uint32_t rt) {
	//figure out destination register for the two stages where a hazard could be
	uint32_t EX_MEM_RD = (EX_MEM.IR  & 4095) >> 7;
	uint32_t MEM_WB_RD = (MEM_WB.IR  & 4095) >> 7;
	if(MEM_WB.RegWrite && EX_MEM_RD != 0 && (EX_MEM_RD == rs)) {
		//hazard forwardA = 10
		if(ENABLE_FORWARDING == TRUE) {
			//If we are forwarding, we directly get the ouput from that pipeline register and set it to our ID_EX pipeline reg.
			ID_EX.A = EX_MEM.ALUOutput;
		}
		else {
			//Otherwise, since this insturction that is a hazard is in the EX_MEM stage, we need to do two nops (which since this is decremented later before a nop is done, is set to 3 to start.).
			IF_ID.StallCount = 3;
		}
	}
	//Taking into account if we use an immediate instruction, which passes 0 into rt, so we don't want to check for a hazard if the register is not in use.
	if(rt != 0) {
		if(MEM_WB.RegWrite && EX_MEM_RD != 0 && (EX_MEM_RD == rt)) {
			//hazard forwardB = 10
			if(ENABLE_FORWARDING == TRUE) {
				ID_EX.B = EX_MEM.ALUOutput;
			}
			else {
				IF_ID.StallCount = 3;
			}
		}
	}
	uint32_t instopcode = EX_MEM.IR & 127;
	//Catching double hazards with this if statement
	if(MEM_WB.RegWrite && MEM_WB_RD != 0 && !(EX_MEM.RegWrite && (EX_MEM_RD != 0) && (EX_MEM_RD == rs)) && (MEM_WB_RD == rs)) {
		//hazard forwarda = 01
		if(ENABLE_FORWARDING == TRUE) {
			if(instopcode == 3) {
				//If the instruction opcode is for a load, the thing that needs to be forwarded is in LMD, not ALU output, so we take that result
				ID_EX.A = MEM_WB.LMD;
			}
			else {
				ID_EX.A = MEM_WB.ALUOutput;
			}
		}
		else {
			//Since this hazard occurs in the MEM_WB phase, we only need one nop to continue (set to one more since this is decremented immediately later)
			IF_ID.StallCount = 2;
		}
	}
	if(rt != 0) {
		if(MEM_WB.RegWrite && MEM_WB_RD != 0 && !(EX_MEM.RegWrite && (EX_MEM_RD != 0) && (EX_MEM_RD == rt)) && (MEM_WB_RD == rt)) {
			//hazard forwardB = 01
			if(ENABLE_FORWARDING == TRUE) {
				if(instopcode == 3) {
					ID_EX.B = MEM_WB.LMD;
				}
				else {
					ID_EX.B = MEM_WB.ALUOutput;
				}
			}
			else {
				IF_ID.StallCount = 2;
			}
		}
	}
}
void ID()
{	
	//This covers stalls/flushes. If either conditions are true, this stage will be skipped/stalled.
	if(IF_ID.jumpStallCount > 0 || IF_ID.jumpDetected == TRUE) {
		return;
	}
	uint32_t instruction = IF_ID.IR;
	//Update next stage pipeline reg.
	ID_EX.IR = IF_ID.IR;
	ID_EX.PC = IF_ID.PC;
	uint32_t rs1 = 0;
	uint32_t rs2 = 0;
	uint32_t imm = 0;
	uint32_t imm2 = 0;
	//127 in base-10 is = 1111111 in base 2, which will allow us to extract the opcode from the instruction
	uint32_t opcode = instruction & 127;
	switch(opcode) {
		//R-type instructions
		case(51):
		//Pull out rs1 & rs2, update pipeline regs w/ it.
			rs1 = (instruction & 1015808) >> 15;
			rs2 = (instruction & 32505856) >> 20;
			//detect hazard here
			ID_EX.A = CURRENT_STATE.REGS[rs1];
			ID_EX.B = CURRENT_STATE.REGS[rs2];
			ID_EX.RegWrite = TRUE;
			//look for hazards based on rs1 and rs2 reg numbers
			detect_hazard(rs1,rs2);
			break;
		//I-type Instructions
		case(19):
			rs1 = (instruction & 1015808) >> 15;
			imm = (instruction & 4293918720) >> 20;
			//If this bit is a 1, it is a negative number, so we extend the sign bit all the way over.
			if((imm & 4096) == 4096) {
				imm |= 0xFFFFE000;
			}
			ID_EX.A = CURRENT_STATE.REGS[rs1];
			ID_EX.imm = imm;
			ID_EX.RegWrite = TRUE;
			//Since we have no rs2, we pass a 0 in to let the function know that this is the case.
			detect_hazard(rs1,0);
			break;
		//I-type load instructions
		case(3):
			rs1 = (instruction & 1015808) >> 15;
			imm = (instruction & 4293918720) >> 20;
			//If this bit is a 1, it is a negative number, so we extend the sign bit all the way over.
			if((imm & 4096) == 4096) {
				imm |= 0xFFFFE000;
			}
			ID_EX.A = CURRENT_STATE.REGS[rs1];
			ID_EX.imm = imm;
			ID_EX.RegWrite = TRUE;
			detect_hazard(rs1,0);
			break;
		//S-type instructions
		case(35):
			imm2 = (instruction & 3968) >> 7;
			rs1 = (instruction & 1015808) >> 15;
			rs2 = (instruction & 32505856) >> 20;
			imm = (instruction & 4261412864) >> 25;
			// Recombine immediate
			uint32_t combinedimm = (imm << 5) + imm2;
			//If this bit is a 1, it is a negative number, so we extend the sign bit all the way over.
			if((combinedimm & 4096) == 4096) {
				combinedimm |= 0xFFFFE000;
			}
			ID_EX.A = CURRENT_STATE.REGS[rs1];
			ID_EX.B = CURRENT_STATE.REGS[rs2];
			ID_EX.imm = combinedimm;
			ID_EX.RegWrite = TRUE;
			detect_hazard(rs1,rs2);
			break;
		//b-type
		case(99):{
			rs1 = (instruction & 1015808) >> 15;
			rs2 = (instruction & 32505856) >> 20;
			int32_t imm11 = (instruction & 128) >> 7;
			int32_t imm4_1 = (instruction & 3840) >> 7;
			int32_t imm10_5 = (instruction & 2113929216) >> 25;
			int32_t imm12 = (instruction & 2147483648) >> 31;
			//recombined immediate
			imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1); //13 bits long
			if(imm12 == 1){
				imm = 0xFFFFE000 | imm;
			}
			ID_EX.RegWrite = FALSE;
			ID_EX.imm = imm;
			ID_EX.A = CURRENT_STATE.REGS[rs1];
			ID_EX.B = CURRENT_STATE.REGS[rs2];
			detect_hazard(rs1,rs2);
			break;
		}
		//j-type
		case(111):
			uint32_t fullimm = (instruction & 0xFFFFF000) >> 12;
			uint32_t bit20 = (fullimm & 0x80000);
			uint32_t bit10_1 = (fullimm & 0x7FE00) >> 9;
			uint32_t bit11 = (fullimm & 0x100) << 4;
			uint32_t bit19_12 = (fullimm & 0xFF) << 12;
			ID_EX.imm = bit20 | bit10_1 | bit11 | bit19_12;
			if((ID_EX.imm & 0x80000) == 0x80000) {
				ID_EX.imm |= 0xFFF00000;
			}
			ID_EX.RegWrite = TRUE;
			break;
		//jalr
		case(103):
			rs1 = (instruction & 1015808) >> 15;
			imm = (instruction & 4293918720) >> 20;
			//If this bit is a 1, it is a negative number, so we extend the sign bit all the way over.
			if((imm & 4096) == 4096) {
				imm |= 0xFFFFE000;
			}
			ID_EX.A = CURRENT_STATE.REGS[rs1];
			ID_EX.imm = imm;
			ID_EX.RegWrite = TRUE;
			//Since we have no rs2, we pass a 0 in to let the function know that this is the case.
			detect_hazard(rs1,0);
			break;
		default:
			break;
	}
	//If a stall is detected, then we need to forward 0 control signals to the ID_EX pipeline reg. to simulate a nop
	if(IF_ID.StallCount > 0) {
		ID_EX.A = 0;
		ID_EX.B = 0;
		ID_EX.ALUOutput = 0;
		ID_EX.PC = 0;
		ID_EX.imm = 0;
		ID_EX.IR = 0;
		ID_EX.RegWrite = 0;
		ID_EX.LMD = 0;
	}
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */
/************************************************************/
void IF()
{
	//catching stalls for jump related instructions in similar way as hazard stalls
	if(IF_ID.jumpStallCount > 0) {
		IF_ID.jumpStallCount--;
		return;
	}
	else if(IF_ID.jumpStallCount == 0 && IF_ID.jumpDetected == TRUE) {
		IF_ID.jumpDetected = FALSE;
	}
	//IF there's a stall that needs to be detected, we don't let the IF_ID registers update. We decrement the stall count by one to
	//indicate a successful stall cycle.
	if(IF_ID.StallCount > 0) {
		IF_ID.StallCount--;
		return;
	}
	uint32_t instruction;
	//Read in instruction based on PC
	instruction = mem_read_32(CURRENT_STATE.PC);
	IF_ID.IR = instruction;
	IF_ID.PC = CURRENT_STATE.PC;
	NEXT_STATE.PC += 4; 
}

/************************************************************/
/* Initialize Memory                                                                                                    */
/************************************************************/
void initialize() {
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in RISCV assembly format)    */
/************************************************************/
void print_program(){
	//implementation taken from lab 1.
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE.PC = MEM_TEXT_BEGIN + 4;
	
	uint32_t addressMemory;
	int i = 0;
	while(RUN_FLAG == TRUE){
		addressMemory = CURRENT_STATE.PC;
		
		print_instruction(addressMemory);
		
		NEXT_STATE.PC += 4;
		i++;
	}
}

void R_Print(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t f7) {
	switch(f3){
		case 0:
			switch(f7){
				case 0:		//add
					printf("add x%d, x%d, x%d\n\n", rd, rs1, rs2);
					break;
				case 32:	//sub
					printf("sub x%d, x%d, x%d\n\n", rd, rs1, rs2);
					break;
				default:
					RUN_FLAG = FALSE;
					break;
				}	
			break;
		case 6: 			//or
			printf("or x%d, x%d, x%d\n\n", rd, rs1, rs2);
			break;
		case 7:				//and
			printf("and x%d, x%d, x%d\n\n", rd, rs1, rs2);
			break;
		default:
			RUN_FLAG = FALSE;
			break;
	}

}

void S_Print(uint32_t imm11, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t imm4){
	uint32_t imm = (imm4<< 5) + imm11;

	switch (f3)
		{
		case 0: //sb
			printf("sb x%d, %d(x%d)\n\n", rs2, imm, rs1);
			break;
		
		case 1: //sh		
			printf("sh x%d, %d(x%d)\n\n", rs2, imm, rs1);
			break;

		case 2: //sw
			printf("sw x%d, %d(x%d)\n\n", rs2, imm, rs1);
			break;

		default:
			RUN_FLAG = FALSE;
			break;
	}
	

}

void I_Print(uint32_t imm, uint32_t f3, uint32_t rs1, uint32_t rd){
	switch (f3)
	{
	case 0: //lb
		printf("lb x%d, %d(x%d)\n\n", rd, imm, rs1);
		break;

	case 1: //lh
		printf("lh x%d, %d(x%d)\n\n", rd, imm, rs1);
		
		break;

	case 2: //lw
		printf("lw x%d, %d(x%d)\n\n", rd, imm, rs1);
		break;
	
	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void Iimm_Print(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t imm){
	uint32_t imm0_4 = (imm << 7) >> 7;
	uint32_t imm5_11 = imm >> 5;
	switch (f3)
	{
	case 0: //addi
		printf("addi x%d, x%d, %d\n\n", rd, rs1, imm);
		break;

	case 4: //xori
		printf("xori x%d, x%d, %d\n\n", rd, rs1, imm);
		break;
	
	case 6: //ori
		printf("ori x%d, x%d, %d\n\n", rd, rs1, imm);
		break;
	
	case 7: //andi
		printf("andi x%d, x%d, %d\n\n", rd, rs1, imm);
		break;
	
	case 1: //slli
		printf("slli x%d, x%d, %d\n\n", rd, rs1, imm0_4); 
		break;
	
	case 5: //srli and srai
		switch (imm5_11)
		{
		case 0: //srli
			printf("srli x%d, x%d, %d\n\n", rd, rs1, imm0_4); 
			break;

		case 32: //srai
			printf("srai x%d, x%d, %d\n\n", rd, rs1, imm0_4); 
			break;
		
		default:
			RUN_FLAG = FALSE;
			break;
		}
		break;
	
	case 2:
		break;

	case 3:
		break;

	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}

}

void B_Print(int32_t imm, uint32_t funct3, uint32_t rs1, uint32_t rs2){
	switch(funct3) {
		case 0: //beq
			printf("beq x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 1: //bne
			printf("bne x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 4: //blt
			printf("blt x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 5: //bgt
			printf("bgt x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 6: //bltu
			printf("bltu x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 7: //bgtu
			printf("bgtu x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;
		
		default:
			printf("Invalid instruction");
			RUN_FLAG = FALSE;
			break;
	}

}

/************************************************************/
/* Print the instruction at given memory address (in RISCV assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){

	uint32_t instruction = mem_read_32(addr);
	uint32_t maskopcode = 0x7F;
	uint32_t opcode = instruction & maskopcode;
	if(opcode == 51) { //R-type
		uint32_t maskrd = 0xF80;
		uint32_t rd = instruction & maskrd;
		rd = rd >> 7;
		uint32_t maskf3 = 0x7000;
		uint32_t f3 = instruction & maskf3;
		f3 = f3 >> 12;
		uint32_t maskrs1 = 0xF8000;
		uint32_t rs1 = instruction & maskrs1;
		rs1 = rs1 >> 15;
		uint32_t maskrs2 = 0x1F00000;
		uint32_t rs2 = instruction & maskrs2;
		rs2 = rs2 >> 20;
		uint32_t maskf7 = 0xFE000000;
		uint32_t f7 = instruction & maskf7;
		f7 = f7 >> 25;
		R_Print(rd,f3,rs1,rs2,f7);
	} else if(opcode == 3) { // I-type load
		uint32_t rd = (instruction & 3968) >> 7;
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t imm = (instruction & 4293918720) >> 20;
		I_Print(imm, funct3, rs1, rd);
	} else if(opcode == 19){ //I-type 
		uint32_t rd = (instruction & 3968) >> 7;
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t imm = (instruction & 4293918720) >> 20;
		Iimm_Print(rd, funct3, rs1, imm);
	} else if(opcode == 35){ //S-type
		uint32_t imm = (instruction & 3968) >> 7;
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t rs2 = (instruction & 32505856) >> 20;
		uint32_t imm2 = (instruction & 4261412864) >> 25;
		S_Print(imm2, funct3, rs1, rs2, imm);
	}  else if(opcode == 99){
		
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t rs2 = (instruction & 32505856) >> 20;
		int32_t imm11 = (instruction & 128) >> 7;
		int32_t imm4_1 = (instruction & 3840) >> 7;
		int32_t imm10_5 = (instruction & 2113929216) >> 25;
		int32_t imm12 = (instruction & 2147483648) >> 31;
		int32_t imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1); //13 bits long
		if(imm12 == 1){
			imm = 0xFFFFE000 | imm;
		}
		B_Print(imm, funct3, rs1, rs2);

	} 
	else if(opcode == 111) {
		uint32_t fullimm = (instruction & 0xFFFFF000) >> 12;
		uint32_t bit20 = (fullimm & 0x80000);
		uint32_t bit10_1 = (fullimm & 0x7FE00) >> 9;
		uint32_t bit11 = (fullimm & 0x100) << 4;
		uint32_t bit19_12 = (fullimm & 0xFF) << 12;
		uint32_t convertedimm = bit20 | bit10_1 | bit11 | bit19_12;
			if((convertedimm & 0x80000) == 0x80000) {
				convertedimm |= 0xFFF00000;
			}
		uint32_t rd = (instruction & 3968) >> 7;
		printf("jal x%d, %d\n\n",rd,convertedimm);
	}
	else if(opcode == 103) {
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t rd = (instruction & 3968) >> 7;
		uint32_t imm = (instruction & 4293918720) >> 20;
		//If this bit is a 1, it is a negative number, so we extend the sign bit all the way over.
		if((imm & 4096) == 4096) {
			imm |= 0xFFFFE000;
		}
		printf("jalr x%d, x%d, %d\n\n", rd, rs1, imm);
	}
	else if (opcode==115) {
		printf("ecall\n\n");
		RUN_FLAG = FALSE;
	} else{
		printf("Instruction not found or at EOF!\n");
		RUN_FLAG = FALSE;
	}
	CURRENT_STATE = NEXT_STATE;
	return;
}
/************************************************************/
/* Print the current pipeline                                                                                    */
/************************************************************/
void show_pipeline(){
	//printing out current pipeline
	printf("Current PC:\t0x%x\n",CURRENT_STATE.PC);
	printf("IF/ID.IR\t0x%x\n",IF_ID.IR);
	printf("IF/ID.PC\t0x%x\n\n",IF_ID.PC);

	printf("ID/EX.IR\t0x%x\n",ID_EX.IR);
	printf("ID/EX.A\t\t%d\n",ID_EX.A);
	printf("ID/EX.B\t\t%d\n",ID_EX.B);
	printf("ID/EX.imm\t%d\n\n",ID_EX.imm);

	printf("EX/MEM.IR\t0x%x\n",EX_MEM.IR);
	printf("EX/MEM.A\t%d\n",EX_MEM.A);
	printf("EX/MEM.B\t%d\n",EX_MEM.B);
	printf("EX/MEM.ALUOutput\t%d\n\n",EX_MEM.ALUOutput);

	printf("MEM/WB.IR\t0x%x\n",MEM_WB.IR);
	printf("MEM/WB.ALUOutput\t%d\n",MEM_WB.ALUOutput);
	printf("MEM/WB.LMD\t%d\n\n",MEM_WB.LMD);
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {
	printf("\n**************************\n");
	printf("Welcome to MU-RISCV SIM...\n");
	printf("**************************\n\n");

	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
