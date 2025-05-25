#include "Vram.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <vector>
#include <cstdint>

vluint64_t sim_time = 0;

double sc_time_stamp() {
    return sim_time;
}

// Функция для одного тактового импульса (только для записи, т.к. чтение асинхронное)
// Для асинхронного чтения, вызов eval() после изменения adr сразу обновит dout.
void tick_write(Vram* top, VerilatedVcdC* tfp) {
    top->clk = 0;
    top->eval(); // Вычислить перед фронтом
    if (tfp) tfp->dump(sim_time);
    sim_time++;

    top->clk = 1;
    top->eval(); // Вычислить на фронте (здесь произойдет запись)
    if (tfp) tfp->dump(sim_time);
    sim_time++;
}

// Для асинхронного чтения, просто меняем адрес и вызываем eval
void eval_read(Vram* top, VerilatedVcdC* tfp) {
    top->eval();
    if (tfp) tfp->dump(sim_time);
    // Не инкрементируем sim_time здесь, если это не "такт"
}


int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vram* top = new Vram; // Используем Vram

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("tb_ram.vcd");

    std::cout << "Starting RAM Testbench" << std::endl;

    // Параметры нашего модуля (из Verilog)
    // Эти значения должны соответствовать тем, что в ram.sv, если они не переопределены при инстанцировании Verilator'ом
    // (что обычно не делается для параметров модуля по умолчанию в простом Verilate)
    // Мы не можем получить их из Vram напрямую в C++, поэтому зададим их здесь для теста.
    const int ADDR_IN_WIDTH = 10; // Как в Verilog по умолчанию
    const int DATA_WIDTH = 64;
    const int OFFSET_BITS = 3;
    const int WORD_ADDR_WIDTH = ADDR_IN_WIDTH - OFFSET_BITS;
    const int NUM_WORDS = 1 << WORD_ADDR_WIDTH;
    const int BYTES_PER_WORD = DATA_WIDTH / 8;

    // Начальная инициализация
    top->clk = 0;
    top->we = 0;
    top->adr = 0;
    top->din = 0;
    top->eval(); // Начальное состояние

    // --- Тестовые сценарии ---
    std::cout << "RAM size: " << NUM_WORDS << " words (" << (NUM_WORDS * BYTES_PER_WORD) << " bytes)" << std::endl;

    // 1. Запись нескольких значений
    std::cout << "Test 1: Writing values" << std::endl;
    uint64_t test_values[] = {0x1122334455667788ULL, 0xAABBCCDDEEFF0011ULL, 0xDEADBEEFCAFEBABEULL};
    uint32_t byte_addresses[] = {0 * BYTES_PER_WORD, 1 * BYTES_PER_WORD, 5 * BYTES_PER_WORD}; // Адреса слов 0, 1, 5

    for (int i = 0; i < 3; ++i) {
        top->adr = byte_addresses[i];
        top->din = test_values[i];
        top->we = 1;
        std::cout << "  Writing 0x" << std::hex << top->din << " to byte_adr 0x" << top->adr << std::dec << std::endl;
        tick_write(top, tfp); // Запись по фронту clk
    }
    top->we = 0; // Отключить запись

    // 2. Чтение записанных значений
    std::cout << "Test 2: Reading written values (asynchronous read)" << std::endl;
    for (int i = 0; i < 3; ++i) {
        top->adr = byte_addresses[i];
        eval_read(top, tfp); // Обновить dout после изменения adr
        std::cout << "  Reading from byte_adr 0x" << std::hex << top->adr << ": got 0x" << top->dout << std::dec << std::endl;
        assert(top->dout == test_values[i] && "Read-after-write failed");
    }

    // 3. Чтение из неинициализированной (но обнуленной initial блоком) ячейки
    std::cout << "Test 3: Reading from unwritten (but initialized to zero) cell" << std::endl;
    uint32_t unwritten_byte_addr = 10 * BYTES_PER_WORD; // Адрес слова 10
    if (unwritten_byte_addr < (NUM_WORDS * BYTES_PER_WORD)) { // Убедимся, что адрес в пределах нашего маленького RAM
        top->adr = unwritten_byte_addr;
        eval_read(top, tfp);
        std::cout << "  Reading from byte_adr 0x" << std::hex << top->adr << ": got 0x" << top->dout << std::dec << std::endl;
        assert(top->dout == 0 && "Unwritten cell not zero");
    } else {
        std::cout << "  Skipping read from unwritten cell, address 0x" << std::hex << unwritten_byte_addr << " is out of modeled range." << std::dec << std::endl;
    }

    // 4. Проверка "заворота" адреса (если ADDR_IN_WIDTH позволяет)
    // Мы используем только младшие ADDR_IN_WIDTH бит адреса, и из них WORD_ADDR_WIDTH для индекса.
    // Если мы подадим адрес, который больше, чем может вместить WORD_ADDR_WIDTH, старшие биты отсекутся.
    std::cout << "Test 4: Address wrapping/aliasing" << std::endl;
    // Адрес, который точно такой же, как byte_addresses[1] по младшим WORD_ADDR_WIDTH битам индекса слова
    // (1 << (OFFSET_BITS + WORD_ADDR_WIDTH)) это размер всего нашего RAM в байтах
    uint32_t aliased_byte_addr = byte_addresses[1] + (NUM_WORDS * BYTES_PER_WORD);

    // Убедимся, что используем только ADDR_IN_WIDTH бит для адреса на входе модуля
    uint64_t input_adr_mask = (1ULL << ADDR_IN_WIDTH) - 1;
    top->adr = aliased_byte_addr & input_adr_mask;

    eval_read(top, tfp);
    std::cout << "  Reading from aliased byte_adr 0x" << std::hex << (aliased_byte_addr & input_adr_mask)
              << " (effective word index same as for 0x" << byte_addresses[1] << "): got 0x" << top->dout << std::dec << std::endl;
    assert(top->dout == test_values[1] && "Address aliasing read failed");


    std::cout << "RAM Testbench Finished Successfully!" << std::endl;

    if (tfp) {
        tfp->close();
    }
    delete top;
    exit(EXIT_SUCCESS);
}