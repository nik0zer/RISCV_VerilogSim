#include "Vmux3.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>

vluint64_t sim_time = 0;
const int DATA_WIDTH_TEST = 64;

double sc_time_stamp() {
    return sim_time;
}

void eval_mux(Vmux3* mux, VerilatedVcdC* tfp) {
    mux->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vmux3* top = new Vmux3;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_mux3.vcd");

    std::cout << "Starting MUX3 Testbench (Width: " << DATA_WIDTH_TEST << ")" << std::endl;

    uint64_t val0 = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t val1 = 0x5555555555555555ULL;
    uint64_t val2 = 0xF0F0F0F0F0F0F0F0ULL;

    top->data0_i = val0;
    top->data1_i = val1;
    top->data2_i = val2;

    top->sel_i = 0;
    eval_mux(top, tfp);
    std::cout << "Test 1: sel=0b00. Output=0x" << std::hex << top->data_o << std::dec << std::endl;
    assert(top->data_o == val0 && "MUX3 Test 1 Failed: sel=0b00");

    top->sel_i = 1;
    eval_mux(top, tfp);
    std::cout << "Test 2: sel=0b01. Output=0x" << std::hex << top->data_o << std::dec << std::endl;
    assert(top->data_o == val1 && "MUX3 Test 2 Failed: sel=0b01");

    top->sel_i = 2;
    eval_mux(top, tfp);
    std::cout << "Test 3: sel=0b10. Output=0x" << std::hex << top->data_o << std::dec << std::endl;
    assert(top->data_o == val2 && "MUX3 Test 3 Failed: sel=0b10");

    top->sel_i = 3;
    eval_mux(top, tfp);
    std::cout << "Test 4: sel=0b11 (default). Output=0x" << std::hex << top->data_o << std::dec << std::endl;
    assert(top->data_o == val0 && "MUX3 Test 4 Failed: sel=0b11 (default to data0)");


    std::cout << "MUX3 Testbench Finished Successfully!" << std::endl;

    if (tfp) {
        tfp->close();
    }
    delete top;
    exit(EXIT_SUCCESS);
}