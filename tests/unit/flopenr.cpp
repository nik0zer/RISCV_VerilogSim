#include "Vflopenr.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>

vluint64_t sim_time = 0;
const int TEST_WIDTH = 64; // Matches default DATA_WIDTH

double sc_time_stamp() {
    return sim_time;
}

void tick(Vflopenr* top, VerilatedVcdC* tfp) {
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;

    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
    top->clk = 0; // End cycle with clk low
    top->eval();
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vflopenr* top = new Vflopenr;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_flopenr.vcd");

    std::cout << "Starting FLOPENR (Register with Reset and Enable) Testbench" << std::endl;

    // Initialize
    top->clk = 0;
    top->reset = 0;
    top->en = 0;
    top->d = 0;
    top->eval();

    // Test 1: Reset behavior
    std::cout << "Test 1: Reset (en=1, d=non-zero)" << std::endl;
    top->d = 0xAAAAAAAAAAAAAAAAULL;
    top->en = 1;
    top->reset = 1;
    tick(top, tfp);
    assert(top->q == 0 && "Reset failed: q should be 0");
    std::cout << "  q after reset: 0x" << std::hex << top->q << std::dec << std::endl;
    top->reset = 0;

    // Test 2: Load data when en=1
    std::cout << "Test 2: Load data (en=1)" << std::endl;
    uint64_t data_val1 = 0x1234;
    top->d = data_val1;
    top->en = 1;
    tick(top, tfp);
    assert(top->q == data_val1 && "Load with en=1 failed");
    std::cout << "  q after load (en=1): 0x" << std::hex << top->q << std::dec << std::endl;

    // Test 3: Data holds when en=0
    std::cout << "Test 3: Data holds (en=0)" << std::endl;
    top->d = 0x5678; // New data on d
    top->en = 0;    // Disable write
    tick(top, tfp);
    assert(top->q == data_val1 && "Data hold with en=0 failed"); // q should still be data_val1
    std::cout << "  q after attempt load (en=0): 0x" << std::hex << top->q << std::dec << std::endl;

    // Test 4: Load new data when en=1 again
    std::cout << "Test 4: Load new data (en=1 again)" << std::endl;
    uint64_t data_val2 = 0xABCD;
    top->d = data_val2;
    top->en = 1;
    tick(top, tfp);
    assert(top->q == data_val2 && "Load new data with en=1 failed");
    std::cout << "  q after new load (en=1): 0x" << std::hex << top->q << std::dec << std::endl;

    // Test 5: Reset overrides enable
    std::cout << "Test 5: Reset overrides enable (reset=1, en=1)" << std::endl;
    top->d = 0xFFFFFFFFFFFFFFFFULL;
    top->en = 1;
    top->reset = 1;
    tick(top, tfp);
    assert(top->q == 0 && "Reset with en=1 failed");
    std::cout << "  q after reset (en=1): 0x" << std::hex << top->q << std::dec << std::endl;
    top->reset = 0;


    std::cout << "FLOPENR Testbench Finished Successfully!" << std::endl;

    if (tfp) tfp->close();
    delete top;
    exit(EXIT_SUCCESS);
}