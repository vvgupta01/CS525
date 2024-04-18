#include <functional>
#include <vector>

typedef std::vector<std::vector<uint32_t>> matrix;
typedef std::function<uint32_t(uint32_t)> fi;

bool rand_bool();

int rand_uniform(uint32_t min, uint32_t max);

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