#include <functional>
#include <vector>

#include "allocator/mpsp.h"
#include "allocator/sharp.h"

typedef std::vector<std::vector<uint32_t>> matrix;

struct Simulation {
    uint32_t N_, T_;
    int sigma_;

    std::vector<float> utilizations_, welfares_, instant_fairness_;
    std::vector<uint32_t> lowest_accepted_;
    std::vector<uint64_t> tickets_;

    float avg_utilization_ = 0, avg_fairness_ = 0, fairness_ = 0;
    float avg_welfare_ = 0, incentive_ = 0;

    Simulation(uint32_t N, uint32_t T, int sigma);

    void simulate(Allocator& alloc, matrix& demands);

    void simulate(MPSPAllocator& alloc, matrix& demands);

    void simulate(SharpAllocator& alloc, matrix& demands);

    void output_sim(std::ostream& out, std::string label);

    void output_auction(std::ostream& out);

    void output_barter(std::ostream& out);
};