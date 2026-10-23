#pragma once
#include <algorithm>
#include <cstdint>

namespace pib::core {
    enum class Kind { Clear, Landed, Collision, Unknown, Timeout };
    struct Outcome {
        Kind kind = Kind::Unknown;
        int ticks = 0;
        uintptr_t obstacle = 0;
        bool clearedOriginal = false;
    };
    enum class Reason { Disabled, SafeNow, Uncertain, NoSave, Corrected, Budget };
    struct Decision {
        int delay = 0;
        Outcome original;
        Outcome selected;
        Reason reason = Reason::NoSave;
    };

    inline int ticks(int value) { return std::clamp(value, 0, 32); }

    // The probe owns simulation; this policy is deliberately independent of GD.
    template <class Probe>
    Decision choose(int maximum, Probe&& probe) {
        Decision decision;
        maximum = ticks(maximum);
        if (!maximum) { decision.reason = Reason::Disabled; return decision; }
        decision.original = probe(0, uintptr_t{0});
        decision.selected = decision.original;
        if (decision.original.kind == Kind::Clear || decision.original.kind == Kind::Landed) {
            decision.reason = Reason::SafeNow;
            return decision;
        }
        if (decision.original.kind != Kind::Collision) {
            decision.reason = decision.original.kind == Kind::Timeout ? Reason::Budget : Reason::Uncertain;
            return decision;
        }
        for (int delay = 1; delay <= maximum; ++delay) {
            const auto candidate = probe(delay, decision.original.obstacle);
            if (candidate.kind == Kind::Timeout) {
                if (!decision.delay) decision.reason = Reason::Budget;
                break;
            }
            // An uncertain candidate does not invalidate later candidates.
            if (candidate.kind == Kind::Unknown) continue;
            if (candidate.kind == Kind::Clear || candidate.kind == Kind::Landed) {
                decision.delay = delay;
                decision.selected = candidate;
                decision.reason = Reason::Corrected;
                return decision;
            }
            // Do not "save" by crashing into the same wall slightly later.
            // A later obstacle may need the user's NEXT input, so crossing the
            // original obstacle with substantial extra time is useful too.
            const int margin = std::max(8, 2 * delay);
            if (decision.original.obstacle && candidate.obstacle &&
                candidate.obstacle != decision.original.obstacle && candidate.clearedOriginal &&
                candidate.ticks >= decision.original.ticks + margin &&
                candidate.ticks > decision.selected.ticks) {
                decision.delay = delay;
                decision.selected = candidate;
                decision.reason = Reason::Corrected;
            }
        }
        return decision;
    }

    // Prefer one tick AFTER the first validated correction. Probe that exact
    // execution time as well; never add an untested delay to the real queue.
    template <class Probe>
    Decision chooseWithExtraTick(int maximum, Probe&& probe) {
        auto decision = choose(maximum, probe);
        if (!decision.delay) return decision;
        const int later = decision.delay + 1;
        const auto candidate = probe(later, decision.original.obstacle);
        const bool survives = candidate.kind == Kind::Clear || candidate.kind == Kind::Landed;
        const bool improves = candidate.kind == Kind::Collision &&
            decision.selected.kind == Kind::Collision &&
            decision.original.obstacle && candidate.obstacle && candidate.clearedOriginal &&
            candidate.obstacle != decision.original.obstacle &&
            candidate.ticks >= decision.original.ticks + std::max(8, 2 * later) &&
            candidate.ticks >= decision.selected.ticks;
        if (survives || improves) {
            decision.delay = later;
            decision.selected = candidate;
        }
        return decision;
    }
}
