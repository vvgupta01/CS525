#include <iostream>

#include "allocator/karma.h"
#include "allocator/maxmin.h"
#include "allocator/static.h"

void test_simple(Allocator& a) {
    a.add_user(1);
    a.add_user(2);

    a.set_demand(1, 3);
    a.set_demand(2, 1);
    a.allocate();

    a.output_tenant(std::cout, 1);
    a.output_tenant(std::cout, 2);

    a.set_demand(1, 2);
    a.set_demand(2, 4);
    a.allocate();

    a.output_tenant(std::cout, 1);
    a.output_tenant(std::cout, 2);
}

int main() {
    StaticAllocator sa(4);
    MaxMinAllocator mma(4);
    KarmaAllocator karma(4, 0.5, 100);

    test_simple(sa);
    std::cout << std::endl;
    test_simple(mma);
    std::cout << std::endl;
    test_simple(karma);
    return 0;
}