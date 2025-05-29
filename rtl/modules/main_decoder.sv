`include "common/defines.svh"
`include "common/opcodes.svh"

module main_decoder (
    input  logic [6:0] op_i,
    input  logic [2:0] funct3_i,

    output logic       RegWrite_o,
    output logic [1:0] ResultSrc_o,
    output logic       MemWrite_o,
    output logic       Jump_o,         // For JAL, JALR (unconditional)
    output logic       Branch_o,       // For conditional branches
    output logic       ALUSrc_o,       // 0: ReadData2, 1: Immediate
    output logic [1:0] ImmSel_o,       // To select immediate type for imm_gen
    output logic       Is_U_type_o,    // To indicate LUI/AUIPC for special imm handling
    output logic [1:0] ALUOp_type_o,   // To alu_decoder
    output logic       mem_2_store
);

    always_comb begin
        RegWrite_o   = 1'b0;
        ResultSrc_o  = `RESSRC_ALU;
        MemWrite_o   = 1'b0;
        Jump_o       = 1'b0;
        Branch_o     = 1'b0;
        ALUSrc_o     = 1'b0; // Default to using register operand
        ImmSel_o     = `IMM_SEL_I; // Default, often overridden
        Is_U_type_o  = 1'b0;
        ALUOp_type_o = `ALUOP_TYPE_R_I; // Default
        mem_2_store  = 1'b0;

        case (op_i)
            `OPCODE_LUI: begin
                RegWrite_o   = 1'b1;
                ALUSrc_o     = 1'b1; // ALU uses immediate
                Is_U_type_o  = 1'b1; // Special U-type immediate
                ImmSel_o     = `IMM_SEL_I; // Actual ImmSel for imm_gen not used if Is_U_type=1
                ALUOp_type_o = `ALUOP_TYPE_ADD; // ALU will effectively pass U-imm (e.g. 0 + U-imm)
            end
            `OPCODE_AUIPC: begin
                RegWrite_o   = 1'b1;
                ALUSrc_o     = 1'b1; // ALU uses immediate
                Is_U_type_o  = 1'b1; // Special U-type immediate
                ImmSel_o     = `IMM_SEL_I; // Not used if Is_U_type=1
                ALUOp_type_o = `ALUOP_TYPE_ADD; // ALU performs PC + U-imm
            end
            `OPCODE_JAL: begin
                RegWrite_o   = 1'b1;
                ResultSrc_o  = `RESSRC_PC4;
                Jump_o       = 1'b1;
                ALUSrc_o     = 1'b1; // ALU needs PC + J-imm for target address (though PC logic might handle this)
                                     // For now, assume ALU is not involved in JAL target address calculation
                                     // but ImmSel is needed for the jump offset.
                ImmSel_o     = `IMM_SEL_J;
                ALUOp_type_o = `ALUOP_TYPE_ADD; // Not strictly for JAL's primary purpose if PC logic handles jump
            end
            `OPCODE_JALR: begin
                RegWrite_o   = 1'b1;
                ResultSrc_o  = `RESSRC_PC4;
                Jump_o       = 1'b1;
                ALUSrc_o     = 1'b1; // rs1 + I-imm
                ImmSel_o     = `IMM_SEL_I;
                ALUOp_type_o = `ALUOP_TYPE_ADD; // For rs1 + I-imm
            end
            `OPCODE_BRANCH: begin
                RegWrite_o   = 1'b0; // Branches do not write to rd
                Branch_o     = 1'b1;
                ALUSrc_o     = 1'b0; // ALU compares two registers
                ImmSel_o     = `IMM_SEL_B; // For branch offset
                ALUOp_type_o = `ALUOP_TYPE_BRANCH;
            end
            `OPCODE_LOAD: begin
                RegWrite_o   = 1'b1;
                ResultSrc_o  = `RESSRC_MEM;
                MemWrite_o   = 1'b0;
                ALUSrc_o     = 1'b1; // rs1 + I-imm for address
                ImmSel_o     = `IMM_SEL_I;
                ALUOp_type_o = `ALUOP_TYPE_ADD; // For address calculation
            end
            `OPCODE_STORE: begin
                RegWrite_o   = 1'b0; // Stores do not write to rd
                MemWrite_o   = 1'b1;
                ALUSrc_o     = 1'b1; // rs1 + S-imm for address
                ImmSel_o     = `IMM_SEL_S;
                ALUOp_type_o = `ALUOP_TYPE_ADD; // For address calculation
                if(funct3_i == `FUNC3_STORE16) begin
                    mem_2_store = 1'b1;
                end
            end
            `OPCODE_I_ALU: begin
                RegWrite_o   = 1'b1;
                ALUSrc_o     = 1'b1; // rs1 + I-imm
                ImmSel_o     = `IMM_SEL_I;
                ALUOp_type_o = `ALUOP_TYPE_R_I;
            end
            `OPCODE_R_ALU: begin
                RegWrite_o   = 1'b1;
                ALUSrc_o     = 1'b0; // rs1 + rs2
                ImmSel_o     = `IMM_SEL_I; // Not strictly used, but default
                ALUOp_type_o = `ALUOP_TYPE_R_I;
            end
            default: begin // Undefined or unimplemented opcodes
                RegWrite_o   = 1'b0;
                ResultSrc_o  = `RESSRC_ALU; // Default, but likely a NOP or error
                MemWrite_o   = 1'b0;
                Jump_o       = 1'b0;
                Branch_o     = 1'b0;
                ALUSrc_o     = 1'b0;
                ImmSel_o     = `IMM_SEL_I;
                Is_U_type_o  = 1'b0;
                ALUOp_type_o = `ALUOP_TYPE_R_I; // Could be an error signal
            end
        endcase
    end
endmodule