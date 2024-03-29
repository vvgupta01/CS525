#include <fstream>
#include <vector>

#include "allocator/karma.h"
#include "allocator/maxmin.h"
#include "allocator/static.h"
#include "simulation.h"
#include "utils.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: fair_share fraction_selfish num_quanta" << std::endl;
        return 0;
    }

    uint32_t fair_share = std::atoi(argv[1]), T = std::atoi(argv[3]);
    float sigma = std::atof(argv[2]);

    std::ofstream out("test/simulator/out/bench.csv");
    for (float N = 10; N <= 10000; N *= 10) {
        uint32_t B = fair_share * N;
        auto demands = generate_uniform_demands(N, T, fair_share * 2);

        uint32_t si = sigma * N;
        for (uint32_t t = 0; t < T; ++t) {
            for (uint32_t i = 0; i < N; ++i) {
                if (i < si) {
                    demands[t][i] = std::max(demands[t][i], fair_share);
                }
            }
        }

        StaticAllocator static_alloc(B);
        MaxMinAllocator maxmin_alloc(B);
        KarmaAllocator karma(B, 0.5, B * T);

        Simulation s(N, T, sigma);
        s.benchmark(static_alloc, demands);
        s.output_bench(out, "static");

        s.benchmark(maxmin_alloc, demands);
        s.output_bench(out, "maxmin");

        s.benchmark(karma, demands);
        s.output_bench(out, "karma");
    }
    out.close();
}