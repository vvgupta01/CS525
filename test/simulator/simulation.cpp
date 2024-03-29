#include "simulation.h"

#include <chrono>

#include "utils.h"

Simulation::Simulation(uint32_t N, uint32_t T, float sigma) : N_(N), T_(T), sigma_(sigma) {
    utilizations_ = std::vector<float>(T);
    welfares_ = std::vector<float>(N);
    instant_fairness_ = std::vector<float>(T);
}

void Simulation::simulate(Allocator& alloc, matrix& demands) {
    matrix allocations(T_, std::vector<uint32_t>(N_));

    for (uint32_t i = 1; i <= N_; ++i) {
        alloc.add_tenant(i);
    }

    for (uint32_t t = 0; t < T_; ++t) {
        for (uint32_t i = 1; i <= N_; ++i) {
            alloc.set_demand(i, demands[t][i - 1]);
        }

        alloc.allocate();

        for (uint32_t i = 1; i <= N_; ++i) {
            allocations[t][i - 1] = alloc.get_allocation(i);
        }
        utilizations_[t] = utilization(demands[t], allocations[t], alloc.get_total_blocks());
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t]);
    }
    avg_utilization_ = range_average(utilizations_, 0, T_);
    welfares_ = welfares(demands, allocations);
    fairness_ = fairness(welfares_);

    size_t si = N_ * sigma_;
    float alt_welfare_ = range_average(welfares_, si, N_);
    float selfish_welfare = range_average(welfares_, 0, si);
    welfare_disparity_ = alt_welfare_ - selfish_welfare;

    avg_welfare_ = (alt_welfare_ + selfish_welfare) / 2;
    avg_fairness_ = range_average(instant_fairness_, 0, T_);
    avg_runtime_ /= T_;
}

void Simulation::benchmark(Allocator& alloc, matrix& demands) {
    avg_runtime_ = 0;
    for (uint32_t i = 1; i <= N_; ++i) {
        alloc.add_tenant(i);
    }

    for (uint32_t t = 0; t < T_; ++t) {
        for (uint32_t i = 1; i <= N_; ++i) {
            alloc.set_demand(i, demands[t][i - 1]);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        alloc.allocate();
        auto t2 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> time = t2 - t1;
        avg_runtime_ += time.count();
    }
    avg_runtime_ /= T_;
}

void Simulation::output_sim(std::ostream& out, std::string label) {
    out << label << "," << sigma_ << "," << avg_utilization_ << ","
        << avg_welfare_ << "," << welfare_disparity_ << ","
        << fairness_ << "," << avg_fairness_ << std::endl;
}

void Simulation::output_bench(std::ostream& out, std::string label) {
    out << label << "," << N_ << "," << avg_runtime_ << std::endl;
}