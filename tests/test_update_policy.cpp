// Unit tests for the automatic update check policy (macos/ui/VKUpdatePolicy.hpp). MIT License, 1VISION.

#include "../macos/ui/VKUpdatePolicy.hpp"
#include <iostream>

using namespace vkupdate;

static int failures = 0;

static void check(bool ok, const char *what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        failures++;
    }
}

int main() {
    const double day = kMinCheckInterval;
    const double now = 1792396800.0;  // 2026-10-19 00:00 UTC

    // 24 h throttle
    check(day == 86400.0, "interval is 24 hours");
    check(ShouldAutoCheck(true, now, 0.0), "never checked -> check");
    check(ShouldAutoCheck(true, now, -5.0), "invalid stamp -> check");
    check(!ShouldAutoCheck(false, now, 0.0), "disabled -> never check");
    check(!ShouldAutoCheck(false, now, now - 10 * day), "disabled -> never check, even when overdue");
    check(!ShouldAutoCheck(true, now, now), "just checked -> wait");
    check(!ShouldAutoCheck(true, now, now - 60.0), "1 minute ago -> wait");
    check(!ShouldAutoCheck(true, now, now - 3 * 3600.0), "3 hours ago (timer tick) -> wait");
    check(!ShouldAutoCheck(true, now, now - day + 1.0), "1 second short of 24 h -> wait");
    check(ShouldAutoCheck(true, now, now - day), "exactly 24 h -> check");
    check(ShouldAutoCheck(true, now, now - 3 * day), "3 days ago -> check");
    check(!ShouldAutoCheck(true, now, now + 3600.0), "small clock skew backwards -> wait");
    check(!ShouldAutoCheck(true, now, now + day - 1.0), "stamp < 24 h in the future -> wait");
    check(ShouldAutoCheck(true, now, now + day), "clock set back by >= 24 h -> check once");
    check(ShouldAutoCheck(true, now, now + 365 * day), "clock set back by a year -> check once");

    // Simulated timer: ticks every 3 h for 3 days; at most one attempt per 24 h.
    double last = now - 3600.0;  // checked one hour before "now"
    int attempts = 0;
    double prev = last;
    bool spacingOk = true;
    for (double t = now; t <= now + 3 * day; t += 3 * 3600.0) {
        if (ShouldAutoCheck(true, t, last)) {
            if (t - prev < day) spacingOk = false;
            prev = t;
            last = t;
            attempts++;
        }
    }
    check(attempts == 3, "3 attempts in 3 days of 3-hourly ticks");
    check(spacingOk, "attempts are at least 24 h apart");

    // Notify once per version
    check(ShouldNotify(true, "1.1.0", ""), "first newer version -> notify");
    check(!ShouldNotify(true, "1.1.0", "1.1.0"), "same version already notified -> silent");
    check(ShouldNotify(true, "1.2.0", "1.1.0"), "another newer version -> notify again");
    check(!ShouldNotify(false, "1.1.0", ""), "not newer -> silent");
    check(!ShouldNotify(false, "1.0.0", "0.9.0"), "current version -> silent");
    check(!ShouldNotify(true, "", ""), "empty version -> silent");

    // Simulated sequence of automatic checks returning the same release
    std::string lastNotified;
    int notices = 0;
    const char *found[] = {"1.1.0", "1.1.0", "1.1.0", "1.2.0", "1.2.0"};
    for (const char *v : found) {
        if (ShouldNotify(true, v, lastNotified)) {
            notices++;
            lastNotified = v;
        }
    }
    check(notices == 2, "two distinct versions -> exactly two notices");
    check(lastNotified == "1.2.0", "last notified version persisted");

    if (failures) {
        std::cerr << failures << " update policy test(s) failed\n";
        return 1;
    }
    std::cout << "Update policy tests passed.\n";
    return 0;
}
