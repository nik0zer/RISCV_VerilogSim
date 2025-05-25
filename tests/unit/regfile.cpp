#include "Vregfile.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>

vluint64_t sim_time = 0;

double sc_time_stamp() {
    return sim_time;
}

void tick(Vregfile* top, VerilatedVcdC* tfp) {
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;

    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vregfile* top = new Vregfile;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_regfile.vcd");


    std::cout << "Starting 64-bit Regfile Testbench" << std::endl; // Изменено сообщение

    top->we3 = 0;
    top->a1 = 0;
    top->a2 = 0;
    top->a3 = 0;
    top->wd3 = 0;

    std::cout << "Cycle 0: Initial state checks" << std::endl;
    tick(top, tfp);
    top->a1 = 0; top->eval(); assert(top->rd1 == 0 && "Initial read x0 failed");
    top->a1 = 1; top->eval(); assert(top->rd1 == 0 && "Initial read x1 failed (should be 0 due to initial block)");

    const uint64_t val1 = 0xDEADBEEFCAFEBABFULL;
    const uint64_t val2 = 0x123456789ABCDEF0ULL;
    const uint64_t val_bad = 0xBAD0BAD0BAD0BAD0ULL;
    const uint64_t val3 = 0xAABBCCDDEEFF0011ULL;


    std::cout << "Cycle 1: Write 0x" << std::hex << val1 << " to x1 (reg_addr=1)" << std::dec << std::endl;
    top->a3 = 1;
    top->wd3 = val1;
    top->we3 = 1;
    tick(top, tfp);
    top->we3 = 0;

    std::cout << "Cycle 2: Check x1. Write 0x" << std::hex << val2 << " to x5 (reg_addr=5)" << std::dec << std::endl;
    top->a1 = 1; top->eval();
    std::cout << "  Read x1: 0x" << std::hex << top->rd1 << std::dec << std::endl;
    assert(top->rd1 == val1 && "Read x1 after write failed");

    top->a1 = 0; top->eval(); assert(top->rd1 == 0 && "Read x0 sanity check failed");

    top->a3 = 5;
    top->wd3 = val2;
    top->we3 = 1;
    tick(top, tfp);
    top->we3 = 0;

    std::cout << "Cycle 3: Check x1, x5. Attempt write 0x" << std::hex << val_bad << " to x0 (reg_addr=0)" << std::dec << std::endl;
    top->a1 = 1; top->eval(); assert(top->rd1 == val1 && "Read x1 again failed");
    top->a2 = 5; top->eval();
    std::cout << "  Read x5: 0x" << std::hex << top->rd2 << std::dec << std::endl;
    assert(top->rd2 == val2 && "Read x5 after write failed");

    top->a3 = 0;
    top->wd3 = val_bad;
    top->we3 = 1;
    tick(top, tfp);
    top->we3 = 0;

    std::cout << "Cycle 4: Check x0 (should be 0), x1, x5" << std::endl;
    top->a1 = 0; top->eval();
    std::cout << "  Read x0 after attempted write: 0x" << std::hex << top->rd1 << std::dec << std::endl;
    assert(top->rd1 == 0 && "Read x0 after attempted write failed (should remain 0)");
    top->a1 = 1; top->eval(); assert(top->rd1 == val1 && "Read x1 again (cycle 4) failed");
    top->a2 = 5; top->eval(); assert(top->rd2 == val2 && "Read x5 again (cycle 4) failed");

    std::cout << "Cycle 5: Write x2 (we3=1), then attempt x3 (we3=0)" << std::endl;
    top->a3 = 2; top->wd3 = val3; top->we3 = 1;
    tick(top, tfp);
    top->we3 = 0;

    top->a3 = 3; top->wd3 = 0x8765432112345678ULL;
    tick(top, tfp);

    std::cout << "Cycle 6: Check x2 and x3" << std::endl;
    top->a1 = 2; top->eval();
    std::cout << "  Read x2: 0x" << std::hex << top->rd1 << std::dec << std::endl;
    assert(top->rd1 == val3 && "Read x2 after write failed");
    top->a1 = 3; top->eval();
    std::cout << "  Read x3 (attempted write with we3=0): 0x" << std::hex << top->rd1 << std::dec << std::endl;
    assert(top->rd1 == 0 && "Read x3 after write attempt with we3=0 failed (should be 0 from initial)");


    std::cout << "64-bit Regfile Testbench Finished Successfully!" << std::endl;

    if (tfp) {
        tfp->close();
    }
    delete top;
    exit(EXIT_SUCCESS);
}