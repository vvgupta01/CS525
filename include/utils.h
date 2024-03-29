#include <functional>
#include <vector>

typedef std::vector<std::vector<uint32_t>> matrix;

matrix generate_uniform_demands(uint32_t N, uint32_t T, uint32_t max_demand);

std::vector<float> welfares(matrix& demands, matrix& allocations);

float fairness(std::vector<float>& welfares);

float fairness(matrix& demands, matrix& allocations);

float instant_fairness(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations);

float utilization(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations, uint64_t blocks);

float range_average(std::vector<float>& arr, size_t a, size_t b);