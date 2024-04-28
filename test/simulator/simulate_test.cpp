#include <fstream>
#include <vector>

#include "allocator/karma.h"
#include "allocator/maxmin.h"
#include "allocator/mpsp.h"
#include "allocator/sharp.h"
#include "allocator/static.h"
#include "simulation.h"
#include "utils.h"

uint32_t valuation(uint32_t q) {
    return 100;
}

void print_progress(uint progress, uint width = 100) {
    std::cout << "[";
    int pos = width * progress / 100.0;
    for (size_t i = 0; i < width; ++i) {
        if (i < pos) {
            std::cout << "=";
        } else if (i == pos) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << progress << "%\r";
    std::cout.flush();
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: num_blocks num_tenants num_quanta" << std::endl;
        return 0;
    }

    uint32_t B = std::atoi(argv[1]), N = std::atoi(argv[2]), T = std::atoi(argv[3]);
    uint32_t fair_share = B / N;
    matrix demands = generate_uniform_demands(N, T, fair_share * 2);

    std::ofstream sim_out("test/simulator/out/sim.csv");
    std::ofstream auction_out("test/simulator/out/auction.csv");
    std::ofstream barter_out("test/simulator/out/barter.csv");

    for (int sigma = 0; sigma <= 100; sigma += 10) {
        print_progress(sigma);

        StaticAllocator static_alloc(B);
        MaxMinAllocator maxmin_alloc(B);
        KarmaAllocator karma(B, 1, B * T);
        MPSPAllocator mpsp(B, 0, valuation);
        SharpAllocator sharp(B, 2, 2);

        Simulation s(N, T, sigma);
        s.simulate(static_alloc, demands);
        s.output_sim(sim_out, "static");

        s.simulate(maxmin_alloc, demands);
        s.output_sim(sim_out, "maxmin");

        s.simulate(karma, demands);
        s.output_sim(sim_out, "karma");

        s.simulate(mpsp, demands);
        s.output_sim(sim_out, "mpsp");

        s.simulate(sharp, demands);
        s.output_sim(sim_out, "sharp");
        if (sigma == 50) {
            s.output_auction(auction_out);
            s.output_barter(barter_out);
        }
    }
    std::cout << std::endl;
    sim_out.close();
}