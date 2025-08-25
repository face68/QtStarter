# Batch Starter (Qt)

**Deutsch** | [English](README.en.md)

Ein kleines Windows-Tool, um mehrere Programme mit einem Klick zu starten. Apps im Baum durchsuchen, auswählen und eine `.bat` exportieren – mit optionalen Admin-Rechten (UAC), pro App eigenen Argumenten und (für manuell hinzugefügte Apps) eigenem Working Directory.

## Funktionen
- **Layout:** Suchfeld + Buttons oben; **Tree (oben)** und **Auswahlliste (unten)**, getrennt durch einen **vertikalen QSplitter**.
- **Suche:** case-insensitive, matcht nur **Apps**; Ordner bleiben als Container sichtbar, wenn ein Kind passt.
- **Auswahlliste pro App:** Checkbox (einschließen), **UAC-Flag**, **Args**, **WorkDir** *(nur für manuell hinzugefügte Apps editierbar)*.
- **Add EXE/BAT:** Dateien, die nicht im Startmenü sind, kannst du manuell hinzufügen. Diese Einträge erscheinen **nur in der Auswahlliste** (werden im Tree ausgeblendet). Duplikate (z. B. mehrere `cmd.exe` mit unterschiedlichen Args) sind erlaubt.
- **Export als .BAT (pro Eintrag):**
  - **UAC aus:** normaler Start (`start "" ...`).
  - **UAC an:** dieser Eintrag wird erhöht gestartet (PowerShell `Start-Process ... -Verb RunAs`).
  - Es gibt **keinen globalen** Self-Elevate-Header mehr.

## Wie die exportierte Batch funktioniert
Vor jedem Start steht eine Kommentarzeile mit Meta-Infos, z. B.:
REM Name=<NAME> | Manual=<0|1> | Args=<ARGS>

**Unelevated (UAC aus):**
start "" /D "E:\Optionales\WorkingDir" "C:\Path\To\App.exe" --args

**Elevated (UAC an):**
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "Start-Process -FilePath 'C:\Path\To\App.exe' -WorkingDirectory 'E:\Optionales\WorkingDir' -ArgumentList '--args' -Verb RunAs"

Hinweise:
- `WorkingDir` wird bei „nackten“ Laufwerken wie `E:` zu `E:\` normalisiert.
- Manuell hinzugefügte Einträge haben ein editierbares **WorkDir** in der Liste; Tree-Einträge nutzen das aus der `.lnk` gelesene Working Directory (nicht editierbar).
- Drag&Drop zwischen **erhöhten** und **normalen** Fenstern ist durch Windows (UIPI) blockiert.
- Gemappte Netzlaufwerke fehlen in erhöhten Prozessen u. U.; ggf. UNC-Pfade (`\\server\share`) verwenden.

## Anforderungen
- **OS:** Windows 10/11
- **Build:** Qt 6.x (Widgets), MSVC oder MinGW
- **Tools:** Qt Creator oder CMake/Ninja

## Build
1. Qt 6.x installieren (MSVC empfohlen).
2. Projekt in Qt Creator öffnen und bauen – oder mit CMake konfigurieren.

## Nutzung
1. App starten.
2. Über das Suchfeld filtern.
3. Gewünschte Apps anhaken; **UAC** und **Args** setzen.
4. **Add EXE/BAT**, wenn etwas nicht im Tree ist; **WorkDir** in der Liste bearbeiten (nur bei manuellen Einträgen).
5. **Batch speichern** klicken.
6. `.bat` ausführen; UAC-Abfragen erscheinen **pro App**, die erhöhte Rechte braucht.

## Lizenz
MIT Lizenz
