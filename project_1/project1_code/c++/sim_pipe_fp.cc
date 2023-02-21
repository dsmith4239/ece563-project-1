#include "C:\Users\Smith\Desktop\NCSU\spring 23\ECE563\project\ece563-project-1\project_1\project1_code\c++\sim_pipe_fp.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

//NOTE: structural hazards on MEM/WB stage not handled
//====================================================

//#define DEBUG
//#define DEBUG_MEMORY

using namespace std;

//used for debugging purposes/*
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP", "LWS", "SWS", "ADDS", "SUBS", "MULTS", "DIVS"};
static const char *unit_names[4]={"INTEGER", "ADDER", "MULTIPLIER", "DIVIDER"};

/* =============================================================

   HELPER FUNCTIONS

   ============================================================= */

/* converts array of char into integer - little indian */
inline unsigned char2int(unsigned char *buffer){
	unsigned d;
	memcpy(&d, buffer, sizeof d);
	return d;
}

/* convert a float into an unsigned */
inline unsigned float2unsigned_local(float value){
	unsigned result;
	memcpy(&result, &value, sizeof value);
	return result;
}

/* convert an unsigned into a float */
inline float unsigned_to_float(unsigned value){
	float result;
	memcpy(&result, &value, sizeof value);
	return result;
}

/* convert integer into array of unsigned char - little indian */
inline void unsigned2char(unsigned value, unsigned char *buffer){
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = (value >> 16) & 0xFF;
        buffer[3] = (value >> 24) & 0xFF;
}

/* convert array of char into integer - little indian */
inline unsigned char2unsigned(unsigned char *buffer){
       return buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
}

/* the following functions return the kind of the considered opcode */

bool is_branch(opcode_t opcode){
	return (opcode == BEQZ || opcode == BNEZ || opcode == BLTZ || opcode == BLEZ || opcode == BGTZ || opcode == BGEZ || opcode == JUMP);
}

bool is_memory(opcode_t opcode){
        return (opcode == LW || opcode == SW || opcode == LWS || opcode == SWS);
}

bool is_int_r(opcode_t opcode){
        return (opcode == ADD || opcode == SUB || opcode == XOR);
}

bool is_int_imm(opcode_t opcode){
        return (opcode == ADDI || opcode == SUBI);
}

bool is_int_alu(opcode_t opcode){
        return (is_int_r(opcode) || is_int_imm(opcode));
}

bool is_fp_alu(opcode_t opcode){
        return (opcode == ADDS || opcode == SUBS || opcode == MULTS || opcode == DIVS);
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
			case LWS:
			case SWS:
				return(a + imm);
			case BEQZ:
			case BNEZ:
			case BGTZ:
			case BGEZ:
			case BLTZ:
			case BLEZ:
			case JUMP:
				return(npc+imm);
			case ADDS:
				return(float2unsigned_local(unsigned_to_float(a)+unsigned_to_float(b)));
				break;
			case SUBS:
				return(float2unsigned_local(unsigned_to_float(a)-unsigned_to_float(b)));
				break;
			case MULTS:
				return(float2unsigned_local(unsigned_to_float(a)*unsigned_to_float(b)));
				break;
			case DIVS:
				return(float2unsigned_local(unsigned_to_float(a)/unsigned_to_float(b)));
				break;
			default:	
				return (-1);
	}
}

/* =============================================================

   CODE PROVIDED - NO NEED TO MODIFY FUNCTIONS BELOW

   ============================================================= */

/* ============== primitives to allocate/free the simulator ================== */

sim_pipe_fp::sim_pipe_fp(unsigned mem_size, unsigned mem_latency){
	data_memory_size = mem_size;
	data_memory_latency = mem_latency;
	data_memory = new unsigned char[data_memory_size];
	num_units = 0;

	null_inst.opcode = NOP;
	null_inst.dest = UNDEFINED;
	null_inst.src1 = UNDEFINED;
	null_inst.src2 = UNDEFINED;
	null_inst.immediate = UNDEFINED;

	reset();
}
	
sim_pipe_fp::~sim_pipe_fp(){
	delete [] data_memory;
}

/* =============   primitives to print out the content of the memory & registers and for writing to memory ============== */ 

void sim_pipe_fp::print_memory(unsigned start_address, unsigned end_address){
	cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	for (unsigned i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3){
#ifdef DEBUG_MEMORY 
			unsigned u = char2unsigned(&data_memory[i-3]);
			cout << " - unsigned=" << u << " - float=" << unsigned_to_float(u);
#endif
			cout << endl;
		}
	} 
}

