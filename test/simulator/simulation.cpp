#include "simulation.h"

#include "assert.h"
#include "utils.h"

Simulation::Simulation(uint32_t N, uint32_t T, int sigma) : N_(N), T_(T), sigma_(sigma) {
    assert(sigma >= 0 && sigma <= 100);

    welfares_ = std::vector<double>(N);
    instant_fairness_ = std::vector<double>(T);
}

void Simulation::simulate(Allocator& alloc, matrix& demands) {
    size_t si = sigma_ / 100.0 * N_;
    matrix allocations(T_, std::vector<uint32_t>(N_));

    for (uint32_t i = 1; i <= N_; ++i) {
        alloc.add_tenant(i);
    }

    for (uint32_t t = 0; t < T_; ++t) {
        uint64_t total_demand = 0, total_alloc = 0;
        for (uint32_t i = 1; i <= N_; ++i) {
            alloc.set_demand(i, demands[t][i - 1], i <= si);
            total_demand += demands[t][i - 1];
        }

        alloc.allocate();

        for (uint32_t i = 1; i <= N_; ++i) {
            allocations[t][i - 1] = alloc.get_allocation(i);
            total_alloc += allocations[t][i - 1];

            // std::cout << allocations[t][i - 1] << "/" << demands[t][i - 1] << ",";
        }
        // std::cout << " -> " << total_alloc << "/" << total_demand << std::endl;
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t], si);
    }
    utilization_ = utilization(demands, allocations, alloc.get_num_blocks());
    welfares_ = welfares(demands, allocations);
    fairness_ = fairness(welfares_, si);

    double alt_welfare_ = range_average(welfares_, si, N_);
    double selfish_welfare = range_average(welfares_, 0, si);

    if (alt_welfare_ == 0) {
        alt_welfare_ = selfish_welfare;
    } else if (selfish_welfare == 0) {
        selfish_welfare = alt_welfare_;
    }
    incentive_ = alt_welfare_ - selfish_welfare;

    avg_welfare_ = range_average(welfares_, 0, N_);
    avg_fairness_ = range_average(instant_fairness_, 0, T_);
}

void Simulation::simulate(MPSPAllocator& alloc, matrix& demands) {
    size_t si = sigma_ / 100.0 * N_;
    matrix allocations(T_, std::vector<uint32_t>(N_));
    matrix payments = allocations;

    for (uint32_t i = 1; i <= N_; ++i) {
        alloc.add_tenant(i);
    }

    for (uint32_t t = 0; t < T_; ++t) {
        uint64_t total_demand = 0, total_alloc = 0;
        for (uint32_t i = 1; i <= N_; ++i) {
            alloc.set_demand(i, demands[t][i - 1], i <= si);
            total_demand += demands[t][i - 1];
        }

        alloc.allocate();

        for (uint32_t i = 1; i <= N_; ++i) {
            allocations[t][i - 1] = alloc.get_allocation(i);
            payments[t][i - 1] = alloc.get_payment(i);
            total_alloc += allocations[t][i - 1];

            // std::cout << allocations[t][i - 1] << "/" << demands[t][i - 1] << "@" << payments[t][i - 1] << ",";
        }
        // std::cout << " -> " << total_alloc << "/" << total_demand << std::endl;
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t], payments[t], alloc.get_valuation(), si);
    }
    utilization_ = utilization(demands, allocations, alloc.get_num_blocks());
    welfares_ = welfares(demands, allocations, payments, alloc.get_valuation());
    fairness_ = fairness(welfares_, si);

    double alt_welfare_ = range_average(welfares_, si, N_);
    double selfish_welfare = range_average(welfares_, 0, si);

    if (alt_welfare_ == 0) {
        alt_welfare_ = selfish_welfare;
    } else if (selfish_welfare == 0) {
        selfish_welfare = alt_welfare_;
    }
    incentive_ = alt_welfare_ - selfish_welfare;

    avg_welfare_ = range_average(welfares_, 0, N_);
    avg_fairness_ = range_average(instant_fairness_, 0, T_);
}

void Simulation::simulate(SharpAllocator& alloc, matrix& demands) {
    size_t si = sigma_ / 100.0 * N_;
    matrix allocations(T_, std::vector<uint32_t>(N_));
    matrix payments = allocations;

    for (uint32_t i = 1; i <= N_; ++i) {
        alloc.add_tenant(i);
    }

    for (uint32_t t = 0; t < T_; ++t) {
        for (uint32_t i = 1; i <= N_; ++i) {
            alloc.set_demand(i, demands[t][i - 1], i <= si);
        }

        alloc.allocate();

        for (uint32_t i = 1; i <= N_; ++i) {
            allocations[t][i - 1] = alloc.get_allocation(i);
        }
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t], si);
    }
    utilization_ = utilization(demands, allocations, alloc.get_num_blocks());
    welfares_ = welfares(demands, allocations);
    fairness_ = fairness(welfares_, si);

    double alt_welfare_ = range_average(welfares_, si, N_);
    double selfish_welfare = range_average(welfares_, 0, si);

    if (alt_welfare_ == 0) {
        alt_welfare_ = selfish_welfare;
    } else if (selfish_welfare == 0) {
        selfish_welfare = alt_welfare_;
    }
    incentive_ = alt_welfare_ - selfish_welfare;

    avg_welfare_ = range_average(welfares_, 0, N_);
    avg_fairness_ = range_average(instant_fairness_, 0, T_);
}

void Simulation::output_sim(std::ostream& out, std::string label) {
    out << label << "," << sigma_ << "," << utilization_ << ","
        << avg_welfare_ << "," << incentive_ << ","
        << fairness_ << "," << avg_fairness_ << std::endl;
}
