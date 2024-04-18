#pragma once

#include <functional>
#include <iostream>

#define PUBLIC_ID 0

class Allocator {
   public:
    Allocator(uint32_t num_blocks) : total_blocks_(num_blocks) {
    }

    virtual ~Allocator() = default;

    virtual void add_tenant(uint32_t id) = 0;

    virtual void remove_tenant(uint32_t id) = 0;

    virtual void allocate() = 0;

    virtual void set_demand(uint32_t id, uint32_t demand, bool greedy) = 0;

    virtual uint32_t get_num_tenants() = 0;

    virtual uint32_t get_allocation(uint32_t id) = 0;

    uint64_t get_total_blocks() {
        return total_blocks_;
    }

   protected:
    uint64_t total_blocks_;
    uint32_t fair_share_;
};