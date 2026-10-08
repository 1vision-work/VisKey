# Architecture Decision Records

Short ADRs for decisions not covered by CLAUDE.md. Newest last.

## ADR-001 — Where CLAUDE.md and the design doc disagree, CLAUDE.md wins
Status: accepted. Example: paced delay defaults are 3 ms (terminal) / 8 ms (terminal inside IDE).

## ADR-002 — Policy schema gains `axDescriptionContains`
Status: accepted. The design doc uses it for terminals embedded in IDEs; CLAUDE.md §4.5 omits it.
Rule order is kept: terminal-in-JetBrains (paced) is evaluated before generic `com.jetbrains.*` (selection).
Terminal-in-IDE rules are finalised only after `Tools/axdump` output (M1).

## ADR-003 — Input Monitoring permission
Status: accepted. Check with `CGPreflightListenEventAccess()`; only call
`CGRequestListenEventAccess()` when it is actually missing; onboarding shows this step only when needed.

## ADR-004 — G1 snapshots cover the 5 existing code tables only
Status: accepted. VIQR/VISCII/NCR arrive in M4.

## ADR-005 — Snapshots record original engine behaviour, bugs included
Status: accepted. Known engine bugs are tagged `[known-bug]` in the snapshot and fixed
in a later, separate commit that updates the snapshot with a reason.

## ADR-006 — Work split: Linux session does M0 steps 1–5, macOS machine does the rest
Status: accepted. The Linux session delivers engine + C API + Catch2 + ubuntu CI (PR "M0a").
Swift/Xcode work (M0 step 6 onward) is done on a Mac. The macOS CI job is committed with `if: false`
until Swift code exists.
