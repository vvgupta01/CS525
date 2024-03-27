#include "allocator/maxmin.h"

#include "allocator/bheap.h"

MaxMinAllocator::MaxMinAllocator(uint64_t num_blocks) : Allocator(num_blocks) {
}

void MaxMinAllocator::add_tenant(uint32_t id) {
    if (tenants_.find(id) != tenants_.end()) {
        return log("add_tenant(): tenant ID already exists");
    }
    tenants_[id] = Tenant();
}

void MaxMinAllocator::remove_tenant(uint32_t id) {
    if (tenants_.find(id) != tenants_.end()) {
        return log("remove_tenant(): tenant ID does not exist");
    }
    tenants_.erase(id);
}

void MaxMinAllocator::allocate() {
    if (total_blocks_ >= total_demand_) {
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

void MaxMinAllocator::set_demand(uint32_t id, uint32_t demand) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        return log("set_demand(): tenant ID does not exist");
    }
    total_demand_ += demand - it->second.demand_;
    it->second.demand_ = demand;
}

uint32_t MaxMinAllocator::get_num_tenants() {
    return tenants_.size();
}

uint32_t MaxMinAllocator::get_allocation(uint32_t id) {
    return tenants_[id].allocation_;
}

void MaxMinAllocator::output_tenant(std::ostream& s, uint32_t id) {
    auto t = tenants_[id];
    s << "ID: " << id << " - alloc=" << t.allocation_ << "/" << t.demand_ << std::endl;
}
