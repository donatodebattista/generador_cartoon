#ifndef CARTOON_TIMER_HPP
#define CARTOON_TIMER_HPP

#include <chrono>

/**
 * @brief Temporizador de alta precisión para benchmarking de etapas de procesamiento.
 */
class Timer {
public:
    Timer() { reset(); }

    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Retorna el tiempo transcurrido en milisegundos con precisión decimal.
     */
    double elapsed_ms() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time_;
        return duration.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

#endif // CARTOON_TIMER_HPP
