#include "simulation.h"

#include "assert.h"
#include "utils.h"

Simulation::Simulation(uint32_t N, uint32_t T, int sigma) : N_(N), T_(T), sigma_(sigma) {
    assert(sigma >= 0 && sigma <= 100);

    utilizations_ = std::vector<float>(T);
    welfares_ = std::vector<float>(N);
    instant_fairness_ = std::vector<float>(T);
    lowest_accepted_ = std::vector<uint32_t>(T);
}

void Simulation::simulate(Allocator& alloc, matrix& demands) {
    size_t si = sigma_ / 100.0 * N_;
    matrix allocations(T_, std::vector<uint32_t>(N_));

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
        utilizations_[t] = utilization(demands[t], allocations[t], alloc.get_total_blocks());
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t], si);
    }
    avg_utilization_ = range_average(utilizations_, 0, T_);
    welfares_ = welfares(demands, allocations);
    fairness_ = fairness(welfares_, si);

    float alt_welfare_ = range_average(welfares_, si, N_);
    float selfish_welfare = range_average(welfares_, 0, si);

    if (alt_welfare_ == 0) {
        alt_welfare_ = selfish_welfare;
    } else if (selfish_welfare == 0) {
        selfish_welfare = alt_welfare_;
    }
    welfare_disparity_ = alt_welfare_ - selfish_welfare;

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
        for (uint32_t i = 1; i <= N_; ++i) {
            alloc.set_demand(i, demands[t][i - 1], i <= si);
        }

        alloc.allocate();

        for (uint32_t i = 1; i <= N_; ++i) {
            allocations[t][i - 1] = alloc.get_allocation(i);
            payments[t][i - 1] = alloc.get_payment(i);
        }

        utilizations_[t] = utilization(demands[t], allocations[t], alloc.get_total_blocks());
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t], payments[t], alloc.get_valuation(), si);
        lowest_accepted_[t] = alloc.get_border_bids().first;
    }
    avg_utilization_ = range_average(utilizations_, 0, T_);
    welfares_ = welfares(demands, allocations, payments, alloc.get_valuation());
    fairness_ = fairness(welfares_, si);

    float alt_welfare_ = range_average(welfares_, si, N_);
    float selfish_welfare = range_average(welfares_, 0, si);

    if (alt_welfare_ == 0) {
        alt_welfare_ = selfish_welfare;
    } else if (selfish_welfare == 0) {
        selfish_welfare = alt_welfare_;
    }
    welfare_disparity_ = alt_welfare_ - selfish_welfare;

    avg_welfare_ = range_average(welfares_, 0, N_);
    avg_fairness_ = range_average(instant_fairness_, 0, T_);
}

void Simulation::output_sim(std::ostream& out, std::string label) {
    out << label << "," << sigma_ << "," << avg_utilization_ << ","
        << avg_welfare_ << "," << welfare_disparity_ << ","
        << fairness_ << "," << avg_fairness_ << std::endl;
}

void Simulation::output_auction(std::ostream& out) {
    for (uint32_t t = 0; t < T_; ++t) {
        out << lowest_accepted_[t] << std::endl;
    }
}