void sim_pipe_fp::write_memory(unsigned address, unsigned value){
	unsigned2char(value,data_memory+address);
}


void sim_pipe_fp::print_registers(){
        cout << "Special purpose registers:" << endl;
        unsigned i, s;
        for (s=0; s<NUM_STAGES; s++){
                cout << "Stage: " << stage_names[s] << endl;
                for (i=0; i< NUM_SP_REGISTERS; i++)
                        if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
        }
        cout << "General purpose registers:" << endl;
        for (i=0; i< NUM_GP_REGISTERS; i++)
                if (get_int_register(i)!=(int)UNDEFINED) cout << "R" << dec << i << " = " << get_int_register(i) << hex << " / 0x" << get_int_register(i) << endl;
        for (i=0; i< NUM_GP_REGISTERS; i++)
                if (get_fp_register(i)!=UNDEFINED) cout << "F" << dec << i << " = " << get_fp_register(i) << hex << " / 0x" << float2unsigned_local(get_fp_register(i)) << endl;
}


/* =============   primitives related to the functional units ============== */ 

/* initializes an execution unit */ 
void sim_pipe_fp::init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances){
	for (unsigned i=0; i<instances; i++){
		execution_unit_t new_unit;
		
		new_unit.type = exec_unit;
		new_unit.latency = latency;
		new_unit.busy = 0;
		//new_unit.instruction.opcode = NOP;
		new_unit.instruction = null_inst;

		exec_units.push_back(new_unit);

		num_units++;
	}
}

/* returns a free unit for that particular operation or UNDEFINED if no unit is currently available */
unsigned sim_pipe_fp::get_free_unit(opcode_t opcode){
	if (num_units == 0){
		cout << "ERROR:: simulator does not have any execution units!\n";
		::exit(-1);
	}
	for (unsigned u=0; u<num_units; u++){
		switch(opcode){
			//Integer unit
			case LW:
			case SW:
			case ADD:
			case ADDI:
			case SUB:
			case SUBI:
			case XOR:
			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
			case JUMP:
			case LWS: 
			case SWS:
				if (exec_units[u].type==INTEGER && exec_units[u].busy==0) return u;
				break;
			// FP adder
			case ADDS:
			case SUBS:
				if (exec_units[u].type==ADDER && exec_units[u].busy==0) return u;
				break;
			// Multiplier
			case MULTS:
				if (exec_units[u].type==MULTIPLIER && exec_units[u].busy==0) return u;
				break;
			// Divider
			case DIVS:
				if (exec_units[u].type==DIVIDER && exec_units[u].busy==0) return u;
				break;
			default:
				cout << "ERROR:: operations not requiring exec unit!\n";
				::exit(-1);
		}
	}
	return UNDEFINED;
}

/* decrease the amount of clock cycles during which the functional unit will be busy - to be called at each clock cycle  */
void sim_pipe_fp::decrement_units_busy_time(){
	for (unsigned u=0; u<num_units; u++){
		if (exec_units[u].busy > 0) exec_units[u].busy --;
	}
}


/* prints out the status of the functional units */
void sim_pipe_fp::debug_units(){
	for (unsigned u=0; u<num_units; u++){
		cout << " -- unit " << unit_names[exec_units[u].type] << " latency=" << exec_units[u].latency << " busy=" << exec_units[u].busy <<
			" instruction=" << instr_names[exec_units[u].instruction.opcode] << endl;
	}
}

/* ========= end primitives related to functional units ===============*/


/* ========================parser ==================================== */

void sim_pipe_fp::load_program(const char *filename, unsigned base_address){

   /* initializing the base instruction address */
   instr_base_address = base_address;
   sp_registers[IF][PC] = instr_base_address;	// added
   /* creating a map with the valid opcodes and with the valid labels */
   map<string, opcode_t> opcodes; //for opcodes
   map<string, unsigned> labels;  //for branches
   for (int i=0; i<NUM_OPCODES; i++) // fails after 16
	 opcodes[string(instr_names[i])]=(opcode_t)i;

   /* opening the assembly file */
   ifstream fin(filename, ios::in | ios::binary);
   if (!fin.is_open()) {
      cerr << "error: open file " << filename << " failed!" << endl;
      ::exit(-1);
   }

   /* parsing the assembly file line by line */
   string line;
   unsigned instruction_nr = 0;
   while (getline(fin,line)){

	// set the instruction field
	char *str = const_cast<char*>(line.c_str());

  	// tokenize the instruction
	char *token = strtok (str," \t");
	//*token = strtok (str, "\r");
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
		case ADDS:
		case SUBS:
		case MULTS:
		case DIVS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "RF"));
			instr_memory[instruction_nr].src2 = atoi(strtok(par3, "RF"));
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
		case LWS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
			break;
		case SW:
		case SWS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "RF"));
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
		instr_memory[i].immediate = (labels[instr.label] - i - 1)<< 2;//  instructions are sequential in an array - should not be multiplied by 4
	}
        i++;
   }

}

