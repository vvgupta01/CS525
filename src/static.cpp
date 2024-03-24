#include "static.h"

StaticAllocator::StaticAllocator(uint32_t num_blocks) : Allocator(num_blocks) {
}

void StaticAllocator::add_user(uint32_t id) {
    if (allocations_.find(id) != allocations_.end()) {
        return log("add_user(): tenant ID already exists");
    }
    allocations_[id] = 0;
}

void StaticAllocator::remove_user(uint32_t id) {
    if (allocations_.find(id) == allocations_.end()) {
        return log("remove_user(): tenant ID does not exist");
    }
    allocations_.erase(id);
}

void StaticAllocator::allocate() {
    fair_share_ = total_blocks_ / get_num_tenants();
    for (auto& a : allocations_) {
        a.second = fair_share_;
    }
}

void StaticAllocator::set_demand(uint32_t id, uint32_t demand) {
    if (allocations_.find(id) == allocations_.end()) {
        return log("set_demand(): tenant ID does not exist");
    }
}

uint32_t StaticAllocator::get_num_tenants() {
    return allocations_.size();
}

void StaticAllocator::output_tenant(std::ostream& s, uint32_t id) {
    s << "ID: " << id << " - alloc=" << allocations_[id] << std::endl;
}
