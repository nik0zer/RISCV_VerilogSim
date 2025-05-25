#include "Vcontrol_unit.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>
#include <vector>
#include <string>

// Re-define opcodes and control signal values for C++ test
// Opcodes (matches opcodes.svh)
const uint8_t OPCODE_LUI     = 0b0110111;
const uint8_t OPCODE_AUIPC   = 0b0010111;
const uint8_t OPCODE_JAL     = 0b1101111;
const uint8_t OPCODE_JALR    = 0b1100111;
const uint8_t OPCODE_BRANCH  = 0b1100011;
const uint8_t OPCODE_LOAD    = 0b0000011;
const uint8_t OPCODE_STORE   = 0b0100011;
const uint8_t OPCODE_I_ALU   = 0b0010011;
const uint8_t OPCODE_R_ALU   = 0b0110011;

// ImmSel (matches opcodes.svh)
const uint8_t IMM_SEL_I = 0b00;
const uint8_t IMM_SEL_S = 0b01;
const uint8_t IMM_SEL_B = 0b10;
const uint8_t IMM_SEL_J = 0b11;

// ResultSrc (matches opcodes.svh)
const uint8_t RESSRC_ALU = 0b00;
const uint8_t RESSRC_MEM = 0b01;
const uint8_t RESSRC_PC4 = 0b10;

// ALU Operations (matches alu_defines.svh)
const uint8_t ALU_OP_ADD      = 0b000;
const uint8_t ALU_OP_SUB      = 0b001;
const uint8_t ALU_OP_AND      = 0b010;
const uint8_t ALU_OP_OR       = 0b011;
const uint8_t ALU_OP_XOR      = 0b100;
const uint8_t ALU_OP_SLT_BASE = 0b101;
const uint8_t ALU_OP_SLL      = 0b110;
const uint8_t ALU_OP_SR_BASE  = 0b111;

// ALU Modifier (matches alu_defines.svh)
const uint8_t ALU_SELECT_SIGNED     = 0;
const uint8_t ALU_SELECT_UNSIGNED   = 1;
const uint8_t ALU_SELECT_LOGICAL_SR = 0;
const uint8_t ALU_SELECT_ARITH_SR   = 1;


vluint64_t sim_time = 0;

double sc_time_stamp() {
    return sim_time;
}

void eval_cu(Vcontrol_unit* cu, VerilatedVcdC* tfp) {
    cu->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}

struct CU_TestCase {
    std::string name;
    uint8_t op_i;
    uint8_t funct3_i;
    uint8_t funct7_5_i;

