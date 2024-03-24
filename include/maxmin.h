#pragma once

#include <unordered_map>

#include "allocator.h"

class MaxMinAllocator : public Allocator {
   public:
    MaxMinAllocator(uint32_t num_blocks);

    virtual ~MaxMinAllocator() = default;

    void add_user(uint32_t id);

    void remove_user(uint32_t id);

    void allocate();

    void set_demand(uint32_t id, uint32_t demand);

    uint32_t get_num_tenants();

    void output_tenant(std::ostream& s, uint32_t id);

   private:
    struct Tenant {
        uint32_t demand_ = 0, allocation_ = 0;
    };

    uint64_t total_demand_ = 0;
    std::unordered_map<uint32_t, Tenant> tenants_;
};