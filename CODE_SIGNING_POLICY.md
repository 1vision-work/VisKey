# Code Signing Policy

> **Status: planned, application pending.** VisKey has not yet been accepted by the SignPath
> Foundation, and **current releases are not code-signed** (Windows shows a SmartScreen prompt;
> the macOS app is only ad-hoc signed). This page describes the policy that will apply to Windows
> release binaries once signing is in place.

Once approved, the following will apply:

**Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/)**

## What gets signed

Only binaries built from this repository by its GitHub Actions workflows
([`.github/workflows/windows.yml`](.github/workflows/windows.yml)) from a tagged commit on the
default branch:

- `VisKey.exe` (inside `VisKey-<version>-Windows-<arch>-portable.zip`)
- `VisKey-<version>-Windows-Setup.exe` (Inno Setup installer)

No binaries built outside CI, and no third-party binaries, are signed. VisKey does not bundle
third-party libraries (see [README.md](README.md#1-giấy-phép--mã-nguồn)).

## Team roles

| Role | Members |
| :--- | :--- |
| **Committers and reviewers** | [datntpro](https://github.com/datntpro) and members of the [1vision-work](https://github.com/1vision-work) GitHub organization |
| **Approvers** (approve each signing request) | [datntpro](https://github.com/datntpro) |

## Source and review requirements

- All source code is public in this repository under the MIT License.
- **All external contributions require review.** Pull requests from anyone outside the team must
  be reviewed and approved by a maintainer before they are merged (see
  [CONTRIBUTING.md](CONTRIBUTING.md)).
- Release builds are produced only by GitHub Actions from the repository contents; signing
  requests are approved manually by an Approver for each release.
- Committers, reviewers and approvers must enable multi-factor authentication on their GitHub
  accounts.

## Privacy

See [PRIVACY.md](PRIVACY.md) for the full policy. In short: VisKey processes keystrokes locally
and never transmits typed text, settings or any identifier. The only network connection it makes
is an anonymous HTTPS request to the public GitHub releases API
(`api.github.com/repos/1vision-work/VisKey/releases`) to check for a newer version — when the
user presses *Check for updates*, or automatically at most once per 24 hours. The automatic check
can be turned off in the settings. VisKey never downloads or installs updates by itself.

This program will not transfer any other information to other networked systems unless
specifically requested by the user or the person installing or operating it.

## Reporting

To report a signed binary that does not match this policy, or a security issue, open an issue at
<https://github.com/1vision-work/VisKey/issues> or contact the maintainer
([datntpro](https://github.com/datntpro)).
