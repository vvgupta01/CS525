#include <functional>
#include <vector>

// demands and allocations of a tenant at each quanta
std::vector<float> welfares(std::vector<std::vector<uint32_t>>& demands,
                            std::vector<std::vector<uint32_t>>& allocations);

float fairness(std::vector<float>& welfares);

// demands and allocations of all tenants at each quanta
float fairness(std::vector<std::vector<uint32_t>>& demands, std::vector<std::vector<uint32_t>>& allocations);

// demands and allocations of all tenants at a given quanta
float instant_fairness(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations);

// demands and allocations of all tenants at a given quanta
float utilization(std::vector<uint32_t>& demands, std::vector<uint32_t>& allocations, uint64_t blocks);