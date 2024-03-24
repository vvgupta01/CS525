#include "karma.h"

#include <algorithm>
#include <iostream>

#include "bheap.h"

KarmaAllocator::KarmaAllocator(uint32_t num_blocks, float alpha, uint32_t init_credits) : Allocator(num_blocks) {
    if (alpha < 0 || alpha > 1) {
        throw std::invalid_argument("alpha must be between 0 and 1");
    }

    public_blocks_ = alpha * total_blocks_;
    init_credits_ = init_credits;

    tenants_.emplace(PUBLIC_ID, 0);
}

void KarmaAllocator::add_user(uint32_t id) {
    if (id == DUMMY_ID || tenants_.find(id) != tenants_.end()) {
        return log("add_user(): tenant ID already exists");
    }

    uint32_t credits = get_num_tenants() > 0 ? total_credits_ / get_num_tenants() : init_credits_;
    tenants_.emplace(id, credits);
    total_credits_ += credits;
}

void KarmaAllocator::remove_user(uint32_t id) {
    if (id == PUBLIC_ID || tenants_.find(id) == tenants_.end()) {
        return log("remove_user(): tenant ID does not exist");
    }
    total_credits_ -= tenants_[id].credits_;
    tenants_.erase(id);
}

void KarmaAllocator::allocate() {
    fair_share_ = (total_blocks_ - public_blocks_) / get_num_tenants();

    std::vector<uint32_t> donors, borrowers;
    uint32_t supply = public_blocks_, demand = 0;

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

    total_credits_ = 0;
    for (auto& [id, t] : tenants_) {
        if (id == PUBLIC_ID) {
            t.credits_ = 0;
        } else {
            t.credits_ += t.rate_;
            total_credits_ += t.credits_;
        }
    }
}

void KarmaAllocator::set_demand(uint32_t id, uint32_t demand) {
    auto it = tenants_.find(id);
    if (id == PUBLIC_ID || it == tenants_.end()) {
        return log("set_demand(): tenant ID does not exist");
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

void KarmaAllocator::borrow_from_poor(uint32_t demand, std::vector<uint32_t>& donors, std::vector<uint32_t>& borrowers) {
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
            uint32_t alpha = std::min({poorest_donors.min(), demand / (uint32_t)poorest_donors.size(),
                                       (uint32_t)(next_c - curr_c)});
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

void KarmaAllocator::donate_to_rich(uint32_t supply, std::vector<uint32_t>& donors, std::vector<uint32_t>& borrowers) {
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
            uint32_t alpha = std::min(richest_borrowers.min(), supply / (uint32_t)richest_borrowers.size());
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

void KarmaAllocator::output_tenant(std::ostream& s, uint32_t id) {
    auto t = tenants_[id];
    s << "ID: " << id << " - alloc=" << t.allocation_ << "/" << t.demand_
      << ", credits=" << t.credits_ << std::endl;
}
