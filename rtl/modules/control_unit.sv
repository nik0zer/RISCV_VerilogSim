`include "common/defines.svh"

module control_unit (
    // Inputs from instruction word (ID stage)
    input  logic [6:0] op_i,         // Opcode instr[6:0]
    input  logic [2:0] funct3_i,     // instr[14:12]
    input  logic       funct7_5_i,   // instr[30]

    // Control Signals for ID/EX pipeline register (and other units in ID)
    output logic       RegWriteD_o,
    output logic [1:0] ResultSrcD_o,
    output logic       MemWriteD_o,
    output logic       JumpD_o,
    output logic       BranchD_o,
    output logic       ALUSrcD_o,
    output logic [1:0] ImmSelD_o,    // To select immediate type for imm_gen
    output logic       Is_U_typeD_o, // To indicate LUI/AUIPC for special imm handling
    output logic [2:0] ALUControlD_o,
    output logic       ALUModifierD_o
);

    logic [1:0] alu_op_type_w; // Wire between main_decoder and alu_decoder

    main_decoder main_dec_inst (
        .op_i(op_i),

        .RegWrite_o(RegWriteD_o),
        .ResultSrc_o(ResultSrcD_o),
        .MemWrite_o(MemWriteD_o),
        .Jump_o(JumpD_o),
        .Branch_o(BranchD_o),
        .ALUSrc_o(ALUSrcD_o),
        .ImmSel_o(ImmSelD_o),
        .Is_U_type_o(Is_U_typeD_o),
        .ALUOp_type_o(alu_op_type_w)
    );

    alu_decoder alu_dec_inst (
        .op_i(op_i), // Pass op_i for R/I type distinction if needed for funct7/funct3 ambiguity
        .ALUOp_type_i(alu_op_type_w),
        .funct3_i(funct3_i),
        .funct7_5_i(funct7_5_i),

        .ALUControl_o(ALUControlD_o),
        .ALUModifier_o(ALUModifierD_o)
    );

endmodule