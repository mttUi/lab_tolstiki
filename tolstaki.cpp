#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <locale>
#include <windows.h>

using namespace std;

mutex mtx;
bool cook_can_work = true;
bool fatmen_can_eat = false;
int ready_dishes = 0;

int dish1, dish2, dish3;
int fatman1_eaten, fatman2_eaten, fatman3_eaten;
int days_passed;
bool simulation_running;
int operations_count;

void reset_simulation() {
    dish1 = dish2 = dish3 = 3000;
    fatman1_eaten = fatman2_eaten = fatman3_eaten = 0;
    days_passed = 0;
    simulation_running = true;
    cook_can_work = true;
    fatmen_can_eat = false;
    ready_dishes = 0;
    operations_count = 0;
}

void cook(int efficiency_factor) {
    while (simulation_running && days_passed < 5) {
        // Ожидание разрешения на работу
        while (!cook_can_work && simulation_running) {
            this_thread::yield();
        }

        if (!simulation_running) break;

        // Критическая секция - работа повара
        mtx.lock();

        dish1 += efficiency_factor;
        dish2 += efficiency_factor;
        dish3 += efficiency_factor;

        ready_dishes = 3;
        cook_can_work = false;
        fatmen_can_eat = true;

        mtx.unlock();

        this_thread::sleep_for(chrono::microseconds(100));
    }
}

void fatman(int fatman_id, int gluttony) {
    int& dish = (fatman_id == 1) ? dish1 : (fatman_id == 2) ? dish2 : dish3;
    int& eaten = (fatman_id == 1) ? fatman1_eaten : (fatman_id == 2) ? fatman2_eaten : fatman3_eaten;

    while (simulation_running && eaten < 10000 && days_passed < 5) {
        // Ожидание разрешения на еду
        while (!fatmen_can_eat && simulation_running) {
            this_thread::yield();
        }

        if (!simulation_running) break;

        // Критическая секция - еда толстяка
        mtx.lock();

        // Обновляем дни на основе операций (каждые 10 операций = 1 день)
        operations_count++;
        if (operations_count >= 10) {
            days_passed++;
            operations_count = 0;
        }

        if (ready_dishes > 0 && dish >= gluttony) {
            dish -= gluttony;
            eaten += gluttony;
            ready_dishes--;

            // Если все тарелки обработаны, передаем ход повару
            if (ready_dishes == 0) {
                fatmen_can_eat = false;
                cook_can_work = true;
            }

            // Проверка условий завершения
            if (dish <= 0 || eaten >= 10000) {
                simulation_running = false;
            }
        }

        mtx.unlock();

        this_thread::sleep_for(chrono::microseconds(100));
    }
}

