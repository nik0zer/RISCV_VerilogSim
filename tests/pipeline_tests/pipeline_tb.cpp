#include "Vpipeline.h"
#include "verilated_vcd_c.h"
#include "verilated.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cassert> // Для assert, если он нужен где-то еще

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
            values.push_back(std::stoull(line, nullptr, 16));
            line_count++;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing hex value '" << line << "' at line " << (line_count + 1) << ": " << e.what() << std::endl;
            return false;
        }
    }
    file.close();
    // Сверяем количество прочитанных значений с количеством циклов.
    // Если значений меньше, тест не сможет проверить все циклы.
    // Если значений больше, это просто предупреждение.
    if (line_count < expected_num_cycles) {
        std::cerr << "ERROR: Number of expected wd3_o values (" << line_count
                  << ") is less than NUM_CYCLES_TO_RUN (" << expected_num_cycles << ")." << std::endl;
        std::cerr << "Please provide an expected value for each cycle." << std::endl;
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

    std::cout << "\nCycle | PC_F     | Instr_D  | WD3_Out (Got) | WD3_Out (Exp) | Status" << std::endl;
    std::cout << "------|----------|----------|---------------|-----------------|-------" << std::endl;

    for (int cycle = 0; cycle < G_NUM_CYCLES_TO_RUN; ++cycle) {
        tick(top, tfp);

        uint64_t current_wd3 = top->wd3_d_o;
        uint64_t expected_wd3 = expected_wd3_per_cycle[cycle]; // Мы уже проверили, что размер достаточный

        std::cout << std::setw(5) << std::dec << cycle << " | "
                  << "0x" << std::setw(8) << std::setfill('0') << std::hex << top->pc_f_o << " | "
                  << "0x" << std::setw(8) << std::setfill('0') << std::hex << top->instr_f_o << " | "
                  << "0x" << std::setw(16) << std::setfill('0') << std::hex << current_wd3 << " | "
                  << "0x" << std::setw(16) << std::setfill('0') << std::hex << expected_wd3;

        if (current_wd3 != expected_wd3) {
            test_passed = false;
            std::cout<< " | FAIL" << std::endl;
        } else {
            std::cout<< " | PASS" << std::endl;
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