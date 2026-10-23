# Contributing to VisKey

Thanks for helping improve VisKey! Issues and pull requests are welcome, in Vietnamese or English.

## Review policy

- **Every external pull request must be reviewed and approved by a maintainer before it is
  merged.** Maintainers are [datntpro](https://github.com/datntpro) and members of the
  [1vision-work](https://github.com/1vision-work) GitHub organization.
- Nothing is merged to `master` without passing CI (`.github/workflows/ci.yml` and
  `.github/workflows/windows.yml`).
- Release binaries are built only by GitHub Actions. This review requirement is part of the
  project's [code signing policy](CODE_SIGNING_POLICY.md).

## Before you open a pull request

1. Keep the change focused; explain what it does and why.
2. Run the tests (see README section 7):
   ```bash
   python3 tests/test_python_engine.py
   python3 tests/test_linux_settings.py
   python3 tests/test_linux_update_check.py
   python3 tests/gen_typing_regression.py --check
   python3 tests/test_typing_regression.py
   python3 tests/test_design_tokens.py
   cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
   ```
   The C++ (`core/viskey_engine.hpp`) and Python (`core/viskey_engine.py`) engines must behave the
   same: change both together.
3. **Network and privacy:** VisKey only contacts the GitHub releases API for the update check.
   A change that adds, removes or alters any network access or the data stored on the user's
   machine must update [PRIVACY.md](PRIVACY.md) in the same pull request.
4. Do not add third-party libraries, fonts, icons or binaries, and never commit secrets,
   certificates or build outputs.

## License

By contributing you agree that your contribution is released under the [MIT License](LICENSE).
