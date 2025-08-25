# Contributing to Batch Starter (Qt)

Thanks for your interest! Contributions are welcome — from bug reports to PRs.

## How to Contribute

### 1) Issues
- **Bug:** describe steps to reproduce, expected vs. actual behavior, logs/screenhots, OS/Qt version.
- **Feature:** describe the use-case; keep scope small where possible.

### 2) Pull Requests
1. Fork and create a feature branch: `feat/…`, `fix/…`, or `chore/…`.
2. Keep PRs focused and small. One logical change per PR.
3. Follow code style (below). Build locally and test the UI flows (load/save batch, UAC+non-UAC mix, search, expand).
4. Reference related issues in the PR description.

### Code Style (C++ / Qt)
- Qt 6.x, C++17.
- Prefer Qt types and idioms (`QString`, `QVector`, `QPointer`, signals/slots).
- Keep UI wiring in `buildUi()` tidy; avoid work in constructors beyond setup.
- Use `const`/`auto` sensibly; avoid raw new/delete (Qt handles ownership for most widgets).
- Consistent formatting: provide or respect a project `.clang-format` if present.
- No blocking operations on the GUI thread; batch expensive updates (e.g., use guards for bulk model changes).

### UI/UX Guidelines
- Search matches **apps only** (folders act as containers).
- Tree (top) and selection list (bottom) via **vertical `QSplitter`**; do not reintroduce fixed heights.
- Keep headers/row heights consistent via stylesheet; avoid per-row custom painting unless necessary.
- When exporting `.bat`:
  - Insert **self-elevate header** only if any selected app requires UAC.
  - Start UAC apps elevated via `start`, non-UAC apps **unelevated** via helper label (Explorer ShellExecute).
  - Respect working directories in both paths.
  - Escape arguments for `cmd.exe` **but** allow environment variable expansion inside args when intended.

### Commit Messages
- Use present tense, imperative mood: `fix: prevent StyleChange loop`, `feat: add vertical splitter`.
- Reference issues: `Fixes #123`.

### Development Quickstart
- Install Qt 6.x (MSVC or MinGW).
- Open in Qt Creator (or use CMake with `CMAKE_PREFIX_PATH` to your Qt).
- Build & run. Test: load a batch, export, run with mixed UAC flags.

### License
By contributing, you agree your contributions are licensed under the project’s **MIT License**.