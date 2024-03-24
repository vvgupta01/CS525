#pragma once

#include <allocator.h>

#include <map>
#include <unordered_map>
#include <vector>

#define PUBLIC_ID 0

class KarmaAllocator : public Allocator {
   public:
    KarmaAllocator(uint32_t num_blocks, float alpha, uint32_t init_credits);

    virtual ~KarmaAllocator();

    void add_user(uint32_t id);

    void remove_user(uint32_t id);

    void allocate();

    void set_demand(uint32_t id, uint32_t demand);

    uint32_t get_num_tenants();

    void print_user(uint32_t id);

   private:
    struct User {
        uint32_t id_, demand_, allocation_, credits_, blocks_;
        int32_t rate_;

        User(uint32_t id, uint32_t init_credits) : id_(id), demand_(0), allocation_(0), rate_(0), blocks_(0), credits_(init_credits) {
        }
    };

    uint32_t public_blocks_, fair_share_, init_credits_;
    std::unordered_map<uint32_t, User*> users_;

    uint32_t get_block_surplus(uint32_t id);

    void borrow_from_poor(uint32_t demand, std::vector<User*>& donors, std::vector<User*>& borrowers);

    void donate_to_rich(uint32_t supply, std::vector<User*>& donors, std::vector<User*>& borrowers);
};