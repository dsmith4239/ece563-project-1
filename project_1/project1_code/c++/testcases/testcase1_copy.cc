#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

//#define DEBUG

using namespace std;

#define PROGRAM_SIZE 50

#define UNDEFINED 0xFFFFFFFF //used to initialize the registers
#define NUM_SP_REGISTERS 9
#define NUM_GP_REGISTERS 7 // changed from 32
#define NUM_OPCODES 16 
#define NUM_STAGES 5

typedef enum {PC = 0, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP} opcode_t;

typedef enum {IF, ID, EXE, MEM, WB} stage_t;

typedef struct{
        opcode_t opcode; //opcode
        unsigned src1; //first source register in the assembly instruction (for SW, register to be written to memory) (rs)
        unsigned src2; //second source register in the assembly instruction (rt)
        unsigned dest; //destination register (rd)
        unsigned immediate; //immediate field
        string label; //for conditional branches, label of the target instruction - used only for parsing/debugging purposes
} instruction_t;

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP"};


class sim_pipe{

	/* Add the data members required by your simulator's implementation here */

        // gp and sp registers (unsigned)
        int gp_registers[NUM_GP_REGISTERS];
		instruction_t IReg[NUM_STAGES];				 // holds IRs: IR[IF] points to next instruction to be fetched
															 // all others correspond to stage they feed into: IR[MEM] holds IR pipeline register at entrance to MEM stage
        unsigned sp_registers[NUM_STAGES][NUM_SP_REGISTERS]; // IR is unused - needs to hold whole instruction
															 // get_sp_register needs to be altered to get correct value from IR array
															 // sp_registers[IF]][PC] holds pc value (beginning/end stage)
															 // other registers correspond to stage they feed into:
															 // stage between EX and MEM is sp_registers[MEM]
															 // stage between ID/EX is sp_registers[EX]
															 // if a SPR is unused by an instruction, it should be set to undefined

        //instruction memory 
        instruction_t instr_memory[PROGRAM_SIZE];

        //base address in the instruction memory where the program is loaded
        unsigned instr_base_address;

	//data memory - should be initialize to all 0xFF
	unsigned char *data_memory;

	//memory size in bytes
	unsigned data_memory_size;
	
	//memory latency in clock cycles
	unsigned data_memory_latency;

    unsigned current_cycle = 0;
    unsigned instructions_executed = 0;
    unsigned stalls = 0;

public:

	//instantiates the simulator with a data memory of given size (in bytes) and latency (in clock cycles)
	/* Note: 
           - initialize the registers to UNDEFINED value 
	   - initialize the data memory to all 0xFF values
	*/
	sim_pipe(unsigned data_mem_size, unsigned data_mem_latency);
	
	//de-allocates the simulator
	~sim_pipe();

	//loads the assembly program in file "filename" in instruction memory at the specified address
	void load_program(const char *filename, unsigned base_address=0x0);

	//runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0) 
	void run(unsigned cycles=0);
	
	//resets the state of the simulator
        /* Note: 
	   - registers should be reset to UNDEFINED value 
	   - data memory should be reset to all 0xFF values
	*/
	void reset();

	// returns value of the specified special purpose register for a given stage (at the "entrance" of that stage)
        // if that special purpose register is not used in that stage, returns UNDEFINED
        //
        // Examples (refer to page C-37 in the 5th edition textbook, A-32 in 4th edition of textbook)::
        // - get_sp_register(PC, IF) returns the value of PC
        // - get_sp_register(NPC, ID) returns the value of IF/ID.NPC
        // - get_sp_register(NPC, EX) returns the value of ID/EX.NPC
        // - get_sp_register(ALU_OUTPUT, MEM) returns the value of EX/MEM.ALU_OUTPUT
        // - get_sp_register(ALU_OUTPUT, WB) returns the value of MEM/WB.ALU_OUTPUT
	// - get_sp_register(LMD, ID) returns UNDEFINED
	/* Note: you are allowed to use a custom format for the IR register.
           Therefore, the test cases won't check the value of IR using this method. 
	   You can add an extra method to retrieve the content of IR */
	unsigned get_sp_register(sp_register_t reg, stage_t stage);

