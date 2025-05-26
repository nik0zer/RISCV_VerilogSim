#include "Vflopr.h"
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

void tick(Vflopr* top, VerilatedVcdC* tfp) {
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;

    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
    top->clk = 0; // End cycle with clk low for next tick
    top->eval();
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vflopr* top = new Vflopr;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_flopr.vcd");

    std::cout << "Starting FLOPR (Register with Reset) Testbench" << std::endl;

    // Initialize
    top->clk = 0;
    top->reset = 0;
    top->d = 0;
    top->eval();

    // Test 1: Reset behavior
    std::cout << "Test 1: Reset" << std::endl;
    top->d = 0xAAAAAAAAAAAAAAAAULL;
    top->reset = 1;
    tick(top, tfp); // Apply reset on posedge clk or posedge reset
    assert(top->q == 0 && "Reset failed: q should be 0");
    std::cout << "  q after reset: 0x" << std::hex << top->q << std::dec << std::endl;
    top->reset = 0; // De-assert reset

    // Test 2: Load data
    std::cout << "Test 2: Load data 0x1234" << std::endl;
    uint64_t data_val1 = 0x1234;
    top->d = data_val1;
    tick(top, tfp); // clk 0->1, load d
    assert(top->q == data_val1 && "Load data_val1 failed");
    std::cout << "  q after load1: 0x" << std::hex << top->q << std::dec << std::endl;

    // Test 3: Load different data
    std::cout << "Test 3: Load data 0xABCD" << std::endl;
    uint64_t data_val2 = 0xABCD;
    top->d = data_val2;
    tick(top, tfp); // clk 0->1, load d
    assert(top->q == data_val2 && "Load data_val2 failed");
    std::cout << "  q after load2: 0x" << std::hex << top->q << std::dec << std::endl;

    // Test 4: Data holds if not reset
    std::cout << "Test 4: Data holds" << std::endl;
    top->d = 0xFFFFFFFFFFFFFFFFULL; // Change d, but q should hold previous value until next posedge clk
    top->clk = 0; top->eval(); // d changes, but q is stable
    assert(top->q == data_val2 && "Data hold before clock edge failed");
    tick(top, tfp); // clk 0->1, new d is loaded
    assert(top->q == 0xFFFFFFFFFFFFFFFFULL && "Load new data after hold failed");
    std::cout << "  q after new load: 0x" << std::hex << top->q << std::dec << std::endl;


    std::cout << "FLOPR Testbench Finished Successfully!" << std::endl;

    if (tfp) tfp->close();
    delete top;
    exit(EXIT_SUCCESS);
}