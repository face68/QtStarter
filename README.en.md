# Batch Starter (Qt)

**English** | [Deutsch](README.md)

A small Windows tool to start multiple programs with one click. Browse apps in the tree, select them, and export a `.bat`—with optional admin rights (UAC), per-app arguments, and (for manually added apps) a custom working directory.

## Features
- **Layout:** Search field + buttons at the top; **Tree (top)** and **Selection list (bottom)** separated by a **vertical QSplitter**.
- **Search:** Case-insensitive; matches **apps only**. Folders stay visible as containers when a child matches.
- **Per app in the selection list:** Checkbox (include), **UAC flag**, **Args**, **WorkDir** *(editable only for manually added apps)*.
- **Add EXE/BAT:** Add files that are not in the Start Menu. These entries appear **only in the selection list** (hidden in the tree). Duplicates are allowed (e.g., several `cmd.exe` with different args).
- **Export to .BAT (per entry):**
  - **UAC off:** normal start (`start "" ...`).
  - **UAC on:** that entry is started elevated (PowerShell `Start-Process ... -Verb RunAs`).
  - There is **no global** self-elevate header anymore.

## How the exported batch works
A comment line with meta info precedes each start, e.g.:
REM Name=<NAME> | Manual=<0|1> | Args=<ARGS>

Unelevated (UAC off):
start "" /D "E:\Optional\WorkingDir" "C:\Path\To\App.exe" --args

Elevated (UAC on):
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "Start-Process -FilePath 'C:\Path\To\App.exe' -WorkingDirectory 'E:\Optional\WorkingDir' -ArgumentList '--args' -Verb RunAs"

Notes:
- A bare drive like `E:` is normalized to `E:\` for `WorkDir`.
- Manually added entries have an editable **WorkDir** in the list; tree entries use the working directory from their `.lnk` (not editable).
- Drag & drop between **elevated** and **normal** windows is blocked by Windows (UIPI).
- Mapped network drives may be missing in elevated processes; consider using UNC paths (`\\server\share`).

## Requirements
- **OS:** Windows 10/11
- **Build:** Qt 6.x (Widgets), MSVC or MinGW
- **Tools:** Qt Creator or CMake/Ninja

## Build
1. Install Qt 6.x (MSVC recommended).
2. Open and build the project in Qt Creator — or configure with CMake.

## Usage
1. Launch the app.
2. Filter via the search field.
3. Tick desired apps; set **UAC** and **Args**.
4. Use **Add EXE/BAT** for items not in the tree; edit **WorkDir** in the list (manual entries only).
5. Click **Save Batch**.
6. Run the `.bat`; UAC prompts appear **per app** that requires elevation.

## License
MIT License
