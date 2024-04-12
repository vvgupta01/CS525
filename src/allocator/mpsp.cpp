#include "allocator/mpsp.h"

#include <assert.h>

#include <queue>

bool rand_bool() {
    return rand() > RAND_MAX / 2;
}

struct q_cmp {
    bool operator()(const q_item& a, const q_item& b) {
        if (a.second == b.second) {
            return rand_bool();
        }
        return a.second < b.second;
    }
};

void MPSPAllocator::Tenant::bid_auction(uint32_t demand, uint32_t fair_share, bool greedy,
                                        uint32_t lowest_accept, uint32_t highest_reject) {
    if (demand <= fair_share) {
        bid_.qty_ = 0;
    } else {
        uint32_t qty = demand - fair_share;
        uint32_t price = std::min(valuation(qty), lowest_accept);

        if (greedy) {
            price += 1 * (rand_bool() ? 1 : -1);
        }
        bid_ = Bid(qty, price);
    }
}

MPSPAllocator::MPSPAllocator(uint64_t num_blocks, uint64_t base_blocks, uint32_t block_val)
    : Allocator(num_blocks), base_blocks_(base_blocks), lowest_accept_(block_val), highest_reject_(block_val) {
    if (base_blocks > num_blocks) {
        throw std::invalid_argument("number of base blocks must be <= total blocks");
    }
    Tenant::BLOCK_VAL = block_val;
    tenants_[PUBLIC_ID].bid_ = Bid(get_free_blocks(), 0);
}

void MPSPAllocator::add_tenant(uint32_t id) {
    if (tenants_.find(id) != tenants_.end()) {
        throw std::invalid_argument("add_tenant(): tenant ID already exists");
    }
    tenants_[id] = Tenant();
}

void MPSPAllocator::remove_tenant(uint32_t id) {
    if (id == PUBLIC_ID || tenants_.find(id) == tenants_.end()) {
        throw std::invalid_argument("remove_tenant(): tenant ID does not exist");
    }
    tenants_.erase(id);
}

void MPSPAllocator::allocate() {
    uint32_t fair_share = get_fair_share();
    std::priority_queue<q_item, std::vector<q_item>, q_cmp> highest_bids;

    for (auto& [id, t] : tenants_) {
        t.allocation_ = id != PUBLIC_ID ? fair_share : 0;
        t.payment_ = 0;

        if (t.bid_.qty_ > 0) {
            highest_bids.emplace(id, t.bid_.price_);
        }
    }

    uint64_t welfare = 0;
    uint32_t free_blocks = get_free_blocks();

    while (free_blocks > 0) {
        auto [id, price] = highest_bids.top();

        uint32_t blocks = std::min(tenants_[id].bid_.qty_, free_blocks);
        tenants_[id].allocation_ = blocks;
        tenants_[id].payment_ = price;

        free_blocks -= blocks;
        welfare += blocks * price;
        lowest_accept_ = price;

        if (blocks == tenants_[id].bid_.qty_) {
            highest_bids.pop();
        } else {
            tenants_[id].bid_.qty_ -= blocks;
        }
    }

    assert(!highest_bids.empty());
    highest_reject_ = highest_bids.top().second;
    assert(lowest_accept_ >= highest_reject_);

    std::vector<q_item> remaining_bids;
    while (!highest_bids.empty()) {
        remaining_bids.push_back(highest_bids.top());
        highest_bids.pop();
    }

    for (auto& [id, t] : tenants_) {
        if (id != PUBLIC_ID) {
            charge_exclusion_payment(t, remaining_bids, free_blocks, welfare);
            t.bid_.qty_ = 0;
        }
    }
}

void MPSPAllocator::set_demand(uint32_t id, uint32_t demand, bool greedy) {
    auto it = tenants_.find(id);
    if (id == PUBLIC_ID || it == tenants_.end()) {
        throw std::invalid_argument("set_bid(): tenant ID does not exist");
    }
    it->second.bid_auction(demand, get_fair_share(), lowest_accept_, highest_reject_, greedy);
}

uint32_t MPSPAllocator::get_num_tenants() {
    return tenants_.size() - 1;
}

uint32_t MPSPAllocator::get_allocation(uint32_t id) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        throw std::invalid_argument("get_allocation(): tenant ID does not exist");
    }
    return it->second.allocation_;
}

void MPSPAllocator::charge_exclusion_payment(Tenant& t, std::vector<q_item>& remaining_bids, uint32_t free_blocks, uint64_t welfare) {
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