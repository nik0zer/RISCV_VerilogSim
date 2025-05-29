`include "common/defines.svh"

module mem_2_store (
    input logic clk,
    input logic [`DATA_WIDTH-1:0] addr,
    input logic mem_2_store_enable,
    input logic rst,

    output logic [`DATA_WIDTH-1:0] new_addr,
    output logic stall_mem_2_store
);

    logic num_of_mem_store;
    assign new_addr = addr + (num_of_mem_store * 8);

    mux2 #(.WIDTH(1))
    mux2_stall_mem_4_store(
        .data0_i(1'b0),
        .data1_i(1'b1),
        .sel_i(mem_2_store_enable && (num_of_mem_store == 1'b0)),
        .data_o(stall_mem_2_store)
    );

    always_ff @(posedge clk) begin
        if (rst) begin
            num_of_mem_store <= 1'b0;
        end else begin
            if (mem_2_store_enable) begin
                num_of_mem_store <= num_of_mem_store + 1;
            end
        end
    end




endmodule