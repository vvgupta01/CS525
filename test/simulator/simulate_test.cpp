#include <fstream>
#include <vector>

#include "allocator/karma.h"
#include "allocator/maxmin.h"
#include "allocator/static.h"
#include "simulation.h"
#include "utils.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: num_blocks num_tenants num_quanta" << std::endl;
        return 0;
    }

    uint32_t B = std::atoi(argv[1]), N = std::atoi(argv[2]), T = std::atoi(argv[3]);
    uint32_t fair_share = B / N;
    matrix demands = generate_uniform_demands(N, T, fair_share * 2);

    std::ofstream out("test/simulator/out/sim.csv");
    for (int sigma = 0; sigma <= 100; sigma += 10) {
        StaticAllocator static_alloc(B);
        MaxMinAllocator maxmin_alloc(B);
        KarmaAllocator karma(B, 0.5, B * T);

        Simulation s(N, T, sigma);
        s.simulate(static_alloc, demands);
        s.output_sim(out, "static");

        s.simulate(maxmin_alloc, demands);
        s.output_sim(out, "maxmin");

        s.simulate(karma, demands);
        s.output_sim(out, "karma");
    }
    out.close();
}