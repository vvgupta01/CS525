#include "simulation.h"

#include "assert.h"
#include "utils.h"

Simulation::Simulation(uint32_t N, uint32_t T, int sigma) : N_(N), T_(T), sigma_(sigma) {
    assert(sigma >= 0 && sigma <= 100);

    utilizations_ = std::vector<float>(T);
    welfares_ = std::vector<float>(N);
    instant_fairness_ = std::vector<float>(T);

    lowest_accepted_ = std::vector<uint32_t>(T);
    tickets_ = std::vector<uint64_t>(T);
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

        utilizations_[t] = utilization(demands[t], allocations[t], alloc.get_num_blocks());
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

        utilizations_[t] = utilization(demands[t], allocations[t], alloc.get_num_blocks());
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

        utilizations_[t] = utilization(demands[t], allocations[t], alloc.get_num_blocks());
        instant_fairness_[t] = instant_fairness(demands[t], allocations[t], si);
        tickets_[t] = alloc.get_tickets();
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
    incentive_ = alt_welfare_ - selfish_welfare;

    avg_welfare_ = range_average(welfares_, 0, N_);
    avg_fairness_ = range_average(instant_fairness_, 0, T_);
}

void Simulation::output_sim(std::ostream& out, std::string label) {
    out << label << "," << sigma_ << "," << avg_utilization_ << ","
        << avg_welfare_ << "," << incentive_ << ","
        << fairness_ << "," << avg_fairness_ << std::endl;
}

void Simulation::output_auction(std::ostream& out) {
    for (uint32_t t = 0; t < T_; ++t) {
        out << lowest_accepted_[t] << std::endl;
    }
}

void Simulation::output_barter(std::ostream& out) {
    for (uint32_t t = 0; t < T_; ++t) {
        out << tickets_[t] << std::endl;
    }
}