#pragma once

#include "allocator.h"

typedef std::pair<uint32_t, uint32_t> q_item;

struct Bid {
    uint32_t qty_ = 0, price_ = 0;

    Bid() {}

    Bid(uint32_t qty, uint32_t price) : qty_(qty), price_(price) {
        if (qty < 0 || price < 0) {
            throw std::invalid_argument("bid qty and price must be non-negative");
        }
    }
};

class MPSPAllocator : public Allocator {
   public:
    MPSPAllocator(uint64_t num_blocks, uint64_t base_blocks, uint32_t block_val);

    virtual ~MPSPAllocator() = default;

    void add_tenant(uint32_t id);

    void remove_tenant(uint32_t id);

    void allocate();

    void set_demand(uint32_t id, uint32_t demand, bool greedy);

    uint32_t get_num_tenants();

    uint32_t get_allocation(uint32_t id);

   private:
    struct Tenant {
        Bid bid_;
        uint32_t allocation_ = 0, payment_ = 0;
        static uint32_t BLOCK_VAL;

        Tenant() {}

        static uint32_t valuation(uint32_t blocks) {
            return BLOCK_VAL;
        }

        void bid_auction(uint32_t demand, uint32_t fair_share, bool greedy,
                         uint32_t lowest_accept, uint32_t highest_reject);
    };

    uint64_t base_blocks_;
    uint32_t lowest_accept_ = 0, highest_reject_ = 0;
    std::unordered_map<uint32_t, Tenant> tenants_;

    void charge_exclusion_payment(Tenant& t, std::vector<q_item>& remaining_bids, uint32_t free_blocks, uint64_t welfare);

    uint32_t get_fair_share();

    uint64_t get_free_blocks();
};