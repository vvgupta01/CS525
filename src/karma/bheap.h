#pragma once

#include <functional>
#include <queue>
#include <utility>
#include <vector>

typedef std::pair<uint32_t, uint32_t> bheap_item;

struct bheap_cmp {
    bool operator()(const bheap_item& a, const bheap_item& b) {
        return a.second > b.second;
    }
};

namespace karma {
class BroadcastHeap {
   public:
    BroadcastHeap();

    void push(uint32_t key, uint32_t val);

    bheap_item pop();

    size_t size();

    bool empty();

    uint32_t min();

    void add_all(int32_t val);

   private:
    std::priority_queue<bheap_item, std::vector<bheap_item>, bheap_cmp> h_;
    int32_t base_val_;
};
}  // namespace karma