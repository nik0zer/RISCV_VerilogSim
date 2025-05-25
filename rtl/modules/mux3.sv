`include "common/defines.svh"

module mux3 #(parameter WIDTH = `DATA_WIDTH) (
    input  logic [WIDTH-1:0] data0_i,
    input  logic [WIDTH-1:0] data1_i,
    input  logic [WIDTH-1:0] data2_i,
    input  logic [1:0]       sel_i,
    output logic [WIDTH-1:0] data_o
);

    always_comb begin
        case (sel_i)
            2'b00:   data_o = data0_i;
            2'b01:   data_o = data1_i;
            2'b10:   data_o = data2_i;
            default: data_o = data0_i;
        endcase
    end

endmodule