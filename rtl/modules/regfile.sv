`include "common/defines.svh"

module regfile (
    input  logic        clk,
    input  logic        we3,
    input  logic [`REG_ADDR_WIDTH-1:0]  a1,
    input  logic [`REG_ADDR_WIDTH-1:0]  a2,
    input  logic [`REG_ADDR_WIDTH-1:0]  a3,
    input  logic [`DATA_WIDTH-1:0] wd3,
    output logic [`DATA_WIDTH-1:0] rd1,
    output logic [`DATA_WIDTH-1:0] rd2
);

    logic [63:0] rf[31:0];



    always_ff @(posedge clk) begin
        if (we3 && (a3 != 5'b0)) begin
            rf[a3] <= wd3;
        end
    end



    assign rd1 = (a1 == `REG_ADDR_WIDTH'b0) ? `DATA_WIDTH'b0 : rf[a1];
    assign rd2 = (a2 == `REG_ADDR_WIDTH'b0) ? `DATA_WIDTH'b0 : rf[a2];


    initial begin
        for (int i = 0; i < 2**`REG_ADDR_WIDTH; i++) begin
            rf[i] = `DATA_WIDTH'b0;
        end
    end

endmodule
