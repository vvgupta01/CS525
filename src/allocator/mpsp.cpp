#include "allocator/mpsp.h"

#include <assert.h>

#include <queue>

#include "utils.h"

struct q_cmp {
    bool operator()(const pi& a, const pi& b) {
        if (a.second == b.second) {
            return rand_bool();
        }
        return a.second < b.second;
    }
};

void MPSPAllocator::Tenant::bid_auction(uint32_t demand, uint32_t fair_share,
                                        uint32_t delta, pi border_bids) {
    if (demand <= fair_share) {
        bid_.qty_ = 0;
    } else {
        uint32_t qty = demand - fair_share;
        uint32_t val = valuation_(qty);

        if (delta > 0) {
            int delta = rand_uniform(-delta, delta);
            delta = std::max(delta, -(int)val);
            val += delta;
        }
        bid_ = Bid(qty, val);
    }
}

MPSPAllocator::MPSPAllocator(uint64_t num_blocks, uint64_t base_blocks, fi valuation)
    : Allocator(num_blocks), base_blocks_(base_blocks), valuation_(valuation) {
    if (base_blocks > num_blocks) {
        throw std::invalid_argument("number of base blocks must be <= total blocks");
    }
    border_bids_.first = valuation(1);
    border_bids_.second = border_bids_.first;
}

void MPSPAllocator::add_tenant(uint32_t id) {
    if (id == PUBLIC_ID || tenants_.find(id) != tenants_.end()) {
        throw std::invalid_argument("add_tenant(): tenant ID already exists");
    }
    tenants_.emplace(id, valuation_);
}

void MPSPAllocator::remove_tenant(uint32_t id) {
    if (id == PUBLIC_ID || tenants_.find(id) == tenants_.end()) {
        throw std::invalid_argument("remove_tenant(): tenant ID does not exist");
    }
    tenants_.erase(id);
}

void MPSPAllocator::allocate() {
    uint32_t fair_share = get_fair_share();
    std::priority_queue<pi, std::vector<pi>, q_cmp> highest_bids;

    for (auto& [id, t] : tenants_) {
        t.allocation_ = fair_share;
        t.payment_ = 0;

        if (t.bid_.qty_ > 0) {
            highest_bids.emplace(id, t.bid_.price_);
        }
    }

    uint64_t welfare = 0;
    uint32_t free_blocks = get_free_blocks();

    while (!highest_bids.empty() && free_blocks > 0) {
        auto [id, price] = highest_bids.top();

        uint32_t blocks = std::min(tenants_[id].bid_.qty_, free_blocks);
        tenants_[id].allocation_ = blocks;
        tenants_[id].payment_ = price;

        free_blocks -= blocks;
        welfare += blocks * price;

        border_bids_.first = price;

        tenants_[id].bid_.qty_ -= blocks;
        if (tenants_[id].bid_.qty_ == 0) {
            highest_bids.pop();
        }
    }

    border_bids_.second = 0;
    if (!highest_bids.empty()) {
        border_bids_.second = highest_bids.top().second;
    }
    assert(border_bids_.first >= border_bids_.second);

    std::vector<pi> remaining_bids;
    while (!highest_bids.empty()) {
        remaining_bids.push_back(highest_bids.top());
        highest_bids.pop();
    }

    for (auto& [id, t] : tenants_) {
        charge_exclusion_payment(t, remaining_bids, free_blocks, welfare);
        t.bid_.qty_ = 0;
    }
}

void MPSPAllocator::set_demand(uint32_t id, uint32_t demand, bool greedy) {
    auto it = tenants_.find(id);
    if (id == PUBLIC_ID || it == tenants_.end()) {
        throw std::invalid_argument("set_bid(): tenant ID does not exist");
    }
    uint32_t delta = greedy ? 10 : 0;
    it->second.bid_auction(demand, get_fair_share(), delta, border_bids_);
}

uint32_t MPSPAllocator::get_num_tenants() {
    return tenants_.size();
}

uint32_t MPSPAllocator::get_allocation(uint32_t id) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        throw std::invalid_argument("get_allocation(): tenant ID does not exist");
    }
    return it->second.allocation_;
}

uint32_t MPSPAllocator::get_payment(uint32_t id) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        throw std::invalid_argument("get_payment(): tenant ID does not exist");
    }
    return it->second.payment_;
}

fi MPSPAllocator::get_valuation() {
    return valuation_;
}

pi MPSPAllocator::get_border_bids() {
    return border_bids_;
}

void MPSPAllocator::charge_exclusion_payment(Tenant& t, std::vector<pi>& remaining_bids, uint32_t free_blocks, uint64_t welfare) {
    if (t.payment_ > 0) {
        free_blocks += t.allocation_;

        uint64_t exclude_welfare = welfare - t.payment_ * t.allocation_;
        welfare = exclude_welfare;

        for (size_t i = 0; i < remaining_bids.size() && free_blocks > 0; ++i) {
            auto [id, price] = remaining_bids[i];

            uint32_t blocks = std::min(tenants_[id].bid_.qty_, free_blocks);
            free_blocks -= blocks;
            welfare += blocks * price;
        }

        uint32_t payment = (welfare - exclude_welfare) / t.allocation_;
        assert(payment <= t.payment_);
        t.payment_ = payment;
    }
}

uint32_t MPSPAllocator::get_fair_share() {
    return base_blocks_ / get_num_tenants();
}

uint64_t MPSPAllocator::get_free_blocks() {
    return total_blocks_ - base_blocks_;
}