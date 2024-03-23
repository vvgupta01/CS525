#include "karma/allocator.h"

#include <algorithm>
#include <iostream>

#include "bheap.h"

namespace karma {
Allocator::User::User(uint32_t id, uint32_t init_credits) {
    id_ = id;
    demand_ = 0;
    allocation_ = 0;
    rate_ = 0;
    blocks_ = 0;
    credits_ = init_credits;
}

void Allocator::print_user(uint32_t id) {
    auto u = users_[id];
    std::cout << "ID: " << u->id_ << " - alloc=" << u->allocation_ << "/" << u->demand_
              << ", credits=" << u->credits_ << " (delta=" << u->rate_ << ")" << std::endl;
}

Allocator::Allocator(uint32_t num_blocks, float alpha, uint32_t init_credits) {
    total_blocks_ = num_blocks;
    if (alpha < 0 || alpha > 1) {
        throw std::invalid_argument("alpha must be between 0 and 1");
    }

    public_blocks_ = (uint32_t)(alpha * total_blocks_);
    init_credits_ = init_credits;

    users_[PUBLIC_ID] = new User(PUBLIC_ID, 0);
}

Allocator::~Allocator() {
    for (auto [_, u] : users_) {
        delete u;
    }
}

void Allocator::add_user(uint32_t id) {
    if (users_.count(id)) {
        throw std::runtime_error("user already exists");
    }

    uint32_t credits = init_credits_;

    if (get_num_tenants() > 0) {
        uint64_t sum_credits = 0;
        for (const auto& [_, u] : users_) {
            sum_credits += u->credits_;
        }
        credits = sum_credits / get_num_tenants();
    }
    users_[id] = new User(id, credits);
}

void Allocator::remove_user(uint32_t id) {
    if (id != PUBLIC_ID) {
        delete users_[id];
        users_.erase(id);
    }
}

void Allocator::allocate() {
    fair_share_ = (total_blocks_ - public_blocks_) / get_num_tenants();

    std::vector<User*> donors, borrowers;
    uint32_t supply = public_blocks_, demand = 0;

    for (auto& [id, u] : users_) {
        u->rate_ = 0;
        u->credits_ += public_blocks_ / get_num_tenants();
        if (id == PUBLIC_ID) {
            continue;
        }

        if (u->demand_ < fair_share_) {
            donors.push_back(u);
            supply += fair_share_ - u->demand_;
        } else if (u->demand_ > fair_share_) {
            borrowers.push_back(u);
            demand += std::min(u->demand_ - fair_share_, u->credits_);
        }
        u->allocation_ = std::min(u->demand_, fair_share_);
    }

    if (public_blocks_ > 0) {
        users_[PUBLIC_ID]->credits_ = users_.size() * init_credits_ + 1;
        donors.push_back(users_[PUBLIC_ID]);
    }

    std::cout << "fair share: " << fair_share_ << ", public blocks: " << public_blocks_ << std::endl;
    std::cout << "supply: " << supply << ", demand: " << demand << std::endl;

    if (supply >= demand) {
        borrow_from_poor(demand, donors, borrowers);
    } else {
        donate_to_rich(supply, donors, borrowers);
    }

    for (auto& [_, u] : users_) {
        u->credits_ += u->rate_;
    }
}

void Allocator::set_demand(uint32_t id, uint32_t demand) {
    users_[id]->demand_ = demand;
}

uint32_t Allocator::get_num_tenants() {
    return users_.size() - 1;
}

uint32_t Allocator::get_block_surplus(uint32_t id) {
    if (id == PUBLIC_ID) {
        return public_blocks_;
    }
    return fair_share_ - users_[id]->demand_;
}

uint32_t Allocator::get_allocation(uint32_t id) {
    return users_[id]->allocation_;
}

uint32_t Allocator::get_credits(uint32_t id) {
    return users_[id]->credits_;
}

void Allocator::borrow_from_poor(uint32_t demand, std::vector<User*>& donors, std::vector<User*>& borrowers) {
    for (const auto& b : borrowers) {
        uint32_t to_borrow = std::min(b->credits_, b->demand_ - fair_share_);
        b->allocation_ += to_borrow;
        b->rate_ -= to_borrow;
    }

    for (const auto& d : donors) {
        d->blocks_ = get_block_surplus(d->id_);
    }
    std::sort(donors.begin(), donors.end(), [](const Allocator::User* a, const Allocator::User* b) {
        return a->credits_ < b->credits_;
    });
    auto temp_u = new User(-1, -1);
    donors.push_back(temp_u);

    int32_t curr_c = -1, next_c = donors[0]->credits_;

    size_t idx = 0;
    auto poorest_donors = BroadcastHeap();

    while (demand > 0) {
        if (poorest_donors.empty()) {
            curr_c = next_c;
        }

        while (donors[idx]->credits_ == curr_c) {
            poorest_donors.push(donors[idx]->id_, donors[idx]->blocks_);
            idx++;
        }
        next_c = donors[idx]->credits_;

        if (demand < poorest_donors.size()) {
            for (uint32_t i = 0; i < demand; ++i) {
                bheap_item item = poorest_donors.pop();
                users_[item.first]->rate_ += get_block_surplus(item.first) - item.second + 1;
            }
            demand = 0;
        } else {
            uint32_t alpha = std::min({poorest_donors.min(), (uint32_t)(demand / poorest_donors.size()), (uint32_t)next_c - curr_c});
            poorest_donors.add_all(-alpha);
            curr_c += alpha;
            demand -= poorest_donors.size() * alpha;
        }

        while (!poorest_donors.empty() && poorest_donors.min() == 0) {
            bheap_item item = poorest_donors.pop();
            users_[item.first]->rate_ += get_block_surplus(item.first);
        }
    }

    while (poorest_donors.size() > 0) {
        bheap_item item = poorest_donors.pop();
        users_[item.first]->rate_ += get_block_surplus(item.first) - item.second;
    }
    delete temp_u;
}

void Allocator::donate_to_rich(uint32_t supply, std::vector<User*>& donors, std::vector<User*>& borrowers) {
    for (const auto& d : donors) {
        uint32_t to_donate = get_block_surplus(d->id_);
        d->rate_ += to_donate;
    }

    for (auto& b : borrowers) {
        b->blocks_ = std::min(b->credits_, b->demand_ - fair_share_);
    }
    std::sort(borrowers.begin(), borrowers.end(), [](const Allocator::User* a, const Allocator::User* b) {
        return a->credits_ > b->credits_;
    });
    auto temp_u = new User(-1, 0);
    borrowers.push_back(temp_u);

    int32_t curr_c = std::numeric_limits<int32_t>::max();
    int32_t next_c = borrowers[0]->credits_;

    size_t idx = 0;
    auto richest_borrowers = BroadcastHeap();

    while (supply > 0) {
        if (richest_borrowers.empty()) {
            curr_c = next_c;
        }

        while (borrowers[idx]->credits_ == curr_c) {
            richest_borrowers.push(borrowers[idx]->id_, borrowers[idx]->blocks_);
            idx++;
        }
        next_c = borrowers[idx]->credits_;

        if (supply < richest_borrowers.size()) {
            for (uint32_t i = 0; i < supply; ++i) {
                bheap_item it = richest_borrowers.pop();
                int32_t blocks = it.second - 1;
                supply--;

                int32_t delta = std::min(users_[it.first]->credits_, users_[it.first]->demand_ - fair_share_) - blocks;
                users_[it.first]->allocation_ += delta;
                users_[it.first]->rate_ -= delta;
            }
        } else {
            uint32_t alpha = std::min(richest_borrowers.min(), (uint32_t)(supply / richest_borrowers.size()));
            richest_borrowers.add_all(-alpha);
            curr_c -= alpha;
            supply -= richest_borrowers.size() * alpha;
        }

        while (!richest_borrowers.empty() && richest_borrowers.min() == 0) {
            bheap_item it = richest_borrowers.pop();
            int64_t delta = std::min(users_[it.first]->credits_, users_[it.first]->demand_ - fair_share_);
            users_[it.first]->allocation_ += delta;
            users_[it.first]->rate_ -= delta;
        }
    }

    while (richest_borrowers.size() > 0) {
        bheap_item it = richest_borrowers.pop();
        int32_t delta = std::min(users_[it.first]->credits_, users_[it.first]->demand_ - fair_share_) - it.second;
        users_[it.first]->allocation_ += delta;
        users_[it.first]->rate_ -= delta;
    }
    delete temp_u;
}
}  // namespace karma