`include "common/defines.svh"
`include "common/opcodes.svh"
`include "common/alu_defines.svh"

module alu_decoder (
    input  logic [6:0] op_i,           // Full opcode
    input  logic [1:0] ALUOp_type_i,   // From main_decoder
    input  logic [2:0] funct3_i,
    input  logic       funct7_5_i,       // Corresponds to instr[30]

    output logic [2:0] ALUControl_o,   // To ALU module
    output logic       ALUModifier_o   // To ALU module (for SLT vs SLTU, SRA vs SRL)
);

    always_comb begin
        ALUControl_o  = `ALU_OP_ADD;       // Default operation
        ALUModifier_o = `ALU_SELECT_SIGNED; // Default modifier (signed for SLT, logical for SR)

        case (ALUOp_type_i)
            `ALUOP_TYPE_ADD: begin // Used by LW, SW, AUIPC, LUI, JALR, JAL(if ALU computes target)
                ALUControl_o = `ALU_OP_ADD;
            end
            `ALUOP_TYPE_BRANCH: begin // Used by BEQ, BNE, etc.
                ALUControl_o = `ALU_OP_SUB; // Branches use subtraction for comparison
            end
            `ALUOP_TYPE_R_I: begin // R-type and I-type ALU operations
                case (funct3_i)
                    3'b000: begin // ADD, ADDI, SUB
                        if (op_i == `OPCODE_R_ALU && funct7_5_i) begin // SUB (R-type only)
                            ALUControl_o = `ALU_OP_SUB;
                        end else begin // ADD (R-type) or ADDI (I-type)
                            ALUControl_o = `ALU_OP_ADD;
                        end
                    end
                    3'b001: begin // SLL, SLLI
                        ALUControl_o = `ALU_OP_SLL;
                    end
                    3'b010: begin // SLT, SLTI
                        ALUControl_o  = `ALU_OP_SLT_BASE;
                        ALUModifier_o = `ALU_SELECT_SIGNED;
                    end
                    3'b011: begin // SLTU, SLTIU
                        ALUControl_o  = `ALU_OP_SLT_BASE;
                        ALUModifier_o = `ALU_SELECT_UNSIGNED;
                    end
                    3'b100: begin // XOR, XORI
                        ALUControl_o = `ALU_OP_XOR;
                    end
                    3'b101: begin // SRL, SRLI, SRA, SRAI
                        ALUControl_o = `ALU_OP_SR_BASE;
                        // For R-type (SRA/SRL) and I-type (SRAI/SRLI),
                        // instr[30] (funct7_5_i) distinguishes arithmetic vs logical for sr*
                        if (funct7_5_i) begin // SRA or SRAI
                            ALUModifier_o = `ALU_SELECT_ARITH_SR;
                        end else begin // SRL or SRLI
                            ALUModifier_o = `ALU_SELECT_LOGICAL_SR;
                        end
                    end
                    3'b110: begin // OR, ORI
                        ALUControl_o = `ALU_OP_OR;
                    end
                    3'b111: begin // AND, ANDI
                        ALUControl_o = `ALU_OP_AND;
                    end
                    default: begin // Should not be reached with valid funct3
                        ALUControl_o  = `ALU_OP_ADD; // Default to ADD
                    end
                endcase
            end
            default: begin // Should not be reached if ALUOp_type_i is always valid
                ALUControl_o = `ALU_OP_ADD;
            end
        endcase
    end
endmodule