// VisKey for Windows - pure logic of the update check: version parsing, picking the newest
// release from the GitHub reply, the 24 h throttle of the automatic check and the "notify once
// per version" rule. No Windows headers, so windows/tests/test_update_policy.cpp can build it on
// any host (MIT License, 1VISION).
#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace vk {
namespace update {

constexpr int64_t kAutoCheckIntervalSec = 24 * 60 * 60;  // at most one automatic check per day

// "v1.2.3", "1.2", "1.2.3-beta.1" -> {1, 2, 3}. False if s does not start with a number.
inline bool ParseVersion(const std::wstring& s, int v[3]) {
    v[0] = v[1] = v[2] = 0;
    size_t i = 0;
    if (i < s.size() && (s[i] == L'v' || s[i] == L'V')) ++i;
    for (int part = 0; part < 3; ++part) {
        if (i >= s.size() || s[i] < L'0' || s[i] > L'9') return part > 0;
        int n = 0;
        while (i < s.size() && s[i] >= L'0' && s[i] <= L'9' && n < 100000000) n = n * 10 + (s[i++] - L'0');
        v[part] = n;
        if (i < s.size() && s[i] == L'.') ++i;
        else return true;
    }
    return true;
}

inline int CompareVersion(const int a[3], const int b[3]) {
    for (int i = 0; i < 3; ++i) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

// True if version string a is strictly newer than b (an unparsable b counts as older than anything).
inline bool IsNewer(const std::wstring& a, const std::wstring& b) {
    int va[3], vb[3];
    if (!ParseVersion(a, va)) return false;
    if (!ParseVersion(b, vb)) return true;
    return CompareVersion(va, vb) > 0;
}

inline std::wstring FormatVersion(const int v[3]) {
    return std::to_wstring(v[0]) + L"." + std::to_wstring(v[1]) + L"." + std::to_wstring(v[2]);
}

// Newest "tag_name" in the body of GET /repos/.../releases. Every listed release counts,
// pre-releases included (VisKey publishes its builds as GitHub pre-releases).
// Returns false if no tag parses as a version.
inline bool PickLatestRelease(const std::string& body, std::wstring& tag, std::wstring& latest) {
    int best[3] = {-1, -1, -1};
    size_t pos = 0;
    while ((pos = body.find("\"tag_name\"", pos)) != std::string::npos) {
        pos += 10;
        size_t colon = body.find(':', pos);
        size_t q1 = colon == std::string::npos ? colon : body.find('"', colon);
        size_t q2 = (q1 == std::string::npos) ? q1 : body.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        std::string raw = body.substr(q1 + 1, q2 - q1 - 1);
        std::wstring t(raw.begin(), raw.end());  // tags are ASCII; anything else will not parse
        int v[3];
        if (ParseVersion(t, v) && CompareVersion(v, best) > 0) {
            std::memcpy(best, v, sizeof(best));
            tag = t;
        }
        pos = q2;
    }
    if (best[0] < 0) return false;
    latest = FormatVersion(best);
    return true;
}

// Automatic check: due when enabled and the last attempt (manual or automatic, successful or
// not) is at least 24 h old. lastAttempt == 0 means never. A last attempt far in the future
// (clock moved back by more than a day) is treated as stale so the check cannot stall forever;
// a smaller backwards jump just waits.
inline bool AutoCheckDue(bool enabled, int64_t now, int64_t lastAttempt) {
    if (!enabled || now <= 0) return false;
    if (lastAttempt <= 0) return true;
    if (lastAttempt > now + kAutoCheckIntervalSec) return true;
    return now - lastAttempt >= kAutoCheckIntervalSec;
}

// Tray balloon after an automatic check: only for a release newer than the running build and
// newer than the last version already announced.
inline bool ShouldNotify(const std::wstring& latest, const std::wstring& current, const std::wstring& lastNotified) {
    if (!IsNewer(latest, current)) return false;
    return lastNotified.empty() || IsNewer(latest, lastNotified);
}

// Version to advertise (nav dot, flyout line) when no check ran in this session yet: the last
// announced version, as long as it is still newer than the running build.
inline bool KnownNewerVersion(const std::wstring& lastNotified, const std::wstring& current) {
    return !lastNotified.empty() && IsNewer(lastNotified, current);
}

}  // namespace update
}  // namespace vk
