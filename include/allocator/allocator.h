#pragma once

#include <functional>
#include <iostream>

class Allocator {
   public:
    Allocator(uint32_t num_blocks) : total_blocks_(num_blocks) {
    }

    virtual ~Allocator() = default;

    virtual void add_tenant(uint32_t id) = 0;

    virtual void remove_tenant(uint32_t id) = 0;

    virtual void allocate() = 0;

    virtual void set_demand(uint32_t id, uint32_t demand) = 0;

    virtual uint32_t get_num_tenants() = 0;

    virtual uint32_t get_allocation(uint32_t id) = 0;

    virtual void output_tenant(std::ostream& s, uint32_t id) = 0;

    void log(std::string msg) {
        std::cerr << msg << std::endl;
    }

   protected:
    uint64_t total_blocks_;
};