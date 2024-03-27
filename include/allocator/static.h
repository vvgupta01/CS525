#pragma once

#include <unordered_map>

#include "allocator.h"

class StaticAllocator : public Allocator {
   public:
    StaticAllocator(uint64_t num_blocks);

    virtual ~StaticAllocator() = default;

    void add_tenant(uint32_t id);

    void remove_tenant(uint32_t id);

    void allocate();

    void set_demand(uint32_t id, uint32_t demand);

    uint32_t get_num_tenants();

    uint32_t get_allocation(uint32_t id);

    void output_tenant(std::ostream& s, uint32_t id);

   private:
    uint32_t fair_share_;
    std::unordered_map<uint32_t, uint32_t> allocations_;
};