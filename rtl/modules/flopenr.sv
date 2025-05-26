`include "common/defines.svh"

module flopenr #(parameter WIDTH = `DATA_WIDTH)
                (input logic clk, reset, en,
                 input logic [WIDTH-1:0] d,
                 output logic [WIDTH-1:0] q);
    always_ff @(posedge clk, posedge reset) begin
        if (reset)      q <= {WIDTH{1'b0}};
        else if (en)    q <= d;
    end
endmodule