#include "Vpipeline.h"
#include "verilated_vcd_c.h"
#include "verilated.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>

// Макросы, определяемые CMake
#ifndef PIPELINE_TEST_CASE_NAME_STR_RAW
#error "PIPELINE_TEST_CASE_NAME_STR_RAW not defined! Pass it via CFLAGS from CMake."
#endif

#ifndef EXPECTED_WD3_FILE_PATH_STR_RAW
#error "EXPECTED_WD3_FILE_PATH_STR_RAW not defined! Pass it via CFLAGS from CMake."
#endif

#ifndef NUM_CYCLES_TO_RUN
#error "NUM_CYCLES_TO_RUN not defined! Pass it via CFLAGS from CMake."
#endif

// Вспомогательные макросы для превращения в строку
#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

// Глобальные переменные из макросов
const std::string G_PIPELINE_TEST_CASE_NAME = STRINGIFY(PIPELINE_TEST_CASE_NAME_STR_RAW);
const std::string G_EXPECTED_WD3_FILE_PATH = STRINGIFY(EXPECTED_WD3_FILE_PATH_STR_RAW);
const int G_NUM_CYCLES_TO_RUN = NUM_CYCLES_TO_RUN;
const uint64_t X_DEF = 0xFFFFFFFFFFFFFFFFUL;


vluint64_t sim_time = 0;

double sc_time_stamp() {
    return sim_time;
}

void tick(Vpipeline* top, VerilatedVcdC* tfp) {
    top->clk_i = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;

    top->clk_i = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}

bool load_expected_wd3_values(const std::string& filepath, std::vector<uint64_t>& values, int expected_num_cycles) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open expected wd3_o file: " << filepath << std::endl;
        return false;
    }
    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        try {
            // Если строка "X" или "x", то это "не важно" или "нет записи", будем хранить спец. значение, например X_DEF
            if (line == "X" || line == "x") {
                values.push_back(X_DEF);
            } else {
                values.push_back(std::stoull(line, nullptr, 16));
            }
            line_count++;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing hex value '" << line << "' at line " << (line_count + 1) << ": " << e.what() << std::endl;
            return false;
        }
    }
    file.close();
    if (line_count < expected_num_cycles) {
        std::cerr << "ERROR: Number of expected wd3_o values (" << line_count
                  << ") is less than NUM_CYCLES_TO_RUN (" << expected_num_cycles << ")." << std::endl;
        std::cerr << "Please provide an expected value (or 'X' if no write) for each cycle." << std::endl;
        return false;
    }
    if (line_count > expected_num_cycles) {
         std::cerr << "Warning: Number of expected wd3_o values (" << line_count
                  << ") is greater than NUM_CYCLES_TO_RUN (" << expected_num_cycles << ")." << std::endl;
    }
    return true;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vpipeline* top = new Vpipeline;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    std::string vcd_file_name = G_PIPELINE_TEST_CASE_NAME + "_pipeline_tb.vcd";
    tfp->open(vcd_file_name.c_str());

    std::cout << "Starting Pipeline Test Case: " << G_PIPELINE_TEST_CASE_NAME << std::endl;
    std::cout << "Expected wd3_o file: " << G_EXPECTED_WD3_FILE_PATH << std::endl;
    std::cout << "Number of cycles to run: " << G_NUM_CYCLES_TO_RUN << std::endl;

    std::vector<uint64_t> expected_wd3_per_cycle;
    if (!load_expected_wd3_values(G_EXPECTED_WD3_FILE_PATH, expected_wd3_per_cycle, G_NUM_CYCLES_TO_RUN)) {
        if (tfp) tfp->close();
        delete top;
        return 1;
    }

    top->rst_i = 1;
    for(int i=0; i<2; ++i) {
        tick(top, tfp);
    }
    top->rst_i = 0;
    std::cout << "Reset complete." << std::endl;

    bool test_passed = true;

    std::cout << "\nCycle | PC_F     | Instr_D  | WE3 | WD3_Out (Got) | WD3_Out (Exp) | Status" << std::endl;
    std::cout << "------|----------|----------|-----|---------------|-----------------|-------" << std::endl;

    for (int cycle = 0; cycle < G_NUM_CYCLES_TO_RUN; ++cycle) {
        tick(top, tfp);

        uint64_t current_wd3_value = top->wd3_d_o;
        bool current_we3 = top->we3_d_o;
        uint64_t effective_wd3_got = current_we3 ? current_wd3_value : 0; // Считаем 0, если WE3=0

        uint64_t expected_wd3 = expected_wd3_per_cycle[cycle];
        bool expect_write = (expected_wd3 != X_DEF); // Если не X_DEF, значит ожидаем запись
        uint64_t effective_expected_wd3 = expect_write ? expected_wd3 : 0;


        std::cout << std::setw(5) << std::dec << cycle << " | "
                  << "0x" << std::setw(8) << std::setfill('0') << std::hex << top->pc_f_o << " | "
                  << "0x" << std::setw(8) << std::setfill('0') << std::hex << top->instr_f_o << " | "
                  << std::setw(3) << std::dec << (current_we3 ? "1" : "0") << " | " // Вывод WE3
                  << "0x" << std::setw(16) << std::setfill('0') << std::hex << current_wd3_value << " | "
                  << (expect_write ? ("0x" + [&]{std::stringstream ss; ss << std::setw(16) << std::setfill('0') << std::hex << expected_wd3; return ss.str(); }()) : "      X (no write)     ");

        bool cycle_pass = true;
        if (expect_write) { // Если в expected файле число (а не X)
            if (!current_we3) { // А записи не было
                cycle_pass = false;
                std::cout << " | FAIL (Exp Write, Got No Write)";
            } else if (current_wd3_value != expected_wd3) { // Запись была, но значение не то
                cycle_pass = false;
                std::cout << " | FAIL (Value Mismatch)";
            } else { // Запись была, значение то
                std::cout << " | PASS";
            }
        } else { // Если в expected файле X (ожидаем, что записи не будет)
            if (current_we3) { // А запись была
                cycle_pass = false;
                std::cout << " | FAIL (Exp No Write, Got Write)";
            } else { // Записи не было, как и ожидалось
                std::cout << " | PASS (No Write)";
            }
        }
        std::cout << std::endl;

        if (!cycle_pass) {
            test_passed = false;
        }
        std::cout << std::setfill(' ');
    }

    if (tfp) {
        tfp->close();
    }
    delete top;

    if (test_passed) {
        std::cout << "\nPipeline Test Case: " << G_PIPELINE_TEST_CASE_NAME << " - PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "\nPipeline Test Case: " << G_PIPELINE_TEST_CASE_NAME << " - FAILED" << std::endl;
        return 1;
    }
}