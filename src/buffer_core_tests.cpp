#include "buffer_core.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace pib::core;

void require(bool value, char const* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

Decision scenario(int limit, std::vector<Outcome> const& outcomes) {
    return choose(limit, [&](int delay, uintptr_t original) {
        if (delay) require(original == outcomes[0].obstacle, "baseline obstacle supplied to probe");
        require(delay < static_cast<int>(outcomes.size()), "probe stays within configured window");
        return outcomes[delay];
    });
}

int main() {
    using K = Kind;
    int calls = 0;
    auto disabled = choose(0, [&](int, uintptr_t) { ++calls; return Outcome{}; });
    require(disabled.delay == 0 && calls == 0, "disabled buffer never simulates");
    auto safe = scenario(4, {{K::Clear, 120}});
    require(safe.delay == 0 && safe.reason == Reason::SafeNow, "safe click/release passes immediately");
    auto cube = scenario(4, {{K::Collision, 75, 101}, {K::Collision, 78, 101}, {K::Landed, 102}});
    require(cube.delay == 2, "cube landing can complete a correction before full horizon");
    auto release = scenario(4, {{K::Collision, 3, 101}, {K::Collision, 4, 101}, {K::Clear, 48}});
    require(release.delay == 2, "early release can extend hold by two ticks");
    auto sameWall = scenario(2, {{K::Collision, 8, 101}, {K::Collision, 22, 101, true}, {K::Collision, 30, 101, true}});
    require(sameWall.delay == 0, "later death at same obstacle is not a correction");
    auto nextObstacle = scenario(2, {{K::Collision, 8, 101}, {K::Collision, 24, 202, true}, {K::Collision, 23, 202, true}});
    require(nextObstacle.delay == 1, "clearing first hazard allows next-input obstacle later");
    auto notPast = scenario(1, {{K::Collision, 8, 101}, {K::Collision, 30, 202, false}});
    require(notPast.delay == 0, "different collision alone is not proof of clearing danger");
    auto uncertain = scenario(3, {{K::Collision, 8, 101}, {K::Unknown}, {K::Clear, 48}});
    require(uncertain.delay == 2, "unknown candidate does not cancel later valid save");
    auto timeout = scenario(4, {{K::Collision, 8, 101}, {K::Timeout}});
    require(timeout.delay == 0 && timeout.reason == Reason::Budget, "timeout passes raw input");
    auto savedBeforeTimeout = scenario(4, {{K::Collision, 8, 101}, {K::Collision, 24, 202, true}, {K::Timeout}});
    require(savedBeforeTimeout.delay == 1, "timeout retains already validated improvement");
    for (int window = 1; window <= 32; ++window) {
        for (int required = 1; required <= 33; ++required) {
            auto decision = choose(window, [&](int delay, uintptr_t) {
                return delay >= required ? Outcome{K::Clear, 60} : Outcome{K::Collision, 8, 101};
            });
            require(decision.delay == (required <= window ? required : 0), "all tick windows obey exact bound");
        }
    }
    require(ticks(-1) == 0 && ticks(99) == 32, "tick limits are clamped");
    std::cout << "PASS: cube/release/obstacle/budget scenarios and 1056 tick-window cases\n";
}
