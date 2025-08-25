# Contributing to Batch Starter (Qt)

Thanks for your interest! Contributions are welcome — from bug reports to PRs.

## How to Contribute

### Issues
- **Bug:** steps to reproduce, expected vs. actual, OS/Qt version.
- **Feature:** describe the use-case.

### Pull Requests
1. Fork, branch (`feat/…`, `fix/…`).
2. Keep PRs small/focused.
3. Follow code style. Build locally and test: load/save batch, UAC+non-UAC mix, search, expand.
4. Reference related issues.

### Code Style
- Qt 6.x, C++17.
- Prefer Qt idioms (`QString`, `QVector`, signals/slots).
- Tidy `buildUi()`.
- Escape arguments carefully; allow `%ENV%` expansion when intended.

### UI/UX
- Search matches **apps only**.
- Tree (top) + selection list (bottom) via vertical `QSplitter`.
- Headers/rows consistent via stylesheet.
- When exporting `.bat`:
  - Insert self-elevate header only if any selected app requires UAC.
  - UAC apps elevated via `start`, non-UAC apps unelevated via helper label.
  - Escape args for cmd.exe, but don’t double `%` if ENV expansion is wanted.

### License
By contributing you agree your code is under MIT.