`include "common/defines.svh"

module ram #(parameter N = 10, M = `DATA_WIDTH, OFFSET_BITS = (M==32) ? 2 : 3, ADR_WIDTH = `DATA_WIDTH, parameter string INIT_FILE = "")
            (input  logic         clk,
            input  logic         we,
            input  logic [ADR_WIDTH-1:0] adr,
            input  logic [M-1:0] din,
            output logic [M-1:0] dout);

  localparam MEM_DEPTH = 2**(N - OFFSET_BITS);

  logic [M-1:0] mem [2**(N-OFFSET_BITS)-1:0];
  always_ff @(posedge clk)
    if (we) mem [adr[N - 1 : OFFSET_BITS]] <= din;
  assign dout = mem[adr[N - 1 : OFFSET_BITS]];

  initial begin
      if (INIT_FILE != "") begin
          $display("RAM module %m (instance path) initializing from file: %s", $sformatf("%m"), INIT_FILE);
          $readmemh(INIT_FILE, mem);
      end else begin
          for (int i = 0; i < MEM_DEPTH; i++) begin
              mem[i] = {M{1'b0}};
          end
            $display("RAM module %m initialized to zeros (no INIT_FILE).", $sformatf("%m"));
      end
  end


endmodule
