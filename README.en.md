# Batch Starter (Qt)

[Deutsch](README.md) | **English**

A small Windows utility to launch multiple programs with one click. Browse apps in a tree, choose what you need, and export a `.bat` that starts everything—optionally with admin (UAC) and per-app arguments.

## Features
- **Layout:** Search + buttons on top; **tree (top)** and **selection list (bottom)** separated by a **vertical QSplitter** (draggable height).
- **Search:** case-insensitive and matches **apps only**; folders remain as containers when a child matches.
- **Per app:** checkbox (include), **UAC flag**, arguments.
- **Export to .BAT**  
  - If **any** app needs UAC → **one** UAC prompt at start. UAC apps run elevated; non-UAC apps are launched **unelevated** via Explorer’s shell.
  - If **none** needs UAC → all apps start normally (no prompt).

## How the exported batch works
- **Self-elevate header** (only when needed).
- **Elevated app**:
  ```bat
  start "" "C:\Path\App\app.exe" --args

    Non-UAC app unelevated when batch is elevated:

    call :RunUnelevated "C:\Path\App\app.exe" "--args"

The batch closes after launching everything (exit). A goto :eof prevents falling through into the helper label.

    Notes:
    • Drag & drop between elevated and normal windows is blocked by Windows (UIPI).
    • Mapped network drives may differ under the admin token—prefer UNC paths (\server\share).
    • If you want environment variables like %USERPROFILE% to expand inside Args, don’t double the % in args.

Requirements

    OS: Windows 10/11

    Build: Qt 6.x (Widgets), MSVC or MinGW

    Tools: Qt Creator or CMake/Ninja

Build

    Install Qt 6.x (MSVC recommended).

    Open in Qt Creator and build—or configure with CMake.

Usage

    Launch the app.

    Filter using the search bar.

    Check apps; set UAC and Args as needed.

    Click Save Batch to export.

    Run the .bat; if prompted once, click Yes.

License

MIT License