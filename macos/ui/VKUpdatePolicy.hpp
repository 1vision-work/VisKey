// VKUpdatePolicy.hpp — pure decision logic of the automatic update check (no Apple APIs, so it is
// unit-tested by tests/test_update_policy.cpp on every platform). MIT License, 1VISION.

#pragma once

#include <string>

namespace vkupdate {

/// Minimum time between two update checks (manual or automatic), in seconds.
constexpr double kMinCheckInterval = 24.0 * 60.0 * 60.0;

/// Should the automatic check run now?
/// `now` / `lastAttempt` are seconds since 1970; `lastAttempt <= 0` means "never checked".
/// Any attempt counts, whether it succeeded or failed. If the stored stamp lies more than a day in
/// the future (the clock was set back), it is treated as stale so checks do not stop forever.
inline bool ShouldAutoCheck(bool enabled, double now, double lastAttempt) {
    if (!enabled) return false;
    if (lastAttempt <= 0.0) return true;
    double elapsed = now - lastAttempt;
    if (elapsed >= kMinCheckInterval) return true;
    return elapsed <= -kMinCheckInterval;
}

/// Should a newer release found by the automatic check raise the (non-intrusive) notice?
/// At most once per version: `lastNotified` is the persisted version that was last announced.
inline bool ShouldNotify(bool newerThanCurrent, const std::string &latest, const std::string &lastNotified) {
    return newerThanCurrent && !latest.empty() && latest != lastNotified;
}

}  // namespace vkupdate