    // Expected outputs
    bool    expected_RegWriteD;
    uint8_t expected_ResultSrcD; // 2 bits
    bool    expected_MemWriteD;
    bool    expected_JumpD;
    bool    expected_BranchD;
    bool    expected_ALUSrcD;
    uint8_t expected_ImmSelD;    // 2 bits
    bool    expected_Is_U_typeD;
    uint8_t expected_ALUControlD; // 3 bits
    bool    expected_ALUModifierD;
};

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vcontrol_unit* top = new Vcontrol_unit;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_control_unit.vcd");

    std::cout << "Starting Control Unit Testbench" << std::endl;

    std::vector<CU_TestCase> test_cases = {
        // name, op, f3, f7_5 | RegW, ResSrc, MemW, Jmp, Br, ALUSrc, ImmSel, IsU, ALUOp, ALUMdif
        {"LUI",   OPCODE_LUI,   0,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,true, ALU_OP_ADD,ALU_SELECT_SIGNED},
        {"AUIPC", OPCODE_AUIPC, 0,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,true, ALU_OP_ADD,ALU_SELECT_SIGNED},
        {"JAL",   OPCODE_JAL,   0,0,  true,RESSRC_PC4,false,true, false, true,IMM_SEL_J,false,ALU_OP_ADD,ALU_SELECT_SIGNED}, // ALUOp for JAL is ADD (PC+imm) by main_decoder
        {"JALR",  OPCODE_JALR,  0,0,  true,RESSRC_PC4,false,true, false, true,IMM_SEL_I,false,ALU_OP_ADD,ALU_SELECT_SIGNED},

        {"BEQ", OPCODE_BRANCH,0b000,0,false,RESSRC_ALU,false,false,true, false,IMM_SEL_B,false,ALU_OP_SUB,ALU_SELECT_SIGNED},

        {"LW",  OPCODE_LOAD,  0b010,0,  true,RESSRC_MEM,false,false,false, true,IMM_SEL_I,false,ALU_OP_ADD,ALU_SELECT_SIGNED},
        {"SW",  OPCODE_STORE, 0b010,0, false,RESSRC_ALU,true, false,false, true,IMM_SEL_S,false,ALU_OP_ADD,ALU_SELECT_SIGNED},

        {"ADDI",OPCODE_I_ALU, 0b000,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_ADD,ALU_SELECT_SIGNED},
        {"SLTI",OPCODE_I_ALU, 0b010,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_SLT_BASE,ALU_SELECT_SIGNED},
        {"SLTIU",OPCODE_I_ALU,0b011,0, true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_SLT_BASE,ALU_SELECT_UNSIGNED},
        {"XORI",OPCODE_I_ALU, 0b100,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_XOR,ALU_SELECT_SIGNED},
        {"ORI", OPCODE_I_ALU, 0b110,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_OR, ALU_SELECT_SIGNED},
        {"ANDI",OPCODE_I_ALU, 0b111,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_AND,ALU_SELECT_SIGNED},
        {"SLLI",OPCODE_I_ALU, 0b001,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_SLL,ALU_SELECT_SIGNED}, // funct7_5 for SLLI is 0
        {"SRLI",OPCODE_I_ALU, 0b101,0,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_SR_BASE,ALU_SELECT_LOGICAL_SR}, // funct7_5 for SRLI is 0
        {"SRAI",OPCODE_I_ALU, 0b101,1,  true,RESSRC_ALU,false,false,false, true,IMM_SEL_I,false,ALU_OP_SR_BASE,ALU_SELECT_ARITH_SR}, // funct7_5 for SRAI is 1

        {"ADD", OPCODE_R_ALU, 0b000,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_ADD,ALU_SELECT_SIGNED},
        {"SUB", OPCODE_R_ALU, 0b000,1,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_SUB,ALU_SELECT_SIGNED},
        {"SLL", OPCODE_R_ALU, 0b001,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_SLL,ALU_SELECT_SIGNED},
        {"SLT", OPCODE_R_ALU, 0b010,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_SLT_BASE,ALU_SELECT_SIGNED},
        {"SLTU",OPCODE_R_ALU, 0b011,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_SLT_BASE,ALU_SELECT_UNSIGNED},
        {"XOR", OPCODE_R_ALU, 0b100,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_XOR,ALU_SELECT_SIGNED},
        {"SRL", OPCODE_R_ALU, 0b101,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_SR_BASE,ALU_SELECT_LOGICAL_SR},
        {"SRA", OPCODE_R_ALU, 0b101,1,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_SR_BASE,ALU_SELECT_ARITH_SR},
        {"OR",  OPCODE_R_ALU, 0b110,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_OR, ALU_SELECT_SIGNED},
        {"AND", OPCODE_R_ALU, 0b111,0,  true,RESSRC_ALU,false,false,false,false,IMM_SEL_I,false,ALU_OP_AND,ALU_SELECT_SIGNED},
    };

    int passed_count = 0;
    for (const auto& tc : test_cases) {
        top->op_i = tc.op_i;
        top->funct3_i = tc.funct3_i;
        top->funct7_5_i = tc.funct7_5_i;
        eval_cu(top, tfp);

        bool current_pass = true;
        if (top->RegWriteD_o != tc.expected_RegWriteD) {std::cerr << " RegWriteD"; current_pass = false;}
        if (top->ResultSrcD_o != tc.expected_ResultSrcD) {std::cerr << " ResultSrcD"; current_pass = false;}
        if (top->MemWriteD_o != tc.expected_MemWriteD) {std::cerr << " MemWriteD"; current_pass = false;}
        if (top->JumpD_o != tc.expected_JumpD) {std::cerr << " JumpD"; current_pass = false;}
        if (top->BranchD_o != tc.expected_BranchD) {std::cerr << " BranchD"; current_pass = false;}
        if (top->ALUSrcD_o != tc.expected_ALUSrcD) {std::cerr << " ALUSrcD"; current_pass = false;}
        if (top->ImmSelD_o != tc.expected_ImmSelD) {std::cerr << " ImmSelD"; current_pass = false;}
        if (top->Is_U_typeD_o != tc.expected_Is_U_typeD) {std::cerr << " Is_U_typeD"; current_pass = false;}
        if (top->ALUControlD_o != tc.expected_ALUControlD) {std::cerr << " ALUControlD"; current_pass = false;}
        if (top->ALUModifierD_o != tc.expected_ALUModifierD) {std::cerr << " ALUModifierD"; current_pass = false;}

        if (current_pass) {
            passed_count++;
            std::cout << "PASS: " << tc.name << std::endl;
        } else {
            std::cout << "FAIL: " << tc.name << " Errors in:" << std::endl;
            std::cout << "  Inputs: op=0x" << std::hex << (int)tc.op_i
                      << " f3=0x" << (int)tc.funct3_i << " f7_5=" << (int)tc.funct7_5_i << std::dec << std::endl;
            std::cout << "  Outputs:       Got | Expected" << std::endl;
            std::cout << "  RegWriteD:     " << (int)top->RegWriteD_o   << " | " << (int)tc.expected_RegWriteD << std::endl;
            std::cout << "  ResultSrcD:    " << (int)top->ResultSrcD_o  << " | " << (int)tc.expected_ResultSrcD << std::endl;
            std::cout << "  MemWriteD:     " << (int)top->MemWriteD_o   << " | " << (int)tc.expected_MemWriteD << std::endl;
            std::cout << "  JumpD:         " << (int)top->JumpD_o       << " | " << (int)tc.expected_JumpD << std::endl;
            std::cout << "  BranchD:       " << (int)top->BranchD_o     << " | " << (int)tc.expected_BranchD << std::endl;
            std::cout << "  ALUSrcD:       " << (int)top->ALUSrcD_o     << " | " << (int)tc.expected_ALUSrcD << std::endl;
            std::cout << "  ImmSelD:       " << (int)top->ImmSelD_o     << " | " << (int)tc.expected_ImmSelD << std::endl;
            std::cout << "  Is_U_typeD:    " << (int)top->Is_U_typeD_o  << " | " << (int)tc.expected_Is_U_typeD << std::endl;
            std::cout << "  ALUControlD:   " << std::hex << (int)top->ALUControlD_o << " | " << (int)tc.expected_ALUControlD << std::dec << std::endl;
            std::cout << "  ALUModifierD:  " << (int)top->ALUModifierD_o<< " | " << (int)tc.expected_ALUModifierD << std::endl;
            assert(false); // Stop on first failure
        }
    }

    std::cout << "\nControl Unit Testbench Finished. Passed "
              << passed_count << "/" << test_cases.size() << " tests." << std::endl;

    if (tfp) {
        tfp->close();
    }
    delete top;
    return (passed_count == test_cases.size()) ? EXIT_SUCCESS : EXIT_FAILURE;
}