	//returns value of the specified general purpose register
	int get_gp_register(unsigned reg);

	// set the value of the given general purpose register to "value"
	void set_gp_register(unsigned reg, int value);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of clock cycles 
	unsigned get_clock_cycles();

	//returns the number of stalls added by processor
	unsigned get_stalls();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);

	//prints the values of the registers 
	void print_registers();

};

/* =============================================================

   HELPER FUNCTIONS

   ============================================================= */


/* converts integer into array of unsigned char - little indian */
inline void int2char(unsigned value, unsigned char *buffer){
	memcpy(buffer, &value, sizeof value);
}

/* converts array of char into integer - little indian */
inline unsigned char2int(unsigned char *buffer){
	unsigned d;
	memcpy(&d, buffer, sizeof d);
	return d;
}

/* implements the ALU operations */
unsigned alu(unsigned opcode, unsigned a, unsigned b, unsigned imm, unsigned npc){
	switch(opcode){
			case ADD:
				return (a+b);
			case ADDI:
				return(a+imm);
			case SUB:
				return(a-b);
			case SUBI:
				return(a-imm);
			case XOR:
				return(a ^ b);
			case LW:
			case SW:
				return(a + imm);
			case BEQZ:
			case BNEZ:
			case BGTZ:
			case BGEZ:
			case BLTZ:
			case BLEZ:
			case JUMP:
				return(npc+imm);
			default:	
				return (-1);
	}
}

/* =============================================================

   CODE PROVIDED - NO NEED TO MODIFY FUNCTIONS BELOW

   ============================================================= */

/* loads the assembly program in file "filename" in instruction memory at the specified address */
void sim_pipe::load_program(const char *filename, unsigned base_address){

   /* initializing the base instruction address */
   instr_base_address = base_address;

   /* creating a map with the valid opcodes and with the valid labels */
   map<string, opcode_t> opcodes; //for opcodes
   map<string, unsigned> labels;  //for branches
   for (int i=0; i<NUM_OPCODES; i++)
	 opcodes[string(instr_names[i])]=(opcode_t)i;

   /* opening the assembly file */
   ifstream fin(filename, ios::in | ios::binary);
   if (!fin.is_open()) {
      cerr << "error: open file " << filename << " failed!" << endl;
      exit(-1);
   }

   /* parsing the assembly file line by line */
   string line;
   unsigned instruction_nr = 0;
   while (getline(fin,line)){
	// set the instruction field
	char *str = const_cast<char*>(line.c_str());

  	// tokenize the instruction
	char *token = strtok (str," \t");
	map<string, opcode_t>::iterator search = opcodes.find(token);
        if (search == opcodes.end()){
		// this is a label for a branch - extract it and save it in the labels map
		string label = string(token).substr(0, string(token).length() - 1);
		labels[label]=instruction_nr;
                // move to next token, which must be the instruction opcode
		token = strtok (NULL, " \t");
		search = opcodes.find(token);
		if (search == opcodes.end()) cout << "ERROR: invalid opcode: " << token << " !" << endl;
	}
	instr_memory[instruction_nr].opcode = search->second;

	//reading remaining parameters
	char *par1;
	char *par2;
	char *par3;
	switch(instr_memory[instruction_nr].opcode){
		case ADD:
		case SUB:
		case XOR:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
			instr_memory[instruction_nr].src2 = atoi(strtok(par3, "R"));
			break;
		case ADDI:
		case SUBI:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
			instr_memory[instruction_nr].immediate = strtoul (par3, NULL, 0); 
			break;
		case LW:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
			break;
		case SW:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src2 = atoi(strtok(NULL, "R"));
			break;
		case BEQZ:
		case BNEZ:
		case BLTZ:
		case BGTZ:
		case BLEZ:
		case BGEZ:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].label = par2;
			break;
		case JUMP:
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].label = par2;
		default:
			break;

	} 

	/* increment instruction number before moving to next line */
	instruction_nr++;
   }
   //reconstructing the labels of the branch operations
   int i = 0;
   while(true){
   	instruction_t instr = instr_memory[i];
	if (instr.opcode == EOP) break;
	if (instr.opcode == BLTZ || instr.opcode == BNEZ ||
            instr.opcode == BGTZ || instr.opcode == BEQZ ||
            instr.opcode == BGEZ || instr.opcode == BLEZ ||
            instr.opcode == JUMP
	 ){
		instr_memory[i].immediate = (labels[instr.label] - i - 1) << 2;
	}
        i++;
   }

}

/* writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness) */
void sim_pipe::write_memory(unsigned address, unsigned value){
	int2char(value,data_memory+address);
}

/* prints the content of the data memory within the specified address range */
void sim_pipe::print_memory(unsigned start_address, unsigned end_address){
	cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	for (unsigned i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3) cout << endl;
	} 
}

/* prints the values of the registers */
void sim_pipe::print_registers(){
        cout << "Special purpose registers:" << endl;
        unsigned i, s;
        for (s=0; s<NUM_STAGES; s++){
                cout << "Stage: " << stage_names[s] << endl;
                for (i=0; i< NUM_SP_REGISTERS; i++)
                        if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
        }
        cout << "General purpose registers:" << endl;
        for (i=0; i< NUM_GP_REGISTERS; i++)
                if (get_gp_register(i)!=(int)UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}

/* initializes the pipeline simulator */
sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
	data_memory_size = mem_size;
	data_memory_latency = mem_latency;
	data_memory = new unsigned char[data_memory_size];
	reset();
}
	
/* deallocates the pipeline simulator */
sim_pipe::~sim_pipe(){
	delete [] data_memory;
}

/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */


/* body of the simulator */
void sim_pipe::run(unsigned cycles){ // **IF YOU RUN INTO ERRORS, CHECK REGISTER ORDERING IN INSTRUCTION (RS/RT/RD)
    //if cycles = 0, run until EOP
    // or for each cycle ...
	
    /*
    data_memory = new unsigned char[data_memory_size];
    data_memory_latency = mem_latency;

    int gp_registers[NUM_GP_REGISTERS]; //
    unsigned sp_registers[NUM_STAGES][NUM_SP_REGISTERS]; // sp_registers[IF][PC] holds pc value

    //instruction memory
    instruction_t instr_memory[PROGRAM_SIZE];
    */
   current_cycle = 0;
    while((cycles != 0 && current_cycle <= cycles) or (cycles == 0)){
        // during each cycle:
        current_cycle++;        
        // MEM/WB is written back (if necessary)
			IReg[WB] = IReg[MEM];
			for(int i = 0; i < NUM_SP_REGISTERS; i++) sp_registers[WB][i] = sp_registers[MEM][i];
			switch(IReg[WB].opcode){
				case EOP: return; // exit at EOP (but make sure all other instructions have written back)
				break;
				// if ALU: regs[destination] = ALU output (sp_registers[WB][ALU_OUTPUT])
				case ADD:set_gp_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				case SUB:set_gp_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				case XOR:set_gp_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				// if ALU with immediate: regs[rt] = ALU output (sp_registers[WB][ALU_OUTPUT])
				case ADDI:set_gp_register(IReg[WB].src2,sp_registers[WB][ALU_OUTPUT]);
				break;
				case SUBI:set_gp_register(IReg[WB].src2,sp_registers[WB][ALU_OUTPUT]);
				break;
				// if load: regs[rt] = LMD (load memory data) (sp_registers[WB][LMD])
				case LW: set_gp_register(IReg[WB].src2,sp_registers[WB][LMD]);
				break;
			}
        // EX/MEM (memory operations)
			IReg[MEM] = IReg[EXE];
			for(int i = 0; i < NUM_SP_REGISTERS; i++) sp_registers[MEM][i] = sp_registers[EXE][i];
			// PC = NPC
			//sp_registers[IF][PC] = sp_registers[MEM][NPC]; // update PC with propagated value or 
			if(sp_registers[MEM][COND] == 1) sp_registers[MEM][PC] = sp_registers[MEM][ALU_OUTPUT];
			else sp_registers[MEM][PC] = sp_registers[MEM][NPC];
			switch(IReg[MEM].opcode){
				// If load: LMD = Mem[ALU output]
				// If store: Mem[ALU output] = B
				case LW: sp_registers[MEM][LMD] = data_memory[sp_registers[MEM][ALU_OUTPUT]];
				break;
				case SW: write_memory(sp_registers[MEM][ALU_OUTPUT],sp_registers[MEM][B]);
				break;
			}
        // ID/EX (execute)
			IReg[EXE] = IReg[ID];
			for(int i = 0; i < NUM_SP_REGISTERS; i++) sp_registers[EXE][i] = sp_registers[ID][i];
			switch(IReg[EXE].opcode){
				case ADD: 
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][A] + sp_registers[EXE][B];
				break;
				case SUB:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][A] - sp_registers[EXE][B];
				break;
				case ADDI:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][A] + sp_registers[EXE][IMM];
				break;
				case SUBI:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][A] - sp_registers[EXE][IMM];
				break;
				case LW:
					sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][A] + sp_registers[EXE][IMM];
				break;
				case SW:
					sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][A] + sp_registers[EXE][IMM];
				break;
				case XOR:
					sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][A] ^ sp_registers[EXE][B];
				break;
				case BEQZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] == 0);
				break;
				case BNEZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] != 0);
				break;
				case BLTZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] < 0);
				break;
				case BGTZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] > 0);
				break;
				case BLEZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] <= 0);
				break;
				case BGEZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] >= 0);
				break;
				case JUMP:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (true);
				break;

			}
			// If ALU: ALU out = A op B
			// If ALU(Imm): ALU out = A op Imm
			// If memory reference: ALU out = A + Imm
			// Else (if branch) ALU out = NPC + (Imm << 2)
			// Cond = (A == 0)
			
        // IF/ID (decode instruction currently in this register)
			// pass registers through
			for(int i = 0; i < NUM_SP_REGISTERS; i++) sp_registers[ID][i] = sp_registers[IF][i];
			IReg[ID] = IReg[IF];
			// A = regs[rs]
			// B = regs[rt]
			// Imm = (sign-extended) imm field of IR
			sp_registers[ID][A] = gp_registers[IReg[ID].src1];
			if(IReg[ID].opcode != SW && IReg[ID].opcode != LW) sp_registers[ID][B] = gp_registers[IReg[ID].src2]; // do not fill if instruction is LW/SW
			sp_registers[ID][IMM] = IReg[ID].immediate;
			
        // Fetch next instruction (initialize all other registers in IF to UNDEFINED except PC)
			// IR = Mem[PC] (sp_registers[IF][IR])
			// NPC = PC +  (sp_registers[IF][NPC])
			// All other spregs UNDEFINED
			sp_registers[IF][PC] = sp_registers[MEM][PC];
			sp_registers[IF][NPC] = sp_registers[IF][PC] + 1;
			IReg[IF] = instr_memory[sp_registers[IF][PC]]; // IR array instruction type
			for(int i = 1; i < NUM_SP_REGISTERS; i++) sp_registers[IF][i] = UNDEFINED;
			
			
			//sp_registers[IF][NPC] = sp_registers[IF][PC] + 4; // might need to be +1?


    } // exits once current_cycle > cycles
}

