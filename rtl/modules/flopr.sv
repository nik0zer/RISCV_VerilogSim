`include "common/defines.svh"

module flopr #(parameter WIDTH = `DATA_WIDTH)
              (input  logic             clk, reset,
               input  logic [WIDTH-1:0] d,
               output logic [WIDTH-1:0] q);

    always_ff @(posedge clk) begin
        if (reset)  q <= {WIDTH{1'b0}};
        else        q <= d;
    end

endmodule