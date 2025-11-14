#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <vector>
#include <locale>
#include <windows.h>

using namespace std;

// Глобальные переменные для синхронизации
mutex mtx;
atomic<bool> cook_can_work(true);
atomic<bool> fatmen_can_eat(false);
atomic<int> ready_dishes(0);

// Состояние системы
atomic<int> dish1, dish2, dish3;
atomic<int> fatman1_eaten, fatman2_eaten, fatman3_eaten;
atomic<int> days_passed;
atomic<bool> simulation_running;

void reset_simulation() {
    dish1 = dish2 = dish3 = 3000;
    fatman1_eaten = fatman2_eaten = fatman3_eaten = 0;
    days_passed = 0;
    simulation_running = true;
    cook_can_work = true;
    fatmen_can_eat = false;
    ready_dishes = 0;
}

void cook(int efficiency_factor) {
    auto start_time = chrono::steady_clock::now();

    while (simulation_running && days_passed < 5) {
        // Проверяем прошедшие дни
        auto current_time = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(current_time - start_time).count() >= 1) {
            days_passed++;
            start_time = current_time;
        }

        // Ждем разрешения на работу
        while (!cook_can_work && simulation_running) {
            this_thread::yield();
        }

        if (!simulation_running) break;

        mtx.lock();

        // Кук добавляет наггетсы
        dish1 += efficiency_factor;
        dish2 += efficiency_factor;
        dish3 += efficiency_factor;

        ready_dishes = 3; // Все тарелки готовы
        cook_can_work = false;
        fatmen_can_eat = true;

        mtx.unlock();

        // Небольшая задержка для имитации работы
        this_thread::sleep_for(chrono::microseconds(10));
    }
}

void fatman(int fatman_id, int gluttony) {
    atomic<int>& dish = (fatman_id == 1) ? dish1 : (fatman_id == 2) ? dish2 : dish3;
    atomic<int>& eaten = (fatman_id == 1) ? fatman1_eaten : (fatman_id == 2) ? fatman2_eaten : fatman3_eaten;

    while (simulation_running && eaten < 10000 && days_passed < 5) {
        // Ждем разрешения есть
        while (!fatmen_can_eat && simulation_running) {
            this_thread::yield();
        }

        if (!simulation_running) break;

        mtx.lock();

        if (ready_dishes > 0 && dish >= gluttony) {
            dish -= gluttony;
            eaten += gluttony;
            ready_dishes--;

            // Если все тарелки обработаны, разрешаем Куку работать
            if (ready_dishes == 0) {
                fatmen_can_eat = false;
                cook_can_work = true;
            }

            // Проверяем условия завершения
            if (dish <= 0) {
                simulation_running = false;
            }
            if (eaten >= 10000) {
                simulation_running = false;
            }
        }

        mtx.unlock();

        this_thread::sleep_for(chrono::microseconds(10));
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

    // Ограничиваем время выполнения (максимум 6 секунд)
    auto start = chrono::steady_clock::now();
    while (simulation_running) {
        if (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count() >= 6) {
            simulation_running = false;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    cook_thread.join();
    fatman1.join();
    fatman2.join();
    fatman3.join();

    // Сохраняем результаты
    result_days = days_passed;
    result_eaten1 = fatman1_eaten;
    result_eaten2 = fatman2_eaten;
    result_eaten3 = fatman3_eaten;
    result_dish1 = dish1;
    result_dish2 = dish2;
    result_dish3 = dish3;

    // Определяем исход
    bool all_burst = (fatman1_eaten >= 10000) && (fatman2_eaten >= 10000) && (fatman3_eaten >= 10000);
    bool any_empty = (dish1 <= 0) || (dish2 <= 0) || (dish3 <= 0);
    bool time_out = (days_passed >= 5);

    return (any_empty || all_burst || time_out);
}

void find_scenarios() {
    cout << "Poisk koefficientov dlya razlichnyh stsenariev..." << endl;
    cout << "=============================================" << endl;

    // Сценарий 1: Кука уволили (тарелка опустела)
    cout << "\n=== STsENARIY 1: Kuka uvolili ===" << endl;
    for (int eff = 1; eff <= 5; eff++) {
        for (int glut = 10; glut <= 20; glut++) {
            int days, eaten1, eaten2, eaten3, d1, d2, d3;
            if (run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3)) {
                bool empty_plate = (d1 <= 0) || (d2 <= 0) || (d3 <= 0);
                if (empty_plate && days < 5) {
                    cout << "Naideno: efficiency=" << eff << ", gluttony=" << glut << endl;
                    cout << "Rezultat: dney=" << days << ", sedeno: " << eaten1 << "," << eaten2 << "," << eaten3;
                    cout << ", ostalos: " << d1 << "," << d2 << "," << d3 << endl;
                    break;
                }
            }
        }
    }

    // Сценарий 2: Кук не получил зарплату (все толстяки лопнули)
    cout << "\n=== STsENARIY 2: Kuk ne poluchil zarplatu ===" << endl;
    for (int eff = 15; eff <= 30; eff++) {
        for (int glut = 15; glut <= 25; glut++) {
            int days, eaten1, eaten2, eaten3, d1, d2, d3;
            if (run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3)) {
                bool all_burst = (eaten1 >= 10000) && (eaten2 >= 10000) && (eaten3 >= 10000);
                if (all_burst && days < 5) {
                    cout << "Naideno: efficiency=" << eff << ", gluttony=" << glut << endl;
                    cout << "Rezultat: dney=" << days << ", sedeno: " << eaten1 << "," << eaten2 << "," << eaten3;
                    cout << ", ostalos: " << d1 << "," << d2 << "," << d3 << endl;
                    break;
                }
            }
        }
    }

    // Сценарий 3: Кук уволился сам (прошло 5 дней)
    cout << "\n=== STsENARIY 3: Kuk uvolilsya sam ===" << endl;
    for (int eff = 3; eff <= 8; eff++) {
        for (int glut = 3; glut <= 8; glut++) {
            int days, eaten1, eaten2, eaten3, d1, d2, d3;
            if (run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3)) {
                bool time_out = (days >= 5);
                bool plates_ok = (d1 > 0) && (d2 > 0) && (d3 > 0);
                bool not_burst = (eaten1 < 10000) && (eaten2 < 10000) && (eaten3 < 10000);

                if (time_out && plates_ok && not_burst) {
                    cout << "Naideno: efficiency=" << eff << ", gluttony=" << glut << endl;
                    cout << "Rezultat: dney=" << days << ", sedeno: " << eaten1 << "," << eaten2 << "," << eaten3;
                    cout << ", ostalos: " << d1 << "," << d2 << "," << d3 << endl;
                    break;
                }
            }
        }
    }
}

