#pragma once
#include <cstdint>
#include <memory>
#include <vector>

// Reports snapshot a cumulative queue counter; reporting does not reset it.
// Shared segments keep earlier reports valid after reset or slot recycling.
struct OcclusionSegment {
    uint64_t value{};
    bool ready{};
};
struct OcclusionSnapshot {
    uint64_t base{};
    std::vector<std::shared_ptr<OcclusionSegment>> pending;
    uint64_t value() const {
        uint64_t sum = base;
        for (const auto &s : pending)
            sum += s->value;
        return sum;
    }
};
struct OcclusionCounter {
    bool counting{};
    OcclusionSnapshot total;
    void reset() { counting = true; total = {}; }
    void compact() {
        auto &p = total.pending;
        for (auto it = p.begin(); it != p.end();) {
            if ((*it)->ready) {
                total.base += (*it)->value;
                it = p.erase(it);
            } else ++it;
        }
    }
    std::shared_ptr<OcclusionSegment> segment() {
        compact();
        auto s = std::make_shared<OcclusionSegment>();
        total.pending.push_back(s);
        return s;
    }
    OcclusionSnapshot snapshot() { compact(); return total; }
};
