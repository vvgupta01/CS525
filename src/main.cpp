#include <iostream>

#include "karma.h"

int main() {
    KarmaAllocator karma(4, 0.5, 100);

    karma.add_user(1);
    karma.add_user(2);

    karma.set_demand(1, 3);
    karma.set_demand(2, 1);
    karma.allocate();

    karma.print_user(1);
    karma.print_user(2);

    karma.set_demand(1, 2);
    karma.set_demand(2, 4);
    karma.allocate();

    karma.print_user(1);
    karma.print_user(2);

    return 0;
}