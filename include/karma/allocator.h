#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#define PUBLIC_ID 0

namespace karma {
class Allocator {
   public:
    Allocator(uint32_t num_blocks, float alpha, uint32_t init_credits);

    virtual ~Allocator();

    void add_user(uint32_t id);

    void remove_user(uint32_t id);

    void allocate();

    void set_demand(uint32_t id, uint32_t demand);

    uint32_t get_allocation(uint32_t id);

    uint32_t get_credits(uint32_t id);

    void print_user(uint32_t id);

    struct User {
        uint32_t id_, demand_, allocation_, credits_, blocks_;
        int32_t rate_;

        User(uint32_t id, uint32_t init_credits);
    };

   private:
    uint32_t total_blocks_, public_blocks_, fair_share_;
    uint32_t init_credits_;

    std::unordered_map<uint32_t, User*> users_;

    uint32_t get_block_surplus(uint32_t id);

    uint32_t get_num_tenants();

    void borrow_from_poor(uint32_t demand, std::vector<User*>& donors, std::vector<User*>& borrowers);

    void donate_to_rich(uint32_t supply, std::vector<User*>& donors, std::vector<User*>& borrowers);
};
}  // namespace karma