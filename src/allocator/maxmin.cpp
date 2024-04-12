#include "allocator/maxmin.h"

#include <assert.h>

#include "allocator/bheap.h"

MaxMinAllocator::MaxMinAllocator(uint64_t num_blocks) : Allocator(num_blocks) {
}

void MaxMinAllocator::add_tenant(uint32_t id) {
    if (tenants_.find(id) != tenants_.end()) {
        throw std::invalid_argument("add_tenant(): tenant ID already exists");
    }
    tenants_[id] = Tenant();
    fair_share_ = total_blocks_ / get_num_tenants();
}

void MaxMinAllocator::remove_tenant(uint32_t id) {
    if (tenants_.find(id) != tenants_.end()) {
        throw std::invalid_argument("remove_tenant(): tenant ID does not exist");
    }
    tenants_.erase(id);
    fair_share_ = total_blocks_ / get_num_tenants();
}

void MaxMinAllocator::allocate() {
    uint64_t total_demand = 0;
    for (auto& [_, t] : tenants_) {
        total_demand += t.demand_;
    }

    if (total_blocks_ >= total_demand) {
        for (auto& [_, t] : tenants_) {
            t.allocation_ = t.demand_;
        }
    } else {
        auto h = BroadcastHeap();
        for (const auto& [id, t] : tenants_) {
            h.push(id, t.demand_);
        }

        uint64_t supply = total_blocks_;
        while (supply > 0) {
            if (supply < h.size()) {
                for (uint32_t i = 0; i < supply; ++i) {
                    auto [id, v] = h.pop();
                    tenants_[id].allocation_ = tenants_[id].demand_ - v + 1;
                }
                supply = 0;
            } else {
                int32_t alpha = std::min((int64_t)h.min(), (int64_t)(supply / h.size()));
                h.add_all(-alpha);
                supply -= h.size() * alpha;
            }

            while (!h.empty() && h.min() == 0) {
                auto [id, _] = h.pop();
                tenants_[id].allocation_ = tenants_[id].demand_;
            }
        }

        while (!h.empty()) {
            auto [id, v] = h.pop();
            tenants_[id].allocation_ = tenants_[id].demand_ - v;
        }
    }
}

void MaxMinAllocator::set_demand(uint32_t id, uint32_t demand, bool greedy) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        throw std::invalid_argument("set_demand(): tenant ID does not exist");
    }

    if (greedy) {
        demand = std::max(fair_share_, demand);
    }
    it->second.demand_ = demand;
}

uint32_t MaxMinAllocator::get_num_tenants() {
    return tenants_.size();
}

uint32_t MaxMinAllocator::get_allocation(uint32_t id) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        throw std::invalid_argument("get_allocation(): tenant ID does not exist");
    }
    return it->second.allocation_;
}

void MaxMinAllocator::output_tenant(std::ostream& s, uint32_t id) {
    auto t = tenants_[id];
    s << "ID: " << id << " - alloc=" << t.allocation_ << "/" << t.demand_ << std::endl;
}
