#include "allocator/karma.h"

#include <assert.h>

#include <algorithm>

#include "allocator/bheap.h"

KarmaAllocator::KarmaAllocator(uint64_t num_blocks, float alpha, uint32_t init_credits)
    : Allocator(num_blocks), init_credits_(init_credits) {
    if (alpha < 0 || alpha > 1) {
        throw std::invalid_argument("alpha must be between 0 and 1");
    }

    public_blocks_ = alpha * total_blocks_;
    tenants_.emplace(PUBLIC_ID, 0);
}

void KarmaAllocator::add_tenant(uint32_t id) {
    if (id == DUMMY_ID || tenants_.find(id) != tenants_.end()) {
        throw std::invalid_argument("add_tenant(): tenant ID already exists");
    }

    uint64_t total_credits = 0;
    for (const auto& [id, t] : tenants_) {
        total_credits += t.credits_;
    }

    uint32_t credits = get_num_tenants() > 0 ? total_credits / get_num_tenants() : init_credits_;
    tenants_.emplace(id, credits);

    fair_share_ = get_free_blocks() / get_num_tenants();
}

void KarmaAllocator::remove_tenant(uint32_t id) {
    if (id == PUBLIC_ID || tenants_.find(id) == tenants_.end()) {
        throw std::invalid_argument("remove_tenant(): tenant ID does not exist");
    }
    tenants_.erase(id);
    fair_share_ = get_free_blocks() / get_num_tenants();
}

void KarmaAllocator::allocate() {
    std::vector<uint32_t> donors, borrowers;
    uint64_t supply = public_blocks_, demand = 0;

    for (auto& [id, t] : tenants_) {
        t.rate_ = 0;
        if (id == PUBLIC_ID) {
            t.credits_ = init_credits_ * get_num_tenants();
            continue;
        }
        t.credits_ += public_blocks_ / get_num_tenants();

        if (t.demand_ < fair_share_) {
            donors.push_back(id);
            supply += fair_share_ - t.demand_;
        } else if (t.demand_ > fair_share_) {
            borrowers.push_back(id);
            demand += std::min(t.demand_ - fair_share_, t.credits_);
        }
        t.allocation_ = std::min(t.demand_, fair_share_);
    }

    if (public_blocks_ > 0) {
        donors.push_back(PUBLIC_ID);
    }

    if (supply >= demand) {
        borrow_from_poor(demand, donors, borrowers);
    } else {
        donate_to_rich(supply, donors, borrowers);
    }

    for (auto& [id, t] : tenants_) {
        if (id == PUBLIC_ID) {
            t.credits_ = 0;
        } else {
            t.credits_ += t.rate_;
        }
    }
}

void KarmaAllocator::set_demand(uint32_t id, uint32_t demand, bool greedy) {
    auto it = tenants_.find(id);
    if (id == PUBLIC_ID || it == tenants_.end()) {
        throw std::invalid_argument("set_demand(): tenant ID does not exist");
    }

    if (greedy) {
        demand = std::max(fair_share_, demand);
    }
    it->second.demand_ = demand;
}

uint32_t KarmaAllocator::get_num_tenants() {
    return tenants_.size() - 1;
}

uint32_t KarmaAllocator::get_block_surplus(uint32_t id) {
    if (id == PUBLIC_ID) {
        return public_blocks_;
    }
    return fair_share_ - tenants_[id].demand_;
}

uint64_t KarmaAllocator::get_free_blocks() {
    return total_blocks_ - public_blocks_;
}

