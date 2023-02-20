#ifndef SIM_PIPE_FP_H_
#define SIM_PIPE_FP_H_

#include <stdio.h>
#include <string>
#include <vector>	// for execution units

using namespace std;

#define PROGRAM_SIZE 50

#define UNDEFINED 0xFFFFFFFF //used to initialize the registers
#define NUM_SP_REGISTERS 11
#define NUM_GP_REGISTERS 32 // changed from 7->32 (didnt submit integer with 32 oops)
#define NUM_FP_REGISTERS 32
#define NUM_OPCODES 22 // added float opcodes
#define NUM_STAGES 5

typedef enum {PC = 0, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD, FP_A, FP_B} sp_register_t; // changed from -1 and -2

typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP, LWS, SWS, ADDS, SUBS, MULTS, DIVS} opcode_t;

typedef enum {IF, ID, EXE, MEM, WB} stage_t;

typedef enum {TYPE_R, TYPE_I, TYPE_J, TYPE_NOP, TYPE_F_MEM, TYPE_F_MATH} format_t;

typedef enum {INTEGER = 0, ADDER, MULTIPLIER, DIVIDER, UNDEF} exe_unit_t;

typedef struct{
        opcode_t opcode; //opcode
        unsigned src1; //first source register in the assembly instruction (for SW, register to be written to memory) (rs)
        unsigned src2; //second source register in the assembly instruction (rt)
        unsigned dest; //destination register (rd)
        unsigned immediate; //immediate field
        string label; //for conditional branches, label of the target instruction - used only for parsing/debugging purposes
} instruction_t;

typedef struct{
	instruction_t instruction;
	int busy = -1; // if == 0, unit is free. 
		// if instruction enters unit, counts down from (latency). 
		// if == 0 & instruction is present releases instruction. replaces with null_inst
	int latency = -1;
	unsigned cycle_instruction_entered_unit = -1; // what clock cycle did instruction enter the unit? used to resolve
	// conflicts where >1 instruction is ready for release AND to prevent WAW
	exe_unit_t type = UNDEF;
	float fp_output = UNDEFINED;
	int int_output = UNDEFINED;
} execution_unit_t;

//used for debugging purposes
/*static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "LWS", "SWS", "ADDS", "SUBS", "MULTS", "DIVS", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP"};
*/

class sim_pipe_fp{

	/* Add the data members required by your simulator's implementation here */

        // gp and sp registers (unsigned)
        int gp_registers[NUM_GP_REGISTERS];
		instruction_t IReg[NUM_STAGES];				 // holds IRs: IR[IF] points to next instruction to be fetched
															 // all others correspond to stage they feed into: IR[MEM] holds IR pipeline register at entrance to MEM stage
        int sp_registers[NUM_STAGES][NUM_SP_REGISTERS]; // IR is unused - needs to hold whole instruction
															 // get_sp_register needs to be altered to get correct value from IR array
															 // sp_registers[IF]][PC] holds pc value (beginning/end stage)
															 // other registers correspond to stage they feed into:
															 // stage between EX and MEM is sp_registers[MEM]
															 // stage between ID/EX is sp_registers[EX]
		float float_lmd;													 // if a SPR is unused by an instruction, it should be set to undefined
		float float_sval;

		float fp_registers[NUM_FP_REGISTERS];
		float fp_spregs[NUM_STAGES][NUM_SP_REGISTERS];

		vector<execution_unit_t> exec_units; // vector so # is configurable	

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

	// all these copied from integer
	unsigned local_stall_count; // resets to 0 for each hazard
	instruction_t null_inst;
	unsigned lastDest; // for RAW hazards
	bool stall_at_ID;
	bool stall_at_MEM;
	bool stall_at_EXE;
	bool skip_exe_id_if = false;
	unsigned mem_op_release_cycle;
	unsigned local_cycles; // for tracking executions in a run(int)

	// new
	unsigned num_units;

public:

	//instantiates the simulator with a data memory of given size (in bytes) and latency (in clock cycles)
	/* Note: 
           - initialize the registers to UNDEFINED value 
	   - initialize the data memory to all 0xFF values
	*/
	sim_pipe_fp(unsigned data_mem_size, unsigned data_mem_latency);
	
	//de-allocates the simulator
	~sim_pipe_fp();

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

	// my additions
	int get_int_register(unsigned reg);
	void set_int_register(unsigned reg, int value);
	float get_fp_register(unsigned reg);
	void set_fp_register(unsigned reg, float value);
	void decrement_units_busy_time();
	unsigned get_free_unit(opcode_t opcode);
	void init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances);
	void debug_units();

};

#endif /*SIM_PIPE_FP_H_*/
