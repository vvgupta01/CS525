#include <random>
#include <vector>

#include "types.h"

bool rand_bool();

int rand_uniform(int min, int max);

std::discrete_distribution<> get_rand_discrete(std::vector<uint32_t>& weights);

int sample_rand_discrete(std::discrete_distribution<>& dist);

matrix generate_uniform_demands(uint32_t N, uint32_t T, uint32_t max_demand);

std::vector<float> welfares(matrix& demands, matrix& allocations);

std::vector<float> welfares(matrix& demands, matrix& allocations,
                            matrix& payments, fi valuation);

float fairness(std::vector<float>& welfares, size_t si);

// float fairness(matrix& demands, matrix& allocations);

float instant_fairness(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations, size_t si);

float instant_fairness(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations,
                       std::vector<uint32_t>& payments, fi valuation, size_t si);

float utilization(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations, uint64_t blocks);

float range_average(std::vector<float>& arr, size_t a, size_t b);