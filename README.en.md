# Batch Starter (Qt)

[Deutsch](README.md) | **English**

A small Windows utility to launch multiple programs with one click. Browse apps in a tree, choose what you need, and export a `.bat` that starts everything—optionally with admin (UAC) and per-app arguments/working directories.

## Features
- **Layout:** Search + buttons on top; **tree (top)** and **selection list (bottom)** separated by a **vertical QSplitter** (draggable height).
- **Search:** case-insensitive and matches **apps only**; folders remain as containers when a child matches.
- **Per app:** checkbox (include), **UAC flag**, arguments, working directory.
- **Export to .BAT**  
  - If **any** app needs UAC → **one** UAC prompt at start. UAC apps run elevated; non-UAC apps are launched **unelevated** via Explorer’s shell.
  - If **none** needs UAC → all apps start normally (no prompt).
  - Working directories are honored in both cases (`start /D` or via ShellExecute).

## How the exported batch works
- **Self-elevate header** (only when needed):
  ```bat
  whoami /groups | find "S-1-16-12288" >NUL
  if not %errorlevel%==0 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
  )

    Elevated app (with working dir):

start "" /D "C:\\Path\\App" "C:\\Path\\App\\app.exe" --args

Non-UAC app unelevated when batch is elevated (simplified):

    call :RunUnelevated "C:\\Path\\App\\app.exe" "--args" "C:\\Path\\App"
    ...
    :RunUnelevated
    set "exe=%~1"
    set "args=%~2"
    set "wd=%~3"
    powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -Command ^
     "$exe='%exe%'; $args='%args%'; $wd='%wd%';
      if([string]::IsNullOrWhiteSpace($args)){$args=$null};
      if([string]::IsNullOrWhiteSpace($wd)){$wd=$null};
      (New-Object -ComObject Shell.Application).ShellExecute($exe,$args,$wd,'open',1)" ^
     ""
    exit /b

    The batch closes after launching everything (exit). A goto :eof prevents falling through into the helper label.

    Notes:
    • Drag & drop between elevated and normal windows is blocked by Windows (UIPI).
    • Mapped network drives may differ under the admin token—prefer UNC paths (\server\share).
    • If you want environment variables like %USERPROFILE% to expand inside Args, don’t double the % in args (optional switch in code).

Requirements

    OS: Windows 10/11

    Build: Qt 6.x (Widgets), MSVC or MinGW

    Tools: Qt Creator or CMake/Ninja

Build (quick)

    Install Qt 6.x (MSVC recommended).

    Open the project in Qt Creator and build—or configure with CMake (CMAKE_PREFIX_PATH to Qt).

Usage

    Launch the app.

    Filter using the search (folders appear as needed).

    Check the apps; set UAC, Args, Working Dir per app.

    Click Save Batch to export.

    Run the .bat; if prompted once, click Yes.

Troubleshooting

    Command window stays open: batch ends with exit (already handled).

    Explorer opens C:\\Windows\\System32: caused by falling through into the label—generator emits goto :eof.

    Quotes/percent in args: escaped for cmd.exe. For ENV expansion, don’t double % in args (optional).

License

MIT