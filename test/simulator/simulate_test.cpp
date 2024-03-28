#include <numeric>
#include <vector>

#include "allocator/karma.h"
#include "allocator/maxmin.h"
#include "allocator/static.h"
#include "assert.h"
#include "utils.h"

struct SimulationResult {
    float avg_utilization = 0, fairness = 0;
    std::vector<float> welfares, instant_fairness;

    SimulationResult(uint32_t N, uint32_t T) {
        welfares = std::vector<float>(N);
        instant_fairness = std::vector<float>(T);
    }
};

SimulationResult simulate(Allocator& alloc, uint32_t N, uint32_t T, std::vector<std::vector<uint32_t>>& demands) {
    SimulationResult res(N, T);
    std::vector<std::vector<uint32_t>> allocations(T, std::vector<uint32_t>(N));

    for (uint32_t i = 1; i <= N; ++i) {
        alloc.add_tenant(i);
    }

    for (uint32_t t = 0; t < T; ++t) {
        for (uint32_t i = 1; i <= N; ++i) {
            alloc.set_demand(i, demands[t][i - 1]);
        }

        alloc.allocate();

        for (uint32_t i = 1; i <= N; ++i) {
            allocations[t][i - 1] = alloc.get_allocation(i);
        }
        res.avg_utilization += utilization(demands[t], allocations[t], alloc.get_total_blocks());
        res.instant_fairness[t] = instant_fairness(demands[t], allocations[t]);
    }
    res.avg_utilization /= T;
    res.welfares = welfares(demands, allocations);
    res.fairness = fairness(res.welfares);
    return res;
}

std::vector<std::vector<uint32_t>> generate_demands(uint32_t N, uint32_t T, float sigma, uint32_t fair_share) {
    std::vector<std::vector<uint32_t>> demands(T, std::vector<uint32_t>(N));

    uint32_t si = sigma * N;
    for (uint32_t t = 0; t < T; ++t) {
        for (uint32_t i = 0; i < N; ++i) {
            uint32_t demand = std::rand() % 21;
            if (i < si) {
                demand = std::min(demand, fair_share);
            }
            demands[t][i] = demand;
        }
    }
    return demands;
}

void output_simulation_results(SimulationResult& res, std::string label) {
    std::cout << std::string(50, '-') << std::endl
              << label << std::endl
              << std::string(50, '-') << std::endl;
    std::cout << "avg utilization: " << res.avg_utilization << std::endl;
    std::cout << "long-term fairness: " << res.fairness << std::endl;

    float avg_welfare = std::accumulate(res.welfares.begin(), res.welfares.end(), 0.0) / res.welfares.size();
    std::cout << "average welfare: " << avg_welfare << std::endl;

    float avg_fairness = std::accumulate(res.instant_fairness.begin(),
                                         res.instant_fairness.end(), 0.0) /
                         res.instant_fairness.size();
    std::cout << "average instant-fairness: " << avg_fairness << std::endl;
}

int main() {
    uint32_t N = 10, T = 1000;
    auto demands = generate_demands(N, T, 0, 10);

    StaticAllocator static_alloc(100);
    MaxMinAllocator maxmin_alloc(100);
    KarmaAllocator karma(100, 0.5, 100000);

    SimulationResult res = simulate(static_alloc, N, T, demands);
    output_simulation_results(res, "static (N=10, T=1000, sigma=0, f=10)");

    res = simulate(maxmin_alloc, N, T, demands);
    output_simulation_results(res, "maxmin (N=10, T=1000, sigma=0, f=10)");

    res = simulate(karma, N, T, demands);
    output_simulation_results(res, "karma (N=10, T=1000, sigma=0, f=10)");
}