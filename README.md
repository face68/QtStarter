# Batch Starter (Qt)

**Deutsch** | [English](README.en.md)

Ein kleines Windows-Tool, um mehrere Programme mit einem Klick zu starten. Apps im Baum durchsuchen, auswählen und eine `.bat` exportieren – mit optionalen Admin-Rechten (UAC) sowie pro App eigenen Argumenten und Arbeitsverzeichnissen.

## Funktionen
- **Layout:** Suchfeld + Buttons oben; **Tree (oben)** und **Auswahlliste (unten)**, getrennt durch einen **vertikalen QSplitter** (Höhe frei ziehbar).
- **Suche:** case-insensitive, matcht **nur Apps**; Ordner bleiben als Container sichtbar, wenn ein Kind passt.
- **Pro App:** Checkbox (einschließen), **UAC-Flag**, Argumente, Working Directory.
- **Export als .BAT**  
  - Wenn **mindestens eine** App UAC braucht → **eine** UAC-Abfrage am Anfang. UAC-Apps laufen erhöht; Nicht-UAC-Apps werden **absichtlich unelevated** über die Explorer-Shell gestartet.
  - Wenn **keine** UAC braucht → alle Apps starten normal (ohne Abfrage).
  - Working Dirs werden in beiden Fällen korrekt berücksichtigt (`start /D` bzw. beim unelevated-Start per ShellExecute).

## Wie die exportierte Batch funktioniert
- **Self-Elevate-Header** (nur bei Bedarf):
  ```bat
  whoami /groups | find "S-1-16-12288" >NUL
  if not %errorlevel%==0 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
  )

    Erhöhte App (Beispiel mit Working Dir):

start "" /D "C:\\Path\\App" "C:\\Path\\App\\app.exe" --args

Nicht-UAC-App unelevated, wenn Batch erhöht ist (vereinfacht):

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

    Die Batch schließt sich nach dem Start aller Programme (exit). Ein goto :eof verhindert, dass man in das Helper-Label „hineinfällt“.

    Hinweise:
    • Drag&Drop zwischen erhöhten und normalen Fenstern ist durch Windows (UIPI) blockiert.
    • Gemappte Netzlaufwerke können im Admin-Token fehlen – besser UNC (\server\share) nutzen.
    • Wenn Umgebungsvariablen wie %USERPROFILE% in Args absichtlich expandieren sollen, % in Args nicht zu %% escapen.

Anforderungen

    OS: Windows 10/11 (für die ausgeführten Batches)

    Build: Qt 6.x (Widgets), MSVC oder MinGW

    Tools: Qt Creator oder CMake/Ninja

Build (kurz)

    Qt 6.x installieren (MSVC empfohlen).

    Projekt in Qt Creator öffnen und bauen – oder mit CMake konfigurieren (achte auf CMAKE_PREFIX_PATH für Qt).

Verwendung

    App starten.

    Oben im Suchfeld filtern (Ordner werden passend eingeblendet).

    Gewünschte Apps anhaken; UAC, Args, Working Dir setzen.

    Batch speichern klicken.

    .bat ausführen; ggf. einmal UAC bestätigen.

Fehlerbehebung

    CMD bleibt offen: Batch endet mit exit (schon umgesetzt).

    Explorer öffnet C:\\Windows\\System32: entsteht durch „Hineinfallen“ ins Label – Generator setzt goto :eof.

    Anführungszeichen/Prozent in Args: werden für cmd.exe escaped. Für ENV-Expansion % in Args nicht verdoppeln (optional im Code schaltbar).

Lizenz

(Füge hier deine Lizenz ein, z. B. MIT.)