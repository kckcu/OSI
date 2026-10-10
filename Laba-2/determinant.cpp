#include <iostream>
#include <vector>
#include <pthread.h>
#include <ctime>
#include <cstdlib>
#include <iomanip>

using Matrix = std::vector<std::vector<double>>;

// Структура задачи для потока
struct Task {
    Matrix mat;
    double coeff;
};

// Глобальные переменные для синхронизации
std::vector<Task> tasks;
int current_task_idx = 0;
double global_det_parallel = 0.0;

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция замера времени
double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Последовательное вычисление определителя матрицы
double det_seq(const Matrix& mat) {
    int n = mat.size();
    if (n == 1) return mat[0][0];
    if (n == 2) return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

    double d = 0.0;
    int sign = 1;
    for (int i = 0; i < n; i++) {
        Matrix sub(n - 1, std::vector<double>(n - 1));
        for (int j = 1; j < n; j++) {
            int col = 0;
            for (int k = 0; k < n; k++) {
                if (k == i) continue;
                sub[j - 1][col++] = mat[j][k];
            }
        }
        d += sign * mat[0][i] * det_seq(sub);
        sign = -sign;
    }
    return d;
}

// Рекурсивная генерация задач (дробим матрицу, пока размер не станет <= 7)
void generate_tasks(const Matrix& mat, double current_coeff) {
    int n = mat.size();
    if (n <= 7) {
        tasks.push_back({mat, current_coeff});
        return;
    }
    int sign = 1;
    for (int i = 0; i < n; i++) {
        Matrix sub(n - 1, std::vector<double>(n - 1));
        for (int j = 1; j < n; j++) {
            int col = 0;
            for (int k = 0; k < n; k++) {
                if (k == i) continue;
                sub[j - 1][col++] = mat[j][k];
            }
        }
        generate_tasks(sub, current_coeff * sign * mat[0][i]);
        sign = -sign;
    }
}

// Рабочая функция для POSIX потоков
void* worker(void* arg) {
    double local_sum = 0.0;

    while (true) {
        pthread_mutex_lock(&task_mutex);
        if (current_task_idx >= tasks.size()) {
            pthread_mutex_unlock(&task_mutex);
            break; // Задачи закончились
        }
        Task t = tasks[current_task_idx++];
        pthread_mutex_unlock(&task_mutex);

        // Вычисляем детерминант подматрицы
        local_sum += t.coeff * det_seq(t.mat);
    }

    // Добавляем локальную сумму в глобальный результат
    pthread_mutex_lock(&result_mutex);
    global_det_parallel += local_sum;
    pthread_mutex_unlock(&result_mutex);

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <num_threads>\n";
        return 1;
    }

    int num_threads = std::atoi(argv[1]);
    if (num_threads < 1) num_threads = 1;

    int N = 11; // Оптимальный размер для замера времени
    Matrix A(N, std::vector<double>(N));
    
    srand(42); // Фиксированный seed для одинаковой матрицы при каждом запуске
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (rand() % 21) - 10;
        }
    }

    std::cout << "Matrix size: " << N << "x" << N << ", Threads: " << num_threads << "\n";

    // --- ПОСЛЕДОВАТЕЛЬНЫЙ АЛГОРИТМ ---
    double start_seq = get_time_ms();
    double det_s = det_seq(A);
    double time_seq = get_time_ms() - start_seq;
    std::cout << "Sequential Det: " << std::fixed << std::setprecision(0) << det_s 
              << " | Time: " << time_seq << " ms\n";

    // --- ПАРАЛЛЕЛЬНЫЙ АЛГОРИТМ ---
    double start_par = get_time_ms();
    generate_tasks(A, 1.0);
    
    std::vector<pthread_t> threads(num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, worker, nullptr);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }
    double time_par = get_time_ms() - start_par;

    std::cout << "Parallel Det:   " << global_det_parallel 
              << " | Time: " << time_par << " ms\n";
    std::cout << "Tasks processed: " << tasks.size() << "\n";

    return 0;
}