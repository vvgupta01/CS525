#include "static.h"

StaticAllocator::StaticAllocator(uint32_t num_blocks) : Allocator(num_blocks) {
}

void StaticAllocator::add_user(uint32_t id) {
    allocations_[id] = 0;
}

void StaticAllocator::remove_user(uint32_t id) {
    allocations_.erase(id);
}

void StaticAllocator::allocate() {
    fair_share_ = total_blocks_ / get_num_tenants();
    for (auto& a : allocations_) {
        a.second = fair_share_;
    }
}

void StaticAllocator::set_demand(uint32_t id, uint32_t demand) {
}

uint32_t StaticAllocator::get_num_tenants() {
    return allocations_.size();
}
