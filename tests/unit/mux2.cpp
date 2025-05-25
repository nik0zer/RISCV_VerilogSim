#include "Vmux2.h"
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

void eval_mux(Vmux2* mux, VerilatedVcdC* tfp) {
    mux->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vmux2* top = new Vmux2;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_mux2.vcd");

    std::cout << "Starting MUX2 Testbench (Width: " << DATA_WIDTH_TEST << ")" << std::endl;

    uint64_t val0, val1;

    val0 = 0xAAAAAAAAAAAAAAAAULL;
    val1 = 0x5555555555555555ULL;
    top->data0_i = val0;
    top->data1_i = val1;
    top->sel_i = 0;
    eval_mux(top, tfp);
    std::cout << "Test 1: sel=0, data0=0x" << std::hex << val0 << ", data1=0x" << val1 << ". Output=0x" << top->data_o << std::dec << std::endl;
    assert(top->data_o == val0 && "MUX2 Test 1 Failed: sel=0");

    top->sel_i = 1;
    eval_mux(top, tfp);
    std::cout << "Test 2: sel=1, data0=0x" << std::hex << val0 << ", data1=0x" << val1 << ". Output=0x" << top->data_o << std::dec << std::endl;
    assert(top->data_o == val1 && "MUX2 Test 2 Failed: sel=1");

    val0 = 0x0ULL;
    val1 = 0xFFFFFFFFFFFFFFFFULL;
    top->data0_i = val0;
    top->data1_i = val1;
    top->sel_i = 0;
    eval_mux(top, tfp);
    std::cout << "Test 3: sel=0, data0=0x" << std::hex << val0 << ", data1=0x" << val1 << ". Output=0x" << top->data_o << std::dec << std::endl;
    assert(top->data_o == val0 && "MUX2 Test 3 Failed: sel=0 with 0");

    top->sel_i = 1;
    eval_mux(top, tfp);
    std::cout << "Test 4: sel=1, data0=0x" << std::hex << val0 << ", data1=0x" << val1 << ". Output=0x" << top->data_o << std::dec << std::endl;
    assert(top->data_o == val1 && "MUX2 Test 4 Failed: sel=1 with all Fs");


    std::cout << "MUX2 Testbench Finished Successfully!" << std::endl;

    if (tfp) {
        tfp->close();
    }
    delete top;
    exit(EXIT_SUCCESS);
}