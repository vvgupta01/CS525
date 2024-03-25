#include <assert.h>

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

// demands and allocations of a tenant at each quanta
float welfare(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations) {
    assert(demands.size() == allocations.size());

    uint64_t used = 0, total_demand = 0;
    for (size_t i = 0; i < demands.size(); ++i) {
        used += std::min(demands[i], allocations[i]);
        total_demand += demands[i];
    }

    if (total_demand == 0) {
        return 1;
    }
    return (float)used / total_demand;
}

float fairness(std::vector<float>& welfares) {
    auto minmax = std::minmax_element(welfares.begin(), welfares.end());
    if (*minmax.second == 0) {
        return 1;
    }
    return *minmax.first / *minmax.second;
}

// demands and allocations of all tenants at each quanta
float fairness(std::vector<std::vector<uint32_t>>& demands, std::vector<std::vector<uint32_t>>& allocations) {
    uint64_t min_used = std::numeric_limits<uint64_t>::max(), max_used = 0;

    for (size_t i = 0; i < demands[0].size(); ++i) {
        uint64_t used = 0;
        for (size_t t = 0; t < demands.size(); ++t) {
            used += std::min(demands[t][i], allocations[t][i]);
        }
        min_used = std::min(min_used, used);
        max_used = std::max(max_used, used);
    }

    if (max_used == 0) {
        return 1;
    }
    return (float)min_used / max_used;
}

// demands and allocations of all tenants at a given quanta
float instant_fairness(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations) {
    assert(demands.size() == allocations.size());

    float min_welfare = 1, max_welfare = 0;
    for (size_t i = 0; i < demands.size(); ++i) {
        float welfare = 1;
        if (demands[i] > 0) {
            // instantaneous welfare
            welfare = (float)std::min(demands[i], allocations[i]) / demands[i];
        }

        min_welfare = std::min(min_welfare, welfare);
        max_welfare = std::max(max_welfare, welfare);
    }

    if (max_welfare == 0) {
        return 1;
    }
    return min_welfare / max_welfare;
}

// demands and allocations of all tenants at a given quanta
float instant_utilization(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations, uint64_t blocks) {
    assert(blocks > 0);
    assert(demands.size() == allocations.size());

    uint64_t used = 0;
    for (size_t i = 0; i < demands.size(); ++i) {
        used += std::min(demands[i], allocations[i]);
    }
    return (float)used / blocks;
}
