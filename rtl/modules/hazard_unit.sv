`include "common/defines.svh"
`include "common/opcodes.svh"

module hazard_unit (
    input logic [4:0] Rs1E,
    input logic [4:0] Rs2E,
    input logic [4:0] Rs1D,
    input logic [4:0] Rs2D,
    input logic [4:0] RdE,
    input logic [4:0] RdM,
    input logic [4:0] RdW,

    input logic RegWriteM,
    input logic RegWriteW,

    input logic ResultSrcE0,
    input logic PCSrcE,
    input logic stall_mem_2_store,

    output logic [1:0] ForwardAE,
    output logic [1:0] ForwardBE,

    output logic StallF,
    output logic StallD,
    output logic StallE,
    output logic StallM,

    output logic FlushD,
    output logic FlushE
);

    logic lwStall;

    always_comb begin
        if (((Rs1E == RdM) & RegWriteM) & (Rs1E != 0))
            ForwardAE = 2'b10;
        else if (((Rs1E == RdW) & RegWriteW) & (Rs1E != 0))
            ForwardAE = 2'b01;
        else
            ForwardAE = 2'b00;

        if (((Rs2E == RdM) & RegWriteM) & (Rs2E != 0))
            ForwardBE = 2'b10;
        else if (((Rs2E == RdW) & RegWriteW) & (Rs2E != 0))
            ForwardBE = 2'b01;
        else
            ForwardBE = 2'b00;


        lwStall = ResultSrcE0 & ((Rs1D == RdE) | (Rs2D == RdE));
        StallF  = lwStall | stall_mem_2_store;
        StallD  = lwStall | stall_mem_2_store;
        StallE  = stall_mem_2_store;
        StallM  = stall_mem_2_store;

        FlushD = PCSrcE;
        FlushE = lwStall | PCSrcE;
    end

endmodule