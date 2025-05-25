`include "common/defines.svh"

module ram #(parameter N = 10, M = `DATA_WIDTH, OFFSET_BITS = 3)
            (input  logic         clk,
            input  logic         we,
            input  logic [N-1:0] adr,
            input  logic [M-1:0] din,
            output logic [M-1:0] dout);

  logic [M-1:0] mem [2**(N-OFFSET_BITS)-1:0];
  always_ff @(posedge clk)
    if (we) mem [adr[N - 1 : OFFSET_BITS]] <= din;
  assign dout = mem[adr[N - 1 : OFFSET_BITS]];

endmodule