//resets the state of the simulator
/* Note:
- registers should be reset to UNDEFINED value
- data memory should be reset to all 0xFF values
*/
void sim_pipe::reset(){
    //data_memory = new unsigned char[data_memory_size];
    for(int i = 0; i < data_memory_size; i++) data_memory[i] = 0xFF;
    for(int i = 0; i < NUM_STAGES; i++){
        for(int j = 0; j < NUM_SP_REGISTERS; j++)
        sp_registers[i][PC] = 0;
		sp_registers[i][NPC] = 0;
    }
	/*for(int i = 0; i < NUM_STAGES; i++){
	sp_registers[i][PC] = 3 - i;
	sp_registers[i][NPC] = 4 - i;
	}
	sp_registers[EXE][PC] = 0;
	sp_registers[EXE][NPC] = 1;*/
	//sp_registers[EXE][NPC] = sp_registers[ID][PC];
	instruction_t null_inst;
	null_inst.dest = 0;
	null_inst.src1 = 0;
	null_inst.src2 = 0;
	null_inst.opcode = NOP;
	null_inst.immediate = 0;
	null_inst.label = "";
	for(int i = 0; i < NUM_STAGES; i++) IReg[i] = null_inst;
	for(int i = 0; i < NUM_GP_REGISTERS; i++) gp_registers[i] = UNDEFINED;
}

