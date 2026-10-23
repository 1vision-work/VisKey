# VisKey Privacy Policy

VisKey is a Vietnamese input method (Telex/VNI) for Windows, macOS and Linux, published by 1VISION
under the MIT License. This document describes exactly what VisKey sends over the network. It is
based on the source code in this repository: if the code and this document ever disagree, that
is a bug, so please [open an issue](https://github.com/1vision-work/VisKey/issues).

## Summary

- **Your typing never leaves your computer.** Keystrokes, typed text, macros, per-app rules and
  settings are processed and stored locally only.
- **No telemetry, analytics, crash reporting, advertising or accounts.**
- **The only network request VisKey makes is an update check**: an anonymous HTTPS `GET` of the
  public release list of this repository on GitHub. It runs when you press *Kiểm tra cập nhật*
  (Check for updates), or automatically at most once every 24 hours. The automatic check can be
  switched off. VisKey never downloads or installs anything by itself.

## The update check

| | |
| :--- | :--- |
| **Endpoint** | `GET https://api.github.com/repos/1vision-work/VisKey/releases?per_page=5` (HTTPS only) |
| **Request headers set by VisKey** | `User-Agent: VisKey/<version>` (macOS: `VisKey/<version> (macOS)`) and `Accept: application/vnd.github+json` |
| **Request body** | None |
| **Not sent** | No cookies, no authentication token, no device or user identifier, no settings, no typed text, no list of applications |
| **Received** | GitHub's public JSON list of the 5 most recent releases. VisKey only reads the version tags (and, on macOS/Linux, the release page URL) to tell whether a newer version exists |
| **Result** | If a newer version exists, VisKey shows a notice (Windows: one tray balloon per version; macOS: a line in the menu-bar popover, once per version; Linux: a banner and tab badge in the settings window, plus one in-app toast per version). Clicking it opens the release page in your web browser. Nothing is downloaded or installed |
| **Timeout** | 8 seconds |

As with any HTTPS request, GitHub (and any proxy or network operator in between) can see your
IP address, the time of the request and the headers above. GitHub's handling of that data is
covered by the [GitHub Privacy Statement](https://docs.github.com/site-policy/privacy-policies/github-general-privacy-statement).
The request uses the operating system's normal network stack and proxy settings.

### When it runs

| Platform | Code | Manual check | Automatic check |
| :--- | :--- | :--- | :--- |
| **Windows** (`VisKey.exe`) | `windows/ui/store.cpp` (`FetchLatestRelease`, WinHTTP), scheduling in `windows/VisKeyWin.cpp` | *Cập nhật* page → *Kiểm tra cập nhật* | While VisKey is running (tray): first evaluated 30 s after start, then every 3 h; the request is only made if the last attempt is at least 24 h old |
| **macOS** (`VisKey.app`) | `macos/ui/VKUpdate.mm` (`VKFetchLatestRelease`, `NSURLSession` ephemeral session, no cookies, no cache) | *Giới thiệu* page → *Kiểm tra cập nhật* | While VisKey is running: evaluated 20 s after start, every 3 h, and after the Mac wakes; the request is only made if the last attempt is at least 24 h old |
| **Linux** (`viskey-settings`) | `linux/ibus/viskey_settings_model.py` (`fetch_releases`, Python `urllib`) | *Giới thiệu* tab → *Kiểm tra cập nhật* (GTK and Tk settings windows) | Only while the GTK settings window (`viskey-settings`) is open: evaluated 5 s after it opens and every 3 h; the request is only made if the last attempt is at least 24 h old. The IBus input engine itself (`ibus_viskey.py`) never uses the network |

Every attempt counts towards the 24 h limit, whether it succeeded or failed.

### How to turn it off

The automatic check is **on by default** and can be disabled at any time; the manual button keeps
working.

- **Windows:** *Cập nhật* page → switch off *Tự động kiểm tra cập nhật* (stored as
  `AutoUpdateCheck = 0` under `HKCU\Software\VisKey`).
- **macOS:** *Giới thiệu* page → switch off *Tự động kiểm tra cập nhật* (stored as
  `autoUpdateCheck` in the app's `NSUserDefaults`).
- **Linux:** *Giới thiệu* tab → switch off *Tự động kiểm tra cập nhật* (stored as
  `"auto_check_updates": false` in `~/.config/viskey/config.json`).

The UI snapshot mode (`--ui-snapshot`) never makes network requests on any platform.

## Data stored locally

VisKey keeps its settings on your computer only:

- **Windows:** registry key `HKCU\Software\VisKey` (settings, macros, macro usage counts, per-app
  rules, update-check time and last announced version); optionally the `VisKey` value under
  `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` when *start with Windows* is enabled.
- **macOS:** the app's `NSUserDefaults`; optionally
  `~/Library/LaunchAgents/org.viskey.inputmethod.plist` when *launch at login* is enabled.
- **Linux:** `~/.config/viskey/config.json` (settings, macros),
  `~/.local/share/viskey/macro_usage.json` (macro usage counts), `~/.cache/viskey/status.json`
  (engine status) and `~/.local/state/viskey/update_check.json` (update-check time and result).

None of this is transmitted anywhere. Uninstalling VisKey and deleting these locations removes it.

## Other connections

- **Links you click** (project page, issues, releases, license) are opened in your default web
  browser; VisKey itself does not fetch them.
- **Install scripts:** `scripts/install_linux.sh` runs `apt-get update` / `apt-get install` for the
  dependencies (ibus, python3-gi, ...) through your distribution's package manager. This only
  happens when you run that script yourself; the installed input method does not do it.

## Changes

Any change to VisKey's network behaviour must update this document in the same pull request
(see [CONTRIBUTING.md](CONTRIBUTING.md)).
