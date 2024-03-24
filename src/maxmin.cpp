#include "maxmin.h"

#include "bheap.h"

MaxMinAllocator::MaxMinAllocator(uint32_t num_blocks) : Allocator(num_blocks) {
}

void MaxMinAllocator::add_user(uint32_t id) {
    tenants_[id] = Tenant();
}

void MaxMinAllocator::remove_user(uint32_t id) {
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

        uint32_t supply = total_blocks_;
        while (supply > 0) {
            if (supply < h.size()) {
                for (uint32_t i = 0; i < supply; ++i) {
                    auto [id, v] = h.pop();
                    tenants_[id].allocation_ = tenants_[id].demand_ - v + 1;
                }
                supply = 0;
            } else {
                uint32_t alpha = std::min(h.min(), supply / (uint32_t)h.size());
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
    total_demand_ += demand - tenants_[id].demand_;
    tenants_[id].demand_ = demand;
}

uint32_t MaxMinAllocator::get_num_tenants() {
    return tenants_.size();
}

void MaxMinAllocator::output_tenant(std::ostream& s, uint32_t id) {
    auto t = tenants_[id];
    s << "ID: " << id << " - alloc=" << t.allocation_ << "/" << t.demand_ << std::endl;
}
