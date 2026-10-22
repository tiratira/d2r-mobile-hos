#include "gfx/nvn/occlusion_counter.h"
#include <cassert>
int main() {
    OcclusionCounter c;
    c.reset();
    auto a = c.segment();
    auto first = c.snapshot();
    auto b = c.segment(); // report continues counting; may be another submission
    auto second = c.snapshot();
    c.reset(); // old in-flight reports must survive reset
    auto d = c.segment();
    a->value = 7; a->ready = true;
    b->value = 11; b->ready = true;
    assert(first.value() == 7 && second.value() == 18);
    assert(c.snapshot().value() == 0);
    d->value = 3; d->ready = true;
    c.compact();
    assert(c.total.pending.empty() && c.snapshot().value() == 3);
    for (int i = 0; i < 10000; ++i) {
        auto s = c.segment(); s->value = 1; s->ready = true;
    }
    assert(c.snapshot().value() == 10003 && c.total.pending.empty());
    c.reset();
    assert(c.snapshot().value() == 0); // reset followed by report without draws
    auto hidden = c.segment(); hidden->ready = true; // real zero is preserved
    assert(c.snapshot().value() == 0);
    auto fallback = c.segment(); fallback->value = 1; fallback->ready = true;
    assert(c.snapshot().value() == 1); // exhausted measurement is conservative
}