bool run_simulation(int efficiency_factor, int gluttony, int& result_days,
    int& result_eaten1, int& result_eaten2, int& result_eaten3,
    int& result_dish1, int& result_dish2, int& result_dish3) {
    reset_simulation();

    thread cook_thread(cook, efficiency_factor);
    thread fatman1(fatman, 1, gluttony);
    thread fatman2(fatman, 2, gluttony);
    thread fatman3(fatman, 3, gluttony);

    // Ждем завершения симуляции
    auto start_time = chrono::steady_clock::now();
    while (simulation_running) {
        auto current_time = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(current_time - start_time);

        if (elapsed.count() > 2000) { // 2 секунды максимум
            simulation_running = false;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    cook_thread.join();
    fatman1.join();
    fatman2.join();
    fatman3.join();

    auto end_time = chrono::steady_clock::now();
    auto total_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    result_days = days_passed;
    result_eaten1 = fatman1_eaten;
    result_eaten2 = fatman2_eaten;
    result_eaten3 = fatman3_eaten;
    result_dish1 = dish1;
    result_dish2 = dish2;
    result_dish3 = dish3;

    cout << "[Время: " << total_time.count() << " мс] ";

    bool all_burst = (fatman1_eaten >= 10000) && (fatman2_eaten >= 10000) && (fatman3_eaten >= 10000);
    bool any_empty = (dish1 <= 0) || (dish2 <= 0) || (dish3 <= 0);
    bool time_out = (days_passed >= 5);

    return (any_empty || all_burst || time_out);
}

void find_scenarios() {
    cout << "Поиск коэффициентов для различных сценариев..." << endl;
    cout << "=============================================" << endl;

    cout << "\n=== СЦЕНАРИЙ 1: Кука уволили ===" << endl;
    // Маленькая эффективность, большое обжорство - тарелки пустеют быстро
    vector<pair<int, int>> tests = { {5, 100}, {10, 80}, {15, 120} };

    for (const auto& test : tests) {
        int eff = test.first;
        int glut = test.second;
        int days, eaten1, eaten2, eaten3, d1, d2, d3;

        cout << "Тест: eff=" << eff << ", glut=" << glut << " - ";
        if (run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3)) {
            bool empty_plate = (d1 <= 0) || (d2 <= 0) || (d3 <= 0);
            if (empty_plate && days < 5) {
                cout << "УСПЕХ! Тарелка пустая за " << days << " дней" << endl;
                cout << "   Съедено: " << eaten1 << ", " << eaten2 << ", " << eaten3 << endl;
                cout << "   Осталось: " << d1 << ", " << d2 << ", " << d3 << endl;
                break;
            }
            else {
                cout << "не подходит (дни: " << days << ", остаток: " << d1 << ")" << endl;
            }
        }
    }

    cout << "\n=== СЦЕНАРИЙ 2: Кук не получил зарплату ===" << endl;
    // Большая эффективность, среднее обжорство - толстяки быстро наедают 10000
    tests = { {100, 50}, {80, 60}, {120, 40} };

    for (const auto& test : tests) {
        int eff = test.first;
        int glut = test.second;
        int days, eaten1, eaten2, eaten3, d1, d2, d3;

        cout << "Тест: eff=" << eff << ", glut=" << glut << " - ";
        if (run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3)) {
            bool all_burst = (eaten1 >= 10000) && (eaten2 >= 10000) && (eaten3 >= 10000);
            if (all_burst && days < 5) {
                cout << "УСПЕХ! Все лопнули за " << days << " дней" << endl;
                cout << "   Съедено: " << eaten1 << ", " << eaten2 << ", " << eaten3 << endl;
                break;
            }
            else {
                cout << "не подходит (дни: " << days << ", съедено: " << eaten1 << ")" << endl;
            }
        }
    }

    cout << "\n=== СЦЕНАРИЙ 3: Кук уволился сам ===" << endl;
    // Сбалансированные значения - успевают за 5 дней
    tests = { {20, 10}, {25, 8}, {30, 12} };

    for (const auto& test : tests) {
        int eff = test.first;
        int glut = test.second;
        int days, eaten1, eaten2, eaten3, d1, d2, d3;

        cout << "Тест: eff=" << eff << ", glut=" << glut << " - ";
        if (run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3)) {
            bool time_out = (days >= 5);
            bool plates_ok = (d1 > 0) && (d2 > 0) && (d3 > 0);
            bool not_burst = (eaten1 < 10000) && (eaten2 < 10000) && (eaten3 < 10000);

            if (time_out && plates_ok && not_burst) {
                cout << "УСПЕХ! Прошло 5 дней, все нормально" << endl;
                cout << "   Съедено: " << eaten1 << ", " << eaten2 << ", " << eaten3 << endl;
                cout << "   Осталось: " << d1 << ", " << d2 << ", " << d3 << endl;
                break;
            }
            else {
                cout << "не подходит (дни: " << days << ", съедено: " << eaten1 << ")" << endl;
            }
        }
    }
}

void demonstrate_scenarios() {
    cout << "\n\nДЕМОНСТРАЦИЯ СЦЕНАРИЕВ" << endl;
    cout << "=============================================" << endl;

    struct Scenario {
        int eff;
        int glut;
        string description;
        string expected;
    };

    vector<Scenario> scenarios = {
        {10, 80, "СЦЕНАРИЙ 1: Кука уволили", "Тарелка пустая до 5 дней"},
        {100, 50, "СЦЕНАРИЙ 2: Кук не получил зарплату", "Все толстяки лопнули до 5 дней"},
        {25, 8, "СЦЕНАРИЙ 3: Кук уволился сам", "Прошло 5 дней, все в норме"}
    };

    for (const auto& scenario : scenarios) {
        int eff = scenario.eff;
        int glut = scenario.glut;
        string description = scenario.description;
        string expected = scenario.expected;

        cout << "\n" << description << endl;
        cout << "Ожидается: " << expected << endl;
        cout << "Коэффициенты: efficiency=" << eff << ", gluttony=" << glut << endl;

        int days, eaten1, eaten2, eaten3, d1, d2, d3;
        run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3);

        cout << "Фактические результаты:" << endl;
        cout << "- Дней прошло: " << days << endl;
        cout << "- Съедено толстяками: " << eaten1 << ", " << eaten2 << ", " << eaten3 << endl;
        cout << "- Осталось в тарелках: " << d1 << ", " << d2 << ", " << d3 << endl;

        // Анализ результата
        if (d1 <= 0 || d2 <= 0 || d3 <= 0) {
            cout << "✓ ВЫВОД: Кука уволили! (тарелка пустая)" << endl;
        }
        else if (eaten1 >= 10000 && eaten2 >= 10000 && eaten3 >= 10000) {
            cout << "✓ ВЫВОД: Кук не получил зарплату! (все толстяки лопнули)" << endl;
        }
        else if (days >= 5) {
            cout << "✓ ВЫВОД: Кук уволился сам! (прошло 5 дней)" << endl;
        }
        else {
            cout << "? ВЫВОД: Неопределенный результат" << endl;
        }
        cout << "-------------------" << endl;
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    cout << "ЛАБОРАТОРНАЯ РАБОТА №4: ТРИ ТОЛСТЯКА" << endl;
    cout << "Поиск оптимальных коэффициентов..." << endl;

    find_scenarios();

    demonstrate_scenarios();

    cout << "\nПрограмма завершена." << endl;
    system("pause");
    return 0;
}