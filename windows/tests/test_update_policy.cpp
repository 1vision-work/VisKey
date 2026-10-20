// Unit tests for windows/ui/update_policy.h (pure logic, builds on any host):
//   g++ -std=c++17 -Wall -Wextra windows/tests/test_update_policy.cpp -o test_update_policy
// MIT License, 1VISION.
#include "../ui/update_policy.h"

#include <cstdio>

using namespace vk::update;

static int g_failed = 0, g_run = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        ++g_run;                                                       \
        if (!(cond)) {                                                 \
            ++g_failed;                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                              \
    } while (0)

static void TestParseVersion() {
    int v[3];
    CHECK(ParseVersion(L"v1.2.3", v) && v[0] == 1 && v[1] == 2 && v[2] == 3);
    CHECK(ParseVersion(L"1.10", v) && v[0] == 1 && v[1] == 10 && v[2] == 0);
    CHECK(ParseVersion(L"V2", v) && v[0] == 2 && v[1] == 0);
    CHECK(ParseVersion(L"1.2.3-beta.4", v) && v[2] == 3);
    CHECK(!ParseVersion(L"", v));
    CHECK(!ParseVersion(L"latest", v));
    CHECK(IsNewer(L"1.0.1", L"1.0.0"));
    CHECK(IsNewer(L"1.10.0", L"1.9.9"));
    CHECK(!IsNewer(L"1.0.0", L"1.0.0"));
    CHECK(!IsNewer(L"0.9.0", L"1.0.0"));
    CHECK(IsNewer(L"1.0.0", L""));
    CHECK(!IsNewer(L"", L"1.0.0"));
}

static void TestPickLatest() {
    std::wstring tag, latest;
    std::string body = "[{\"tag_name\": \"v1.0.0\"},{\"tag_name\":\"v1.2.0-rc1\"},{\"tag_name\" : \"v1.1.5\"},{\"tag_name\":\"nightly\"}]";
    CHECK(PickLatestRelease(body, tag, latest));
    CHECK(tag == L"v1.2.0-rc1");  // pre-releases count
    CHECK(latest == L"1.2.0");
    tag.clear();
    CHECK(!PickLatestRelease("[]", tag, latest));
    CHECK(!PickLatestRelease("[{\"tag_name\":\"nightly\"}]", tag, latest));
    CHECK(!PickLatestRelease("{\"tag_name\":", tag, latest));  // truncated reply
}

static void TestAutoCheckDue() {
    const int64_t day = kAutoCheckIntervalSec;
    const int64_t now = 1792400000;  // Oct 2026
    CHECK(AutoCheckDue(true, now, 0));                 // never checked
    CHECK(!AutoCheckDue(false, now, 0));               // switch off
    CHECK(!AutoCheckDue(false, now, now - 10 * day));  // switch off, long ago
    CHECK(!AutoCheckDue(true, now, now));              // just checked
    CHECK(!AutoCheckDue(true, now, now - day + 1));    // 1 s short of 24 h
    CHECK(AutoCheckDue(true, now, now - day));         // exactly 24 h
    CHECK(AutoCheckDue(true, now, now - 3 * day));
    CHECK(!AutoCheckDue(true, now, now + 3600));       // clock moved back an hour: wait
    CHECK(AutoCheckDue(true, now, now + 2 * day));     // last attempt absurdly in the future
    CHECK(!AutoCheckDue(true, 0, 0));                  // no clock
    // Startup + a timer every 3 h for 2 days: at most one check per 24 h.
    int64_t last = now - day;  // due at start
    int checks = 0;
    for (int64_t t = now; t < now + 2 * day; t += 3 * 3600) {
        if (AutoCheckDue(true, t, last)) {
            ++checks;
            CHECK(t - last >= day);
            last = t;
        }
    }
    CHECK(checks == 2);
}

static void TestNotifyOnce() {
    CHECK(ShouldNotify(L"1.1.0", L"1.0.0", L""));         // first newer release
    CHECK(!ShouldNotify(L"1.1.0", L"1.0.0", L"1.1.0"));   // already announced
    CHECK(ShouldNotify(L"1.2.0", L"1.0.0", L"1.1.0"));    // another newer release
    CHECK(!ShouldNotify(L"1.1.0", L"1.0.0", L"1.2.0"));   // older than what was announced
    CHECK(!ShouldNotify(L"1.0.0", L"1.0.0", L""));        // up to date
    CHECK(!ShouldNotify(L"0.9.0", L"1.0.0", L""));        // running a newer build
    // Simulate repeated automatic checks: one balloon per version.
    std::wstring notified;
    int balloons = 0;
    const wchar_t* seen[] = {L"1.0.0", L"1.1.0", L"1.1.0", L"1.1.0", L"1.2.0", L"1.2.0"};
    for (const wchar_t* latest : seen) {
        if (ShouldNotify(latest, L"1.0.0", notified)) {
            ++balloons;
            notified = latest;
        }
    }
    CHECK(balloons == 2);
    CHECK(KnownNewerVersion(L"1.2.0", L"1.0.0"));
    CHECK(!KnownNewerVersion(L"1.2.0", L"1.2.0"));  // user has updated
    CHECK(!KnownNewerVersion(L"", L"1.0.0"));
}

int main() {
    TestParseVersion();
    TestPickLatest();
    TestAutoCheckDue();
    TestNotifyOnce();
    std::printf("%d checks, %d failed\n", g_run, g_failed);
    return g_failed ? 1 : 0;
}