void demonstrate_scenarios() {
    cout << "\n\nDEMONSTRATsIYa STsENARIEV S NAIDENNYMI KOEFFITsIENTAMI" << endl;
    cout << "=============================================" << endl;

    // Демонстрация сценариев с подобранными коэффициентами
    struct Scenario {
        int eff;
        int glut;
        string description;
    };

    vector<Scenario> scenarios = {
        {2, 12, "Kuka uvolili"},
        {20, 18, "Kuk ne poluchil zarplatu"},
        {5, 5, "Kuk uvolilsya sam"}
    };

    for (const auto& scenario : scenarios) {
        int eff = scenario.eff;
        int glut = scenario.glut;
        string description = scenario.description;

        cout << "\n=== " << description << " ===" << endl;
        cout << "Koefficienty: efficiency=" << eff << ", gluttony=" << glut << endl;

        int days, eaten1, eaten2, eaten3, d1, d2, d3;
        run_simulation(eff, glut, days, eaten1, eaten2, eaten3, d1, d2, d3);

        cout << "Rezultaty:" << endl;
        cout << "- Dney proshlo: " << days << endl;
        cout << "- Sedeno tolstyakami: " << eaten1 << ", " << eaten2 << ", " << eaten3 << endl;
        cout << "- Ostalos v tarelkah: " << d1 << ", " << d2 << ", " << d3 << endl;

        if (d1 <= 0 || d2 <= 0 || d3 <= 0) {
            cout << "VYVOD: Kuka uvolili!" << endl;
        }
        else if (eaten1 >= 10000 && eaten2 >= 10000 && eaten3 >= 10000) {
            cout << "VYVOD: Kuk ne poluchil zarplatu!" << endl;
        }
        else if (days >= 5) {
            cout << "VYVOD: Kuk uvolilsya sam!" << endl;
        }
        cout << "-------------------" << endl;
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    cout << "LABORATORNAYa RABOTA No4: TRI TOLSTYAKA" << endl;
    cout << "Poisk optimalnyh koefficientov..." << endl;

    // Автоматический поиск коэффициентов
    find_scenarios();

    // Демонстрация работы с найденными коэффициентами
    demonstrate_scenarios();

    cout << "\nProgramma zavershena." << endl;
    system("pause");
    return 0;
}