//return value of special purpose register
unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
	return sp_registers[s][reg]; //please modify
}

//returns value of general purpose register
int sim_pipe::get_gp_register(unsigned reg){
	return gp_registers[reg]; //please modify
}

void sim_pipe::set_gp_register(unsigned reg, int value){
    gp_registers[reg] = value;
}

float sim_pipe::get_IPC(){
        return instructions_executed / (float)current_cycle; //please modify
}

unsigned sim_pipe::get_instructions_executed(){
        return instructions_executed; //please modify
}

unsigned sim_pipe::get_stalls(){
        return stalls; //please modify
}

unsigned sim_pipe::get_clock_cycles(){
        return current_cycle; //please modify
}



/* Test case for pipelined simuator */ 
/* DO NOT MODIFY */

int main(int argc, char **argv){
	cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	unsigned i, j;

	// instantiates the sim_pipe with a 1MB data memory
	sim_pipe *mips = new sim_pipe(1024*1024, 0);

	//loads program in instruction memory at address 0x10000000
	 mips->load_program("C:/Users/Smith/Desktop/NCSU/spring 23/ECE563/project/ece563-project-1/project_1/project1_code/c++/asm/no_dep.asm", 0x10000000);

	//initialize general purpose registers
	for (i=0; i<7; i++) mips->set_gp_register(i,i);

	//initialize data memory and prints its content (for the specified address ranges)
	for (i = 0x0, j=10; i<0x20; i+=4, j+=10) mips->write_memory(i,j);
	
	cout << "\nBEFORE PROGRAM EXECUTION..." << endl;
	cout << "======================================================================" << endl << endl;
	
	//prints the value of the memory and registers
	mips->print_registers();
	mips->print_memory(0x0, 0x20);

	// executes the program	
	cout << "\n*****************************" << endl;
	cout << "STARTING THE PROGRAM..." << endl;
	cout << "*****************************" << endl << endl;

	// first 8 clock cycles
	cout << "First 8 clock cycles: inspecting the registers at each clock cycle..." << endl;
	cout << "======================================================================" << endl << endl;

	for (i=0; i<8; i++){
		cout << "CLOCK CYCLE #" << dec << i << endl;
		mips->run(1);
		mips->print_registers();
		cout << endl;
	}

	// runs program to completion
	cout << "EXECUTING PROGRAM TO COMPLETION..." << endl << endl;
	mips->run(); 

	cout << "PROGRAM TERMINATED\n";
	cout << "===================" << endl << endl;

	//prints the value of registers and data memory
	mips->print_registers();
	mips->print_memory(0x0, 0x20);
	
	cout << endl;

	// prints the number of instructions executed and IPC
	cout << "Instruction executed = " << dec << mips->get_instructions_executed() << endl;
	cout << "Clock cycles = " << dec << mips->get_clock_cycles() << endl;
	cout << "Stall inserted = " << dec  << mips->get_stalls() << endl;
	cout << "IPC = " << dec << mips->get_IPC() << endl;

	delete mips;
}