void KarmaAllocator::borrow_from_poor(uint64_t demand, std::vector<uint32_t>& donors, std::vector<uint32_t>& borrowers) {
    for (uint32_t id : borrowers) {
        uint32_t to_borrow = std::min(tenants_[id].credits_, tenants_[id].demand_ - fair_share_);
        tenants_[id].allocation_ += to_borrow;
        tenants_[id].rate_ -= to_borrow;
    }

    std::vector<Candidate> donor_c;
    for (uint32_t id : donors) {
        donor_c.emplace_back(id, tenants_[id].credits_, get_block_surplus(id));
    }
    std::sort(donor_c.begin(), donor_c.end(), [](const Candidate& a, const Candidate& b) {
        return a.credits_ < b.credits_;
    });
    donor_c.emplace_back(DUMMY_ID, std::numeric_limits<uint32_t>::max(), 0);

    int64_t curr_c = -1, next_c = donor_c[0].credits_;

    size_t idx = 0;
    auto poorest_donors = BroadcastHeap();

    while (demand > 0) {
        if (poorest_donors.empty()) {
            curr_c = next_c;
            assert(curr_c < std::numeric_limits<uint32_t>::max());
        }

        while (donor_c[idx].credits_ == curr_c) {
            poorest_donors.push(donor_c[idx].id_, donor_c[idx].blocks_);
            idx++;
        }
        next_c = donor_c[idx].credits_;

        if (demand < poorest_donors.size()) {
            for (uint32_t i = 0; i < demand; ++i) {
                auto [id, v] = poorest_donors.pop();
                tenants_[id].rate_ += get_block_surplus(id) - v + 1;
            }
            demand = 0;
        } else {
            int32_t alpha = std::min({(int64_t)poorest_donors.min(), (int64_t)(demand / poorest_donors.size()),
                                      next_c - curr_c});
            poorest_donors.add_all(-alpha);
            curr_c += alpha;
            demand -= poorest_donors.size() * alpha;
        }

        while (!poorest_donors.empty() && poorest_donors.min() == 0) {
            auto [id, _] = poorest_donors.pop();
            tenants_[id].rate_ += get_block_surplus(id);
        }
    }

    while (!poorest_donors.empty()) {
        auto [id, v] = poorest_donors.pop();
        tenants_[id].rate_ += get_block_surplus(id) - v;
    }
}

void KarmaAllocator::donate_to_rich(uint64_t supply, std::vector<uint32_t>& donors, std::vector<uint32_t>& borrowers) {
    for (uint32_t id : donors) {
        uint32_t to_donate = get_block_surplus(id);
        tenants_[id].rate_ += to_donate;
    }

    std::vector<Candidate> borrower_c;
    for (uint32_t id : borrowers) {
        uint32_t blocks = std::min(tenants_[id].credits_, tenants_[id].demand_ - fair_share_);
        borrower_c.emplace_back(id, tenants_[id].credits_, blocks);
    }
    std::sort(borrower_c.begin(), borrower_c.end(), [](const Candidate& a, const Candidate& b) {
        return a.credits_ > b.credits_;
    });
    borrower_c.emplace_back(DUMMY_ID, -1, 0);

    int64_t curr_c = std::numeric_limits<int32_t>::max(), next_c = borrower_c[0].credits_;

    size_t idx = 0;
    auto richest_borrowers = BroadcastHeap();

    while (supply > 0) {
        if (richest_borrowers.empty()) {
            curr_c = next_c;
            assert(curr_c > -1);
        }

        while (borrower_c[idx].credits_ == curr_c) {
            richest_borrowers.push(borrower_c[idx].id_, borrower_c[idx].blocks_);
            idx++;
        }
        next_c = borrower_c[idx].credits_;

        if (supply < richest_borrowers.size()) {
            for (uint32_t i = 0; i < supply; ++i) {
                auto [id, v] = richest_borrowers.pop();
                supply--;

                int32_t delta = std::min(tenants_[id].credits_, tenants_[id].demand_ - fair_share_) - v + 1;
                tenants_[id].allocation_ += delta;
                tenants_[id].rate_ -= delta;
            }
            supply = 0;
        } else {
            int32_t alpha = std::min((uint64_t)richest_borrowers.min(), supply / richest_borrowers.size());
            richest_borrowers.add_all(-alpha);
            curr_c -= alpha;
            supply -= richest_borrowers.size() * alpha;
        }

        while (!richest_borrowers.empty() && richest_borrowers.min() == 0) {
            auto [id, _] = richest_borrowers.pop();
            int64_t delta = std::min(tenants_[id].credits_, tenants_[id].demand_ - fair_share_);
            tenants_[id].allocation_ += delta;
            tenants_[id].rate_ -= delta;
        }
    }

    while (!richest_borrowers.empty()) {
        auto [id, v] = richest_borrowers.pop();
        int32_t delta = std::min(tenants_[id].credits_, tenants_[id].demand_ - fair_share_) - v;
        tenants_[id].allocation_ += delta;
        tenants_[id].rate_ -= delta;
    }
}

uint32_t KarmaAllocator::get_allocation(uint32_t id) {
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        throw std::invalid_argument("get_allocation(): tenant ID does not exist");
    }
    return it->second.allocation_;
}

void KarmaAllocator::output_tenant(std::ostream& s, uint32_t id) {
    auto t = tenants_[id];
    s << "ID: " << id << " - alloc=" << t.allocation_ << "/" << t.demand_
      << ", credits=" << t.credits_ << std::endl;
}
