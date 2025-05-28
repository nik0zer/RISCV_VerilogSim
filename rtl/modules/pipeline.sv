`include "common/defines.svh"
`include "common/opcodes.svh"
`include "common/alu_defines.svh"

module pipeline #(parameter string INSTR_MEM_INIT_FILE = "", parameter string DATA_MEM_INIT_FILE = "",
                  parameter [`DATA_WIDTH-1:0] PC_START_ADDR = 64'h0) (
    input  logic clk_i,
    input  logic rst_i,

    output logic [`DATA_WIDTH-1:0] pc_f_o,
    output logic [`INSTR_WIDTH-1:0] instr_f_o,
    output logic [`DATA_WIDTH-1:0] imm_o,
    output logic [`REG_ADDR_WIDTH-1:0] rd_o,
    output logic [`REG_ADDR_WIDTH-1:0] rs1_o,
    output logic [`DATA_WIDTH-1:0] rs1_val_o,
    output logic [`REG_ADDR_WIDTH-1:0] rs2_o,
    output logic [`DATA_WIDTH-1:0] rs2_val_o,
    output logic [`DATA_WIDTH-1:0] wd3_d_o,
    output logic we3_d_o
);
    // Fetch Stage

    logic [`DATA_WIDTH-1:0] pc_f_new;
    logic [`DATA_WIDTH-1:0] pc_f_prev;
    logic stall_f;

    assign pc_f_o = pc_f_new;

    flopenr #(`DATA_WIDTH)
    flopenr_pc_f_prev(
        .clk(clk_i),
        .reset(1'b0),
        .en(!stall_f),
        .d(pc_f_prev),
        .q(pc_f_new));

    logic [`INSTR_WIDTH-1:0] instr_f;
    assign instr_f_o = instr_f;

    ram #(
        .N(`RAM_REAL_SIZE),
        .M(`INSTR_WIDTH),
        .OFFSET_BITS(2),
        .ADR_WIDTH(`DATA_WIDTH),
        .INIT_FILE(INSTR_MEM_INIT_FILE)
    ) ram_instr(
        .clk(clk_i),
        .we(1'b0),
        .adr(pc_f_new),
        .din({`INSTR_WIDTH{1'b0}}),
        .dout(instr_f)
    );

    logic [`DATA_WIDTH-1:0] pc_4_f;

    alu pc_4_alu(
        .operand_a(pc_f_new),
        .operand_b(4),
        .alu_op_select({`ALU_OP_ADD}),
        .alu_modifier(`ALU_SELECT_SIGNED),
        .result(pc_4_f)
    );

    // Flopenr Registers between Fetch and Decode

    logic stall_d;
    logic flush_d;

    logic [`INSTR_WIDTH-1:0] instr_d;
    logic [`DATA_WIDTH-1:0] pc_d;
    logic [`DATA_WIDTH-1:0] pc_4_d;


    flopenr #(`INSTR_WIDTH)
    flopenr_instr_f(
        .clk(clk_i),
        .reset(flush_d || rst_i),
        .en(!stall_d),
        .d(instr_f),
        .q(instr_d)
    );

    flopenr #(`DATA_WIDTH)
    flopenr_pc_4_f(
        .clk(clk_i),
        .reset(flush_d || rst_i),
        .en(!stall_d),
        .d(pc_4_f),
        .q(pc_4_d)
    );

    flopenr #(`DATA_WIDTH)
    flopenr_pc_f(
        .clk(clk_i),
        .reset(flush_d || rst_i),
        .en(!stall_d),
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
    logic we3_d;
    assign wd3_d_o = wd3_d;
    assign we3_d_o = we3_d;
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
        .clk(!clk_i),
        .we3(we3_d),
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

    // Flopr Registers between Decode and Execute

    logic flush_e;

    logic reg_write_e;
    logic [1:0] result_src_e;
    logic mem_write_e;
    logic jump_e;
    logic branch_e;
    logic [3:0] alu_control_e;
    logic alu_src_e;
    logic [`DATA_WIDTH-1:0] rd1_e;
    logic [`DATA_WIDTH-1:0] rd2_e;
    logic [`DATA_WIDTH-1:0] pc_e;
    logic [`REG_ADDR_WIDTH-1:0] rs1_e;
    logic [`REG_ADDR_WIDTH-1:0] rs2_e;
    logic [`REG_ADDR_WIDTH-1:0] rd_e;
    logic [`DATA_WIDTH-1:0] imm_e;
    logic [`DATA_WIDTH-1:0] pc_4_e;

    flopr #(.WIDTH(1))
    flopr_reg_write_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(reg_write_d),
        .q(reg_write_e)
    );

    flopr #(.WIDTH(2))
    flopr_result_src_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(result_src_d),
        .q(result_src_e)
    );

    flopr #(.WIDTH(1))
    flopr_mem_write_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(mem_write_d),
        .q(mem_write_e)
    );

    flopr #(.WIDTH(1))
    flopr_jump_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(jump_d),
        .q(jump_e)
    );

    flopr #(.WIDTH(1))
    flopr_branch_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(branch_d),
        .q(branch_e)
    );

    flopr #(.WIDTH(4))
    flopr_alu_control_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(alu_control_d),
        .q(alu_control_e)
    );

    flopr #(.WIDTH(1))
    flopr_alu_src_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(alu_src_d),
        .q(alu_src_e)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_rd1_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(rs1_val_d),
        .q(rd1_e)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_rd2_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(rs2_val_d),
        .q(rd2_e)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_pc_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(pc_d),
        .q(pc_e)
    );

    flopr #(.WIDTH(`REG_ADDR_WIDTH))
    flopr_rs1_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(rs1_d),
        .q(rs1_e)
    );

    flopr #(.WIDTH(`REG_ADDR_WIDTH))
    flopr_rs2_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(rs2_d),
        .q(rs2_e)
    );

    flopr #(.WIDTH(`REG_ADDR_WIDTH))
    flopr_rd_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(rd_d),
        .q(rd_e)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_imm_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(imm_d),
        .q(imm_e)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_pc_4_e(
        .clk(clk_i),
        .reset(flush_e),
        .d(pc_4_d),
        .q(pc_4_e)
    );

    // Execute Stage

    logic [`DATA_WIDTH-1:0] alu_result_m;
    logic [`DATA_WIDTH-1:0] alu_operand_a_e;
    logic [`DATA_WIDTH-1:0] alu_operand_b_reg;
    logic [`DATA_WIDTH-1:0] alu_operand_b_e;
    logic [1:0] forward_a_e;
    logic [1:0] forward_b_e;
    logic zero_flag_e;
    logic [`DATA_WIDTH-1:0] alu_result_e;
    logic [`DATA_WIDTH-1:0] write_data_e;
    assign write_data_e = alu_operand_b_reg;
    logic [`DATA_WIDTH-1:0] pc_target_e;



    mux3 #(.WIDTH(`DATA_WIDTH))
    mux3_alu_operand_a_e(
        .data0_i(rd1_e),
        .data1_i(wd3_d),
        .data2_i(alu_result_m),
        .sel_i(forward_a_e),
        .data_o(alu_operand_a_e)
    );

    mux3 #(.WIDTH(`DATA_WIDTH))
    mux3_alu_operand_b_e(
        .data0_i(rd2_e),
        .data1_i(wd3_d),
        .data2_i(alu_result_m),
        .sel_i(forward_b_e),
        .data_o(alu_operand_b_reg)
    );

    mux2 #(.WIDTH(`DATA_WIDTH))
    mux2_alu_operand_b_e(
        .data0_i(alu_operand_b_reg),
        .data1_i(imm_e),
        .sel_i(alu_src_e),
        .data_o(alu_operand_b_e)
    );

    alu main_alu(
        .operand_a(alu_operand_a_e),
        .operand_b(alu_operand_b_e),
        .alu_op_select(alu_control_e[2:0]),
        .alu_modifier(alu_control_e[3]),
        .result(alu_result_e),
        .zero_flag(zero_flag_e)
    );

    alu pc_alu(
        .operand_a(pc_e),
        .operand_b(imm_e),
        .alu_op_select(`ALU_OP_ADD),
        .alu_modifier(`ALU_SELECT_SIGNED),
        .result(pc_target_e)
    );

    logic pc_src_e;
    assign pc_src_e = (zero_flag_e && branch_e) || jump_e;

    // Registers between execute and memory

    logic reg_write_m;
    logic [1:0] result_src_m;
    logic mem_write_m;

    logic [`DATA_WIDTH-1:0] write_data_m;
    logic [`REG_ADDR_WIDTH-1:0] rd_m;
    logic [`DATA_WIDTH-1:0] pc_4_m;
    logic flush_m = 1'b0;

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_reg_write_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(reg_write_e),
        .q(reg_write_m)
    );

    flopr #(.WIDTH(2))
    flopr_result_src_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(result_src_e),
        .q(result_src_m)
    );

    flopr #(.WIDTH(1))
    flopr_mem_write_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(mem_write_e),
        .q(mem_write_m)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_alu_result_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(alu_result_e),
        .q(alu_result_m)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_write_data_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(write_data_e),
        .q(write_data_m)
    );

    flopr #(.WIDTH(`REG_ADDR_WIDTH))
    flopr_rd_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(rd_e),
        .q(rd_m)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_pc_4_m(
        .clk(clk_i),
        .reset(flush_m),
        .d(pc_4_e),
        .q(pc_4_m)
    );

    // Memory Stage

    logic [`DATA_WIDTH-1:0] read_data_m;

    ram #(
        .M(`DATA_WIDTH),
        .N(`RAM_REAL_SIZE),
        .ADR_WIDTH(`DATA_WIDTH),
        .OFFSET_BITS(3),
        .INIT_FILE(DATA_MEM_INIT_FILE)
    ) ram_data(
        .clk(clk_i),
        .we(mem_write_m),
        .adr(alu_result_m),
        .din(write_data_m),
        .dout(read_data_m)
    );

    // Registers between memory and writeback
    logic reg_write_w;
    logic [1:0] result_src_w;

    logic [`DATA_WIDTH-1:0] alu_result_w;
    logic [`DATA_WIDTH-1:0] read_data_w;
    logic [`REG_ADDR_WIDTH-1:0] rd_w;
    logic [`DATA_WIDTH-1:0] pc_4_w;
    logic flush_w = 1'b0;

    flopr #(.WIDTH(1))
    flopr_reg_write_w(
        .clk(clk_i),
        .reset(flush_w),
        .d(reg_write_m),
        .q(reg_write_w)
    );

    flopr #(.WIDTH(2))
    flopr_result_src_w(
        .clk(clk_i),
        .reset(flush_w),
        .d(result_src_m),
        .q(result_src_w)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_alu_result_w(
        .clk(clk_i),
        .reset(flush_w),
        .d(alu_result_m),
        .q(alu_result_w)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_read_data_w(
        .clk(clk_i),
        .reset(flush_w),
        .d(read_data_m),
        .q(read_data_w)
    );

    flopr #(.WIDTH(`REG_ADDR_WIDTH))
    flopr_rd_w(
        .clk(clk_i),
        .reset(flush_w),
        .d(rd_m),
        .q(rd_w)
    );

    flopr #(.WIDTH(`DATA_WIDTH))
    flopr_pc_4_w(
        .clk(clk_i),
        .reset(flush_w),
        .d(pc_4_m),
        .q(pc_4_w)
    );

    // Writeback Stage

    logic [`DATA_WIDTH-1:0] result_w;
    assign wd3_d = result_w;
    assign we3_d = reg_write_w;
    assign wa3_d = rd_w;

    mux3 #(.WIDTH(`DATA_WIDTH))
    mux3_result_w(
        .data0_i(alu_result_w),
        .data1_i(read_data_w),
        .data2_i(pc_4_w),
        .sel_i(result_src_w),
        .data_o(result_w)
    );


    logic [`DATA_WIDTH-1:0] pc_f_prev_calc;

    mux2 #(.WIDTH(`DATA_WIDTH))
    mux2_pc_w(
        .data0_i(pc_4_f),
        .data1_i(pc_target_e),
        .sel_i(pc_src_e),
        .data_o(pc_f_prev_calc)
    );

    assign pc_f_prev = rst_i ? PC_START_ADDR - 4 : pc_f_prev_calc;

    hazard_unit hazard_unit_inst(
        .Rs1E(rs1_e),
        .Rs2E(rs2_e),
        .Rs1D(rs1_d),
        .Rs2D(rs2_d),
        .RdE(rd_e),
        .RdM(rd_m),
        .RdW(rd_w),

        .RegWriteM(reg_write_m),
        .RegWriteW(reg_write_w),

        .ResultSrcE0(result_src_e[0]),
        .PCSrcE(pc_src_e),

        .ForwardAE(forward_a_e),
        .ForwardBE(forward_b_e),

        .StallF(stall_f),
        .StallD(stall_d),
        .FlushD(flush_d),
        .FlushE(flush_e)
    );


endmodule