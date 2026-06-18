// Placeholder test — satisfies CMake's file-existence check for the Tests target
// during Phase 2 skeleton setup, before any real DSP classes exist.
// Replace/supplement with real tests starting Phase 4 — see .claude/skills/dsp-testing/SKILL.md

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test harness is wired up correctly", "[placeholder]") {
    REQUIRE(1 + 1 == 2);
}
