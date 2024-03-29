#include <functional>
#include <vector>

#include "allocator/allocator.h"

typedef std::vector<std::vector<uint32_t>> matrix;

struct Simulation {
    uint32_t N_, T_;
    float sigma_;

    std::vector<float> utilizations_, welfares_, instant_fairness_;
    float avg_utilization_ = 0, avg_fairness_ = 0, fairness_ = 0;
    float avg_welfare_ = 0, welfare_disparity_ = 0;

    double avg_runtime_ = 0;

    Simulation(uint32_t N, uint32_t T, float sigma);

    void simulate(Allocator& alloc, matrix& demands);

    void benchmark(Allocator& alloc, matrix& demands);

    void output_sim(std::ostream& out, std::string label);

    void output_bench(std::ostream& out, std::string label);
};