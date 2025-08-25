# Batch Starter (Qt)

**Deutsch** | [English](README.en.md)

Ein kleines Windows-Tool, um mehrere Programme mit einem Klick zu starten. Apps im Baum durchsuchen, auswählen und eine `.bat` exportieren – mit optionalen Admin-Rechten (UAC) sowie pro App eigenen Argumenten.

## Funktionen
- **Layout:** Suchfeld + Buttons oben; **Tree (oben)** und **Auswahlliste (unten)**, getrennt durch einen **vertikalen QSplitter** (Höhe frei ziehbar).
- **Suche:** case-insensitive, matcht **nur Apps**; Ordner bleiben als Container sichtbar, wenn ein Kind passt.
- **Pro App:** Checkbox (einschließen), **UAC-Flag**, Argumente.
- **Export als .BAT**  
  - Wenn **mindestens eine** App UAC braucht → **eine** UAC-Abfrage am Anfang. UAC-Apps laufen erhöht; Nicht-UAC-Apps werden **absichtlich unelevated** über die Explorer-Shell gestartet.
  - Wenn **keine** UAC braucht → alle Apps starten normal (ohne Abfrage).

## Wie die exportierte Batch funktioniert
- **Self-Elevate-Header** (nur bei Bedarf).
- **Erhöhte App**:
  ```bat
  start "" "C:\Path\App\app.exe" --args

    Nicht-UAC-App unelevated, wenn Batch erhöht ist:

    call :RunUnelevated "C:\Path\App\app.exe" "--args"

Die Batch schließt sich nach dem Start aller Programme (exit). Ein goto :eof verhindert, dass man in das Helper-Label „hineinfällt“.

    Hinweise:
    • Drag&Drop zwischen erhöhten und normalen Fenstern ist durch Windows (UIPI) blockiert.
    • Gemappte Netzlaufwerke können im Admin-Token fehlen – besser UNC (\server\share) nutzen.
    • Wenn Umgebungsvariablen wie %USERPROFILE% in Args absichtlich expandieren sollen, % in Args nicht zu %% escapen.

Anforderungen

    OS: Windows 10/11

    Build: Qt 6.x (Widgets), MSVC oder MinGW

    Tools: Qt Creator oder CMake/Ninja

Build

    Qt 6.x installieren (MSVC empfohlen).

    Projekt in Qt Creator öffnen und bauen – oder mit CMake konfigurieren.

Nutzung

    App starten.

    Über das Suchfeld filtern.

    Gewünschte Apps anhaken; UAC und Args setzen.

    Batch speichern klicken.

    .bat ausführen; ggf. einmal UAC bestätigen.

Lizenz

MIT Lizenz