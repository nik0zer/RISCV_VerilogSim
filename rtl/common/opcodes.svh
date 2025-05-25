`ifndef OPCODES_SVH
`define OPCODES_SVH

// RISC-V Opcodes (instr[6:0])
`define OPCODE_LUI     7'b0110111
`define OPCODE_AUIPC   7'b0010111
`define OPCODE_JAL     7'b1101111
`define OPCODE_JALR    7'b1100111 // funct3 should be 000
`define OPCODE_BRANCH  7'b1100011 // BEQ, BNE, BLT, BGE, BLTU, BGEU
`define OPCODE_LOAD    7'b0000011 // LB, LH, LW, LBU, LHU (RV32I); LD, LWU (RV64I)
`define OPCODE_STORE   7'b0100011 // SB, SH, SW (RV32I); SD (RV64I)
`define OPCODE_I_ALU   7'b0010011 // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
`define OPCODE_R_ALU   7'b0110011 // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
// `define OPCODE_FENCE   7'b0001111 // Not implemented
// `define OPCODE_SYSTEM  7'b1110011 // ECALL, EBREAK, CSRxx // Not implemented

// Intermediate ALUOp from main_decoder to alu_decoder
`define ALUOP_TYPE_ADD     2'b00  // For operations that use ALU for address calculation (LW, SW, AUIPC, JAL, JALR) or pass-through (LUI)
`define ALUOP_TYPE_BRANCH  2'b01  // For branch operations (ALU typically performs SUB)
`define ALUOP_TYPE_R_I     2'b10  // For R-type and I-type ALU operations

// ImmSel output from main_decoder (to select immediate type for imm_gen unit)
// These correspond to the 'immsrc' input of your 'imm.sv' module
`define IMM_SEL_I   2'b00
`define IMM_SEL_S   2'b01
`define IMM_SEL_B   2'b10
`define IMM_SEL_J   2'b11

// ResultSrc output from main_decoder (selects what to write back to register file)
`define RESSRC_ALU   2'b00 // Result from ALU
`define RESSRC_MEM   2'b01 // Data from Memory
`define RESSRC_PC4   2'b10 // PC + 4 (for JAL, JALR link address)

`endif // OPCODES_SVH