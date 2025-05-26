`include "common/defines.svh"
`include "common/opcodes.svh"
`include "common/alu_defines.svh"

module pipeline (
    input  logic clk_i,
    input  logic rst_i,

    output logic [`DATA_WIDTH-1:0] pc_f_o,
    output logic [`INSTR_WIDTH-1:0] instr_f_o,
    output logic [`DATA_WIDTH-1:0] imm_o,
    output logic [`REG_ADDR_WIDTH-1:0] rd_o,
    output logic [`DATA_WIDTH-1:0] rd_val_o,
    output logic [`REG_ADDR_WIDTH-1:0] rs1_o,
    output logic [`DATA_WIDTH-1:0] rs1_val_o,
    output logic [`REG_ADDR_WIDTH-1:0] rs2_o,
    output logic [`DATA_WIDTH-1:0] rs2_val_o
);

    // Fetch Stage

    logic [`DATA_WIDTH-1:0] pc_f_new;
    logic [`DATA_WIDTH-1:0] pc_f_prev;

    assign pc_f_o = pc_f_new;

    flopenr #(`DATA_WIDTH)
    flopenr_pc_f_prev(
        .clk(clk_i),
        .reset(rst_i),
        .en(1'b1),
        .d(pc_f_prev),
        .q(pc_f_new));

    logic [`INSTR_WIDTH-1:0] instr_f;
    assign instr_f_o = instr_f;

    ram #(
        .N(`RAM_REAL_SIZE),
        .M(`INSTR_WIDTH),
        .OFFSET_BITS(2),
        .ADR_WIDTH(`DATA_WIDTH)
    ) ram_instr(
        .clk(clk_i),
        .we(1'b0),
        .adr(pc_f_prev),
        .din({`INSTR_WIDTH{1'b0}}),
        .dout(instr_f)
    );

    logic [`DATA_WIDTH-1:0] pc_f_4;

    alu pc_4_alu(
        .operand_a(pc_f_new),
        .operand_b(4),
        .alu_op_select({`ALU_OP_ADD}),
        .alu_modifier(1'b0),
        .result(pc_f_4)
    );

    // Flopenr Registers between Fetch and Decode

    logic stall_d = 1'b0;
    logic flush_d = 1'b0;

    logic [`INSTR_WIDTH-1:0] instr_d;
    logic [`DATA_WIDTH-1:0] pc_d;
    logic [`DATA_WIDTH-1:0] pc_4_d;


    flopenr #(`INSTR_WIDTH)
    flopenr_instr_f(
        .clk(clk_i),
        .reset(flush_d),
        .en(~stall_d),
        .d(instr_f),
        .q(instr_d)
    );

    flopenr #(`DATA_WIDTH)
    flopenr_pc_f_4(
        .clk(clk_i),
        .reset(flush_d),
        .en(~stall_d),
        .d(pc_f_4),
        .q(pc_4_d)
    );

    flopenr #(`DATA_WIDTH)
    flopenr_pc_f(
        .clk(clk_i),
        .reset(flush_d),
        .en(~stall_d),
        .d(pc_f_new),
        .q(pc_d)
    );

    // Decode Stage

    logic [`REG_ADDR_WIDTH-1:0] rs1_d;
    assign rs1_o = rs1_d;
    logic [`DATA_WIDTH-1:0] rs1_val_d;
    assign rs1_val_o = rs1_val_d;
    logic [`REG_ADDR_WIDTH-1:0] rs2_d;
    assign rs2_o = rs2_d;
    logic [`DATA_WIDTH-1:0] rs2_val_d;
    assign rs2_val_o = rs2_val_d;
    logic [`REG_ADDR_WIDTH-1:0] rd_d;
    assign rd_o = rd_d;
    logic [`DATA_WIDTH-1:0] rd_val_d;
    assign rd_val_o = rd_val_d;
    logic [`DATA_WIDTH-1:0] imm_d;
    logic [`INSTR_WIDTH-1:0] imm_d_32;
    assign imm_d = {{(`DATA_WIDTH-`INSTR_WIDTH){imm_d_32[31]}}, imm_d_32};
    assign imm_o = imm_d;
    logic reg_write_d;
    logic [1:0] result_src_d;
    logic mem_write_d;
    logic jump_d;
    logic branch_d;
    logic [3:0] alu_control_d;
    logic alu_src_d;
    logic [1:0] imm_src_d;
    logic [`REG_ADDR_WIDTH-1:0] wa3_d;
    logic [`DATA_WIDTH-1:0] wd3_d;
    logic is_u_type_d;

    control_unit cu(
        .op_i(instr_d[6:0]),
        .funct3_i(instr_d[14:12]),
        .funct7_5_i(instr_d[30]),

        .RegWriteD_o(reg_write_d),
        .ResultSrcD_o(result_src_d),
        .MemWriteD_o(mem_write_d),
        .JumpD_o(jump_d),
        .BranchD_o(branch_d),
        .ALUSrcD_o(alu_src_d),
        .ImmSelD_o(imm_src_d),
        .Is_U_typeD_o(is_u_type_d),
        .ALUControlD_o(alu_control_d[2:0]),
        .ALUModifierD_o(alu_control_d[3])
    );

    regfile regfile(
        .clk(~clk_i),
        .we3(1'b1),
        .a1(instr_d[19:15]),
        .a2(instr_d[24:20]),
        .a3(wa3_d),
        .wd3(wd3_d),
        .rd1(rs1_val_d),
        .rd2(rs2_val_d)
    );

    assign rs1_d = instr_d[19:15];
    assign rs2_d = instr_d[24:20];
    assign rd_d = instr_d[11:7];

    imm imm_gen(
        .instr(instr_d[`INSTR_WIDTH-1:7]),
        .immsrc(imm_src_d),
        .immext(imm_d_32)
    );

    // Flopenr Registers between Decode and Execute

endmodule