#include "Vimm.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>
#include <vector>


const uint8_t IMMSRC_I = 0b00;
const uint8_t IMMSRC_S = 0b01;
const uint8_t IMMSRC_B = 0b10;
const uint8_t IMMSRC_J = 0b11;
const uint8_t IMMSRC_DEFAULT = 0b11;

vluint64_t sim_time = 0;
const int INSTR_WIDTH_TEST = 32;

double sc_time_stamp() {
    return sim_time;
}

void eval_imm(Vimm* top, VerilatedVcdC* tfp) {
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}


uint32_t extract_bits(uint32_t source, int msb, int lsb) {
    int len = msb - lsb + 1;
    return (source >> lsb) & ((1U << len) - 1);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vimm* top = new Vimm;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_imm.vcd");

    std::cout << "Starting Immediate Generation Testbench" << std::endl;

    struct TestCase {
        uint32_t instr_full;
        uint8_t  immsrc;
        uint32_t expected_immext;
        std::string name;
    };

    std::vector<TestCase> test_cases = {

        {0x00A00293, IMMSRC_I, 0x0000000A, "I-type ADDI x5, x0, 10"},
        {0xFF500293, IMMSRC_I, 0xFFFFFFF5, "I-type ADDI x5, x0, -11"},
        {0x7FF00000, IMMSRC_I, 0x000007FF, "I-type max pos (2047)"},
        {0x80000000, IMMSRC_I, 0xFFFFF800, "I-type min neg (-2048)"},

        {0x00A2A2A3, IMMSRC_S, 0x00000005, "S-type SW x10, 5(x5)"},
        {0xFEA2AE23, IMMSRC_S, 0xFFFFFFFC, "S-type SW x10, -4(x5)"},

        {0x00000463, IMMSRC_B, 0x00000008, "B-type BEQ x0,x0,+8"},
        {0xFE000EE3, IMMSRC_B, 0xFFFFFFFC, "B-type BNE x0,x0,-4"},

        {0x0200006F, IMMSRC_J, 0x00000020, "J-type JAL x0, +32"},
        {0xFFE0006F, IMMSRC_J, 0xFFF007FE, "J-type JAL x0, -1046530"}
    };

    int passed_count = 0;
    for (const auto& tc : test_cases) {



        top->instr = (tc.instr_full >> 7);
        top->immsrc = tc.immsrc;
        eval_imm(top, tfp);

        bool pass = (top->immext == tc.expected_immext);
        if (pass) {
            passed_count++;
            std::cout << "PASS: " << tc.name << std::endl;
        } else {
            std::cout << "FAIL: " << tc.name << std::endl;
            std::cout << "  Instruction: 0x" << std::hex << tc.instr_full
                      << " (port val 0x" << (tc.instr_full >> 7) << ")" << std::dec << std::endl;
            std::cout << "  Immsrc: " << std::to_string(tc.immsrc) << std::endl;
            std::cout << "  Got ImmExt:   0x" << std::hex << top->immext << std::dec << std::endl;
            std::cout << "  Expected ImmExt: 0x" << std::hex << tc.expected_immext << std::dec << std::endl;
        }
        assert(pass);
    }


    std::cout << "\nImmediate Generation Testbench Finished. Passed "
              << passed_count << "/" << test_cases.size() << " tests." << std::endl;

    if (tfp) {
        tfp->close();
    }
    delete top;
    return (passed_count == test_cases.size()) ? EXIT_SUCCESS : EXIT_FAILURE;
}