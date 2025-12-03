#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <windows.h>
#include <locale>

using namespace std::chrono_literals;

std::mutex kitchen_lock;
std::atomic<bool> chef_active{ true };
std::atomic<bool> eaters_active{ false };
std::atomic<int> servings_ready{ 0 };

int bowl_A, bowl_B, bowl_C;
int eater_A_total, eater_B_total, eater_C_total;
std::atomic<bool> simulation_active;
std::atomic<bool> scenario_finished;

void initialize_scene() {
    bowl_A = bowl_B = bowl_C = 3000;
    eater_A_total = eater_B_total = eater_C_total = 0;
    simulation_active = true;
    scenario_finished = false;
    chef_active = true;
    eaters_active = false;
    servings_ready = 0;
}

void chef_routine(int productivity) {
    while (simulation_active && !scenario_finished) {
        while (!chef_active && simulation_active && !scenario_finished) {
            std::this_thread::yield();
        }

        if (!simulation_active || scenario_finished) return;

        std::lock_guard<std::mutex> guard(kitchen_lock);

        bowl_A += productivity;
        bowl_B += productivity;
        bowl_C += productivity;

        servings_ready = 3;
        chef_active = false;
        eaters_active = true;
    }
}

void eater_routine(int eater_id, int appetite) {
    int* target_bowl = nullptr;
    int* consumption_record = nullptr;

    if (eater_id == 1) {
        target_bowl = &bowl_A;
        consumption_record = &eater_A_total;
    }
    else if (eater_id == 2) {
        target_bowl = &bowl_B;
        consumption_record = &eater_B_total;
    }
    else {
        target_bowl = &bowl_C;
        consumption_record = &eater_C_total;
    }

    while (simulation_active && !scenario_finished) {
        while (!eaters_active && simulation_active && !scenario_finished) {
            std::this_thread::yield();
        }

        if (!simulation_active || scenario_finished) return;

        {
            std::lock_guard<std::mutex> guard(kitchen_lock);

            bool empty_bowl_exists = (bowl_A <= 0) || (bowl_B <= 0) || (bowl_C <= 0);
            bool all_overfull = (eater_A_total >= 10000) && (eater_B_total >= 10000) && (eater_C_total >= 10000);

            if (empty_bowl_exists || all_overfull) {
                scenario_finished = true;
                return;
            }

            if (servings_ready > 0 && *target_bowl >= appetite) {
                *target_bowl -= appetite;
                *consumption_record += appetite;
                servings_ready--;

                if (servings_ready == 0) {
                    eaters_active = false;
                    chef_active = true;
                }
            }
        }
    }
}

void execute_scenario(int chef_productivity, int eater_appetite,
    const std::string& title, int expected_outcome) {
    initialize_scene();

    std::cout << "\n■■■■ " << title << " ■■■■" << std::endl;
    std::cout << "Параметры: продуктивность=" << chef_productivity
        << ", аппетит=" << eater_appetite << std::endl;

    std::thread chef(chef_routine, chef_productivity);
    std::thread eater1(eater_routine, 1, eater_appetite);
    std::thread eater2(eater_routine, 2, eater_appetite);
    std::thread eater3(eater_routine, 3, eater_appetite);

    auto scenario_start = std::chrono::steady_clock::now();
    int elapsed = 0;

    while (simulation_active && !scenario_finished && elapsed < 5) {
        auto current_time = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            current_time - scenario_start).count();
        std::this_thread::sleep_for(10ms);
    }

    simulation_active = false;
    scenario_finished = true;

    chef.join();
    eater1.join();
    eater2.join();
    eater3.join();

    auto scenario_end = std::chrono::steady_clock::now();
    auto time_taken = std::chrono::duration_cast<std::chrono::seconds>(
        scenario_end - scenario_start);

    std::cout << "Итоги через " << time_taken.count() << " секунд:" << std::endl;
    std::cout << "• Потреблено едоками: " << eater_A_total << ", "
        << eater_B_total << ", " << eater_C_total << std::endl;
    std::cout << "• Остаток в мисках: " << bowl_A << ", "
        << bowl_B << ", " << bowl_C << std::endl;

    bool empty_bowl = (bowl_A <= 0) || (bowl_B <= 0) || (bowl_C <= 0);
    bool all_full = (eater_A_total >= 10000) && (eater_B_total >= 10000) && (eater_C_total >= 10000);
    bool timeout_reached = (time_taken.count() >= 5);

    bool scenario_success = false;

    if (expected_outcome == 1) {
        scenario_success = empty_bowl && !timeout_reached;
        std::cout << "РЕЗУЛЬТАТ: "
            << (scenario_success ? "✅ Шефа уволили! (миска опустела до 5 дней)"
                : "❌ Цель не достигнута") << std::endl;
    }
    else if (expected_outcome == 2) {
        scenario_success = all_full && !timeout_reached;
        std::cout << "РЕЗУЛЬТАТ: "
            << (scenario_success ? "✅ Шеф без зарплаты! (все наелись до 5 дней)"
                : "❌ Цель не достигнута") << std::endl;
    }
    else if (expected_outcome == 3) {
        scenario_success = timeout_reached && !empty_bowl && !all_full;
        std::cout << "РЕЗУЛЬТАТ: "
            << (scenario_success ? "✅ Шеф ушел сам! (5 дней прошло, всё стабильно)"
                : "❌ Цель не достигнута") << std::endl;
    }

    if (!scenario_success) {
        std::cout << "   (Причины: ";
        if (timeout_reached) std::cout << "время истекло, ";
        if (empty_bowl) std::cout << "миска пуста, ";
        if (all_full) std::cout << "все насытились";
        std::cout << ")" << std::endl;
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    std::locale::global(std::locale("Russian"));

    std::cout << "ЛАБОРАТОРНАЯ РАБОТА №4: ТРИ ОБЖОРЫ" << std::endl;
    std::cout << "Демонстрация различных ситуаций..." << std::endl;

    // Ситуация 1: Шефа увольняют - миска быстро пустеет
    execute_scenario(5, 150, "СИТУАЦИЯ 1: Увольнение шефа", 1);

    // Ситуация 2: Шеф без оплаты - все быстро наедаются
    execute_scenario(300, 250, "СИТУАЦИЯ 2: Без оплаты труда", 2);

    // Ситуация 3: Шеф уходит сам - всё сбалансировано
    execute_scenario(20, 15, "СИТУАЦИЯ 3: Добровольный уход", 3);

    std::cout << "\nЗавершение программы." << std::endl;
    return 0;
}