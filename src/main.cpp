#include <iostream>

#include "allocator/karma.h"

int main() {
    KarmaAllocator karma(4, 0.5, 100);

    karma.add_tenant(1);
    karma.add_tenant(2);

    karma.set_demand(1, 0);
    karma.set_demand(2, 4);
    karma.allocate();

    karma.output_tenant(std::cout, 1);
    karma.output_tenant(std::cout, 2);
}