/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */

/* simulator */

/* body of the simulator */
void sim_pipe_fp::run(unsigned cycles){ 
    local_cycles = 0;
	if(current_cycle == 0) sp_registers[IF][PC] = instr_base_address;// load pc
    while((cycles != 0 && local_cycles < cycles) or (cycles == 0)){
        if((cycles != 0 && local_cycles < cycles)) local_cycles++;
		// during each cycle:
		decrement_units_busy_time(); // lower busy times
        current_cycle++;        
		if(stall_at_ID || stall_at_MEM || skip_exe_id_if || stall_at_EXE) {
			//stalls++;
		}
		//if(stall_at_MEM) stalls++; //NEW 5


        // writeback: MEM/WB is written back (if necessary)
			if(!stall_at_MEM){
			
			IReg[WB] = IReg[MEM];
			for(int i = 0; i < NUM_SP_REGISTERS; i++) {
				sp_registers[WB][i] = sp_registers[MEM][i];
				fp_spregs[WB][i] = fp_spregs[MEM][i];
			}
			switch(IReg[WB].opcode){
				case EOP: return; // exit at EOP (but make sure all other instructions have written back)
				break;
				// if ALU: regs[destination] = ALU output (sp_registers[WB][ALU_OUTPUT])
				case ADD:set_int_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				case SUB:set_int_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				case XOR:set_int_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				// if ALU with immediate: regs[rt] = ALU output (sp_registers[WB][ALU_OUTPUT])
				case ADDI:set_int_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				case SUBI:set_int_register(IReg[WB].dest,sp_registers[WB][ALU_OUTPUT]);
				break;
				// if load: regs[rt] = LMD (load memory data) (sp_registers[WB][LMD])
				case LW: set_int_register(IReg[WB].dest,sp_registers[WB][LMD]);
				case LWS: set_fp_register(IReg[WB].dest,float_lmd);	// FPP added LWS
				break;
				case ADDS: 
				case SUBS: 
				case MULTS: 
				case DIVS: set_fp_register(IReg[WB].dest,fp_spregs[WB][ALU_OUTPUT]); break;
			}
			}
			else{ 
				IReg[WB] = null_inst;
				for(int i = 0; i < NUM_SP_REGISTERS; i++){
				fp_spregs[WB][i] = UNDEFINED;
				sp_registers[WB][i] = UNDEFINED;
				}
			}
		




        // memory operations: EX/MEM

		/*
		memory latency: operations in MEM take L+1 cycles to complete
		if latency != 0:
		mem_op_release_cycle = get cycles
		if cycle == mem_op_release_cycle + latency -> execute
		else wait and pause all operations (stall_at_mem) (only need to replace WB with nop)
		if(!stall_at_mem) -> normal stage operations, if(stall_at_mem) do nothing
		*/
		if(stall_at_MEM){ // memory is currently busy: check if stall is released
		//stalls++;
		if(get_clock_cycles() == mem_op_release_cycle || data_memory_latency == 0){	// if operation takes place this cycle, take normal action
			mem_op_release_cycle = UNDEFINED;
			stall_at_MEM = false;
			//stalls--;
			IReg[MEM] = IReg[EXE];
			for(int i = 0; i < NUM_SP_REGISTERS; i++) {sp_registers[MEM][i] = sp_registers[EXE][i];fp_spregs[MEM][i] = fp_spregs[EXE][i];} //
			// PC = NPC
			//sp_registers[IF][PC] = sp_registers[MEM][NPC]; // update PC with propagated value or 
			if(sp_registers[MEM][COND] == 1) {
				// 	branch taken
				sp_registers[MEM][PC] = sp_registers[MEM][ALU_OUTPUT];
				//	flush instructions currently in EXE, ID to NOP
				IReg[EXE] = null_inst;
				IReg[ID] = null_inst;
				// update PC in IFetch
				//	if(sp_registers[MEM][COND] == 1) [IF][PC] == sp_registers[MEM][PC]
			}
			else sp_registers[MEM][PC] = sp_registers[MEM][NPC];
			switch(IReg[MEM].opcode){
				// If load: LMD = Mem[ALU output]
				// If store: Mem[ALU output] = B
				// FPP added LWS/SWS
				case LW: sp_registers[MEM][LMD] = char2int(&data_memory[sp_registers[MEM][ALU_OUTPUT]]);//data_memory[sp_registers[MEM][ALU_OUTPUT]]; LAST THING I CHANGED 
				break;
				case SW: write_memory(sp_registers[MEM][ALU_OUTPUT],sp_registers[MEM][A]);
				break;
				case LWS: //sp_registers[MEM][LMD] = char2int(&data_memory[sp_registers[MEM][ALU_OUTPUT]]);//data_memory[sp_registers[MEM][ALU_OUTPUT]]; LAST THING I CHANGED 
				float_lmd = unsigned_to_float((unsigned)char2int(&data_memory[sp_registers[MEM][ALU_OUTPUT]]));
				break;
				case SWS: write_memory(sp_registers[MEM][ALU_OUTPUT],float2unsigned_local(fp_spregs[MEM][A]));	// store that reg in mem
				break;
			}
		}

		} else if(IReg[EXE].opcode == LW || IReg[EXE].opcode == SW || IReg[EXE].opcode == LWS || IReg[EXE].opcode == SWS) { // memory is not busy - start request and set release cycle to current cycle + latency
			// FPP added LWS/SWS 
			IReg[MEM] = IReg[EXE];

			// passing instruction through
			// this might cause issues if we try to pass LW instruction in if exec unit is done same cycle
			// prioritize exec unit over L/S if so

			for(int i = 0; i < NUM_SP_REGISTERS; i++) {sp_registers[MEM][i] = sp_registers[EXE][i];fp_spregs[MEM][i] = fp_spregs[EXE][i];}
			mem_op_release_cycle = get_clock_cycles() + data_memory_latency;//+ 2;
			stall_at_MEM = true;
			//stalls++;
		}else{// just passin through :)
			
			// NEEDS TO CHANGE TO GET FROM COMPLETED UNIT - IF NO UNITS COMPLETED, GENERATE NOP
			// for all units
			// check if current unit is not busy & opcode is not nop
			// store in IReg[MEM]
			// check all other instructions to see if there is another instruction that's done & is older. replace if necessary
			
			unsigned earliest_cycle = UNDEFINED;
			unsigned chosen_unit = UNDEFINED;
			for(int i = 0; i < exec_units.size(); i++){
				execution_unit_t current_unit = exec_units.at(i);
				if(current_unit.busy == 0 && current_unit.instruction.opcode != NOP && current_unit.cycle_instruction_entered_unit < earliest_cycle){
					for(int i = 0; i < NUM_SP_REGISTERS; i++) {sp_registers[MEM][i] = sp_registers[EXE][i];fp_spregs[MEM][i] = fp_spregs[EXE][i];}
					// copy sp regs to mem
					// get instruction and result from this unit
					IReg[MEM] = current_unit.instruction;
					if(current_unit.type) fp_spregs[MEM][ALU_OUTPUT] = current_unit.fp_output;	// integer type == 0, all others are float types
					else sp_registers[MEM][ALU_OUTPUT] = current_unit.int_output;
					chosen_unit = i; // this unit is the one getting pulled from exe
					skip_exe_id_if = false;
					IReg[EXE] = null_inst;
					instructions_executed++;
				}

			}

			// clear chosen unit's instruction after result has been pulled out. 
			//after any unit release, check if instruction in EXE is waiting on that type.
			// if(get_free_unit != undefined), release stall. 
			if(chosen_unit != UNDEFINED){
				exec_units.at(chosen_unit).instruction = null_inst;
				if(IReg[EXE].opcode != NOP && get_free_unit(IReg[EXE].opcode)) stall_at_EXE = false; // might need to be ID? FP1
			}

			// if no units are done & next instruction is arithmetic, NOP
			// if non arithmetic instruction, pass (FPP Hazards?)
			if(chosen_unit == UNDEFINED){
				for(int i = 0; i < NUM_SP_REGISTERS; i++) sp_registers[MEM][i] = UNDEFINED;
				IReg[MEM] = null_inst;
				for(int i = 0; i < num_units; i++){
					if(exec_units.at(i).busy == 1) {skip_exe_id_if = true;stalls++;} 	// instruction is coming out of unit next turn 
																			// don't pass instruction at exe into mem
				}
				if(
					//IReg[EXE].opcode != ADD &&
					IReg[EXE].opcode != ADDS &&
					//IReg[EXE].opcode != ADDI &&
					//IReg[EXE].opcode != SUB &&
					//IReg[EXE].opcode != SUBI &&
					IReg[EXE].opcode != SUBS &&
					IReg[EXE].opcode != MULTS &&
					IReg[EXE].opcode != DIVS &&	// if the instruction is ready to be passed into mem and an instruction is not coming out next cycle				
					!skip_exe_id_if
				){
					IReg[MEM] = IReg[EXE];
					for(int i = 0; i < NUM_SP_REGISTERS; i++) {
						{sp_registers[MEM][i] = sp_registers[EXE][i];
						fp_spregs[MEM][i] = fp_spregs[EXE][i];}
					}
				}
			}


			if(sp_registers[MEM][COND] == 1) {
				// 	branch taken
				sp_registers[MEM][PC] = sp_registers[MEM][ALU_OUTPUT];
				//	flush instructions currently in EXE, ID to NOP
				IReg[EXE] = null_inst;
				IReg[ID] = null_inst;
				IReg[IF] = null_inst; // NEW 6
				// update PC in IFetch
				//	if(sp_registers[MEM][COND] == 1) [IF][PC] == sp_registers[MEM][PC]
			}
			else sp_registers[MEM][PC] = sp_registers[MEM][NPC];
		
		}




        // execute: ID/EX
			//decrement_units_busy_time();
			if(!stall_at_MEM){	// only execute lower stages if memory is not busy - otherwise, whole pipeline waits
			
			// check for stalls (if every unit is empty, go ahead)
			stall_at_EXE = false;
			for(int i = 0; i < exec_units.size(); i++){
					if(exec_units.at(i).busy != 0) {stall_at_EXE = true;stalls++;}
				}
			//if(get_free_unit(IReg[ID].opcode) != UNDEFINED) stall_at_EXE = false;
			//else stall_at_EXE = tre;
			
			if(!stall_at_EXE){
				// if an instruction is about to come out of unit, stall for one cycle
				/*for(int i = 0; i < num_units; i++){
					if(exec_units.at(i).busy == 0) skip_exe_id_if = true;
				}*/

				// check if any unit is busy - do not push eop if so
				for(int i = 0; i < exec_units.size(); i++){
					if(exec_units.at(i).busy != 0) pause_eop_exe = true;
				}
				if(!skip_exe_id_if && !pause_eop_exe){
					for(int i = 0; i < NUM_SP_REGISTERS; i++) {
						sp_registers[EXE][i] = sp_registers[ID][i];
						fp_spregs[EXE][i] = fp_spregs[ID][i];
					}
				IReg[EXE] = IReg[ID];

				// just passed instruction in: look at next ID (currently in IF)
				// to see if we need to stall 


				}
				else{// instruction is about to come out of execution unit: nop this ?

				}
			} // skip pulling if instruction is already waiting

			// currently stalled - check to release stall
			if(exe_stall_release_cycle <= current_cycle){	// FP1
				stall_at_EXE = false;
			}


			//sp_registers[EXE][ALU_OUTPUT] = alu(IReg[EXE].opcode, sp_registers[EXE][A], sp_registers[EXE][B], IReg[EXE].immediate, sp_registers[EXE][NPC]);
			//if(!stall_at_ID && IReg[EXE].opcode != NOP)instructions_executed++;
			if(!stall_at_EXE){
			switch(IReg[EXE].opcode){
				case NOP: break;
				case LWS: sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][A] + sp_registers[EXE][IMM];break;
				case SWS: sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][B] + sp_registers[EXE][IMM];break;
				case ADDS: {
					int f = get_free_unit(ADDS);
					if(f != UNDEFINED){ 
					// if free unit, push

						// FP1
						// there might be a divide or mult waiting for this instruction already:
						// if the divider's instruction's destination is this 
						// instruction's destination, stall
						// stall = true, stall release cycle = current cycle + divider latency + 3
						for(int i = 0; i < exec_units.size(); i++){
							if((exec_units.at(i).type == DIVIDER || exec_units.at(i).type == MULTIPLIER) && exec_units.at(i).instruction.dest == IReg[EXE].dest){
								stall_at_EXE = true;
								stalls++;
								exe_stall_release_cycle = get_clock_cycles() + exec_units.at(i).latency + 3;
								if(exec_units.at(i).type == DIVIDER) break;	// stop looking if we have the max delay
							}
						}



						if(!stall_at_EXE){
						exec_units[f].busy = exec_units[f].latency;
						exec_units[f].instruction = IReg[EXE];
						exec_units[f].fp_output = fp_spregs[EXE][FP_A] + fp_spregs[EXE][FP_B]; // expression
						exec_units[f].cycle_instruction_entered_unit = current_cycle;

						// pull instruction out of IReg
						//IReg[EXE] = null_inst;

						}
					}// else stall at EXE
					else {stall_at_EXE = true;stalls++;}
				/*
				push to unit. 
				
				sets busy high & starts countdown. 
				every exe cycle, decrement all countdowns 
				if >0 and release instructions that are busy
				(if busy && 0 pass to memory)

				*/
				break;}
				case SUBS: {
					int f = get_free_unit(SUBS);
					if(f != UNDEFINED){ 
					// if free unit, push


						// there might be a divide or mult waiting for this instruction already:
						// if the divider's instruction's destination is this 
						// instruction's destination, stall
						// stall = true, stall release cycle = current cycle + divider latency + 3
						for(int i = 0; i < exec_units.size(); i++){
							if((exec_units.at(i).type == DIVIDER || exec_units.at(i).type == MULTIPLIER) && exec_units.at(i).instruction.dest == IReg[EXE].dest){
								stall_at_EXE = true;
								stalls++;
								exe_stall_release_cycle = get_clock_cycles() + exec_units.at(i).latency + 3;
								if(exec_units.at(i).type == DIVIDER) break;	// stop looking if we have the max delay
							}
						}




						if(!stall_at_EXE){
						exec_units[f].busy = exec_units[f].latency;
						exec_units[f].instruction = IReg[EXE];
						exec_units[f].fp_output = fp_spregs[EXE][FP_A] - fp_spregs[EXE][FP_B]; 
						exec_units[f].cycle_instruction_entered_unit = current_cycle;

						// pull instruction out of IReg
						//IReg[EXE] = null_inst;

						}
					}// else stall at EXE
					else {stall_at_EXE = true;stalls++;}
				break;}
				case MULTS: {
					int f = get_free_unit(MULTS);
					if(f != UNDEFINED){ 
					// if free unit, push
						
						// there might be a divide waiting for this instruction already:
						// if the divider's instruction's destination is this 
						// instruction's destination, stall
						// stall = true, stall release cycle = current cycle + divider latency + 3
						for(int i = 0; i < exec_units.size(); i++){
							if(exec_units.at(i).type == DIVIDER && exec_units.at(i).instruction.dest == IReg[EXE].dest){
								stall_at_EXE = true;
								stalls++;
								exe_stall_release_cycle = get_clock_cycles() + exec_units.at(i).latency + 3;
							}
						}
						
						if(!stall_at_EXE){
						// otherwise
						exec_units[f].busy = exec_units[f].latency;
						exec_units[f].instruction = IReg[EXE];
						exec_units[f].fp_output = fp_spregs[EXE][FP_A] * fp_spregs[EXE][FP_B];
						exec_units[f].cycle_instruction_entered_unit = current_cycle;

						// pull instruction out of IReg
						//IReg[EXE] = null_inst;
						}
					}// else stall at EXE
					else {stall_at_EXE = true;stalls++;}
				break;}
				case DIVS: {
					int f = get_free_unit(DIVS);
					if(f != UNDEFINED){ 
					// if free unit, push
						exec_units[f].busy = exec_units[f].latency;
						exec_units[f].instruction = IReg[EXE];
						exec_units[f].fp_output = fp_spregs[EXE][FP_A] / fp_spregs[EXE][FP_B]; 
						exec_units[f].cycle_instruction_entered_unit = current_cycle;
						// pull instruction out of IReg
						//IReg[EXE] = null_inst;
					}// else stall at EXE
					else {stall_at_EXE = true;stalls++;}
				break;}
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
					sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][B] + sp_registers[EXE][IMM];
					
				break;
				case XOR:
					sp_registers[EXE][ALU_OUTPUT] =  sp_registers[EXE][A] ^ sp_registers[EXE][B];
					
				break;
				case BEQZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM] << 2);
					sp_registers[EXE][COND] = (sp_registers[EXE][A] == 0);
					
				break;
				case BNEZ:
					sp_registers[EXE][ALU_OUTPUT] = sp_registers[EXE][NPC] + (sp_registers[EXE][IMM]); //<< 2); //produces wrong results with << 2?
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
			}
			// if [EXE][COND], need to clear pipeline

			// If ALU: ALU out = A op B
			// If ALU(Imm): ALU out = A op Imm
			// If memory reference: ALU out = A + Imm
			// Else (if branch) ALU out = NPC + (Imm << 2)
			// Cond = (A == 0)
			








		if(!stall_at_EXE){ // only execute these stages if we are not stalled at exe (waiting for exec unit)
         // decode: IF/ID
			// pass registers through if not stalled. if stalled, generate nops NEW
			if((IReg[MEM].opcode != NOP && IReg[MEM].opcode != SWS && IReg[MEM].opcode != SW && IReg[MEM].opcode != EOP) && (IReg[MEM].dest == IReg[IF].src1 || IReg[MEM].dest == IReg[IF].src2)) {
				if(IReg[MEM].opcode < 7 || IReg[MEM].opcode > 16){//FPP
				stall_at_ID = true;// no stall on nops: null instructions are all UNDEFINED
					//stalls++;
				}
			}
			// RAW block: if src1 or src2 == lastDest, stall
			lastDest = IReg[IF].dest;
			if(stall_at_ID){
				// stall behavior - no fetch. increment stall_count, if == NUM_DEPENDENCY STALLS
				// then release stall_at_ID.
				// Reset stall_count to 0 at every detection
				local_stall_count++;
				
				if(local_stall_count == 3) {
					//stalls++;
					stall_at_ID = false;// magic number
					local_stall_count = 0;
				}
				
				//cout << "STALLED!"; // remove
			}	// MOVED STALL CHECK BLOCK TO ID
			if(stall_at_ID){
				// stall behavior: generate nops
				IReg[ID] = null_inst;
			}else{ // normal decode
				
			for(int i = 1; i < NUM_SP_REGISTERS; i++) {sp_registers[ID][i] = sp_registers[IF][i];fp_spregs[ID][i] = fp_spregs[IF][i];}
			if(IReg[IF].opcode != 0xbaadf00d) IReg[ID] = IReg[IF];
			
			
			//sp_registers[ID][A] = gp_registers[IReg[ID].src1];
			format_t format;
			if(IReg[ID].opcode == EOP || IReg[ID].opcode == NOP) format = TYPE_NOP;
			else if(IReg[ID].opcode == ADD || IReg[ID].opcode == SUB || IReg[ID].opcode == XOR) format = TYPE_R;
			else if(IReg[ID].opcode == JUMP) format = TYPE_J;
			else if(IReg[ID].opcode >= LW && IReg[ID].opcode <= JUMP ) format = TYPE_I; // branches, ALU immediates, load/store
			else if(IReg[ID].opcode == LWS || IReg[ID].opcode == SWS) format = TYPE_F_MEM;
			else if(IReg[ID].opcode == ADDS ||IReg[ID].opcode == DIVS || IReg[ID].opcode == SUBS ||IReg[ID].opcode == MULTS) format = TYPE_F_MATH;
			switch(format){
				case TYPE_NOP: break;

				case TYPE_F_MATH: 
				fp_spregs[ID][FP_A] = fp_registers[IReg[ID].src1];
				fp_spregs[ID][FP_B] = fp_registers[IReg[ID].src2];
				break;

				case TYPE_F_MEM:	// store/load fpA at offset with B/imm
				sp_registers[ID][A] = gp_registers[IReg[ID].src1]; // store: put reg at src1 into mem
				//if(IReg[ID].opcode == SWS)
				sp_registers[ID][IMM] = IReg[ID].immediate;
				if(IReg[ID].opcode == SWS){
					fp_spregs[ID][A] = fp_registers[IReg[ID].src1];
					// offset from gp registers[src2] into sp registers id b
					sp_registers[ID][B] = gp_registers[IReg[ID].src2];
					// immediate
					
				}
				break;
				case TYPE_R:
				//sp_registers[ID][] = IReg[ID].dest;
				sp_registers[ID][A] = gp_registers[IReg[ID].src1];
				sp_registers[ID][B] = gp_registers[IReg[ID].src2];
				break;
				case TYPE_I:
				//sp_registers[ID][] = IReg[ID].dest;
				sp_registers[ID][A] = gp_registers[IReg[ID].src1];
				if(IReg[ID].opcode == SW)sp_registers[ID][B] = gp_registers[IReg[ID].src2];
				sp_registers[ID][IMM] = IReg[ID].immediate;
				if(IReg[ID].opcode == BNEZ) sp_registers[ID][IMM] += 16;
				break;
				case TYPE_J:
				sp_registers[ID][IMM] = IReg[ID].immediate;
				break;
			}
			//if(IReg[ID].opcode != SW && IReg[ID].opcode != LW) sp_registers[ID][B] = gp_registers[IReg[ID].src2]; // do not fill if instruction is LW/SW
			if(IReg[ID].opcode != BNEZ) sp_registers[ID][IMM] = IReg[ID].immediate;
			}















         // fetch: next instruction (initialize all other registers in IF to UNDEFINED except PC) 
			if(sp_registers[MEM][COND] == 1) {
				sp_registers[IF][PC] = sp_registers[MEM][PC] - 4;
			}
			sp_registers[IF][NPC] = sp_registers[IF][PC] + 4;
			if(!stall_at_ID){ // 2 blocks so if we come out of stall we can advance in the same CC
				// if(sp_registers[MEM][COND] == 1) sp_registers[IF][PC] = sp_registers[MEM][PC] - 1; // overwrite with new PC if branch taken in MEM stage
				// sp_registers[IF][NPC] = sp_registers[IF][PC] + 1;
				IReg[IF] = instr_memory[(sp_registers[IF][PC] - instr_base_address) / 4]; // IR array instruction type
				sp_registers[IF][PC]+=4;
				if(IReg[IF].immediate == 0xbaadf00d) IReg[IF].immediate = UNDEFINED;
				for(int i = 2; i < NUM_SP_REGISTERS; i++) sp_registers[IF][i] = UNDEFINED;
				for(int i = 0; i < NUM_SP_REGISTERS; i++) fp_spregs[IF][i] = UNDEFINED;
				// RAW: if new instruction src1 or src2 wants to access value of last destination register, stall				
				if((lastDest == IReg[IF].src1 || lastDest == IReg[IF].src2) && lastDest != UNDEFINED && lastDest != 0xbaadf00d) {
					stall_at_ID = true;
					//stalls++;
				}
			
			}
			if(!stall_at_ID) sp_registers[IF][NPC]+= 4; // NEW
			// check for RAW stall again
			//if(IReg[ID].dest == IReg[IF].src1 || IReg[ID].dest == IReg[IF].src2) stall_at_ID = true;

			//sp_registers[IF][NPC] = sp_registers[IF][PC] + 4; // might need to be +1?
		}

    } else{
		// exe is stalled at memory: nop
		/*IReg[EXE] = null_inst;
		for(int j = 0; j < NUM_SP_REGISTERS; j++){
			sp_registers[EXE][j] = UNDEFINED;
			fp_spregs[EXE][j] = UNDEFINED;
		}*/
	} 
	
	// exits once current_cycle > cycles
	// once program has run to completion:
	
	//if(data_memory_latency == 0) stalls = 0;
}

}

	
//reset the state of the sim_pipe_fpulator
void sim_pipe_fp::reset(){
	// init data memory
	for (unsigned i=0; i<data_memory_size; i++) data_memory[i]=0xFF;
	// init instruction memory
	for (int i=0; i<PROGRAM_SIZE;i++){
		instr_memory[i].opcode=(opcode_t)NOP;
		instr_memory[i].src1=UNDEFINED;
		instr_memory[i].src2=UNDEFINED;
		instr_memory[i].dest=UNDEFINED;
		instr_memory[i].immediate=UNDEFINED;
	}

	/* complete the reset function here */    
	for(int i = 0; i < NUM_STAGES; i++){
		for(int j = 0; j < NUM_SP_REGISTERS; j++){
			sp_registers[i][j] = UNDEFINED;
			fp_spregs[i][j] = UNDEFINED;
		}
	}
	//sp_registers[IF][PC] = instr_base_address;

	for(int i = 0; i < NUM_STAGES; i++) IReg[i] = null_inst;
	for(int i = 0; i < NUM_GP_REGISTERS; i++) gp_registers[i] = UNDEFINED;
	for(int i = 0; i < NUM_FP_REGISTERS; i++) fp_registers[i] = UNDEFINED;
	current_cycle = 0;

	stall_at_ID = false;
	stall_at_MEM = false;
	stall_at_EXE = false;

	local_stall_count = 0;
}

//return value of special purpose register
unsigned sim_pipe_fp::get_sp_register(sp_register_t reg, stage_t s){
	return sp_registers[s][reg]; // please modify
}

int sim_pipe_fp::get_int_register(unsigned reg){
	return gp_registers[reg]; // please modify
}

void sim_pipe_fp::set_int_register(unsigned reg, int value){
	gp_registers[reg] = value;
}

float sim_pipe_fp::get_fp_register(unsigned reg){
	return fp_registers[reg]; // please modify
}

void sim_pipe_fp::set_fp_register(unsigned reg, float value){
	fp_registers[reg] = value;
}


float sim_pipe_fp::get_IPC(){
	return (float)get_instructions_executed() / get_clock_cycles(); // please modify
}

unsigned sim_pipe_fp::get_instructions_executed(){
	return instructions_executed; // please modify
}

unsigned sim_pipe_fp::get_clock_cycles(){
	return current_cycle; // please modify
}

unsigned sim_pipe_fp::get_stalls(){
	return stalls; // please modify
}
