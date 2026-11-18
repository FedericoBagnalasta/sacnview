# Build per Windows

> **Nota:** questa guida è stata **generata da Claude** leggendo `README.md`, `CMakePresets.json` e
> `.github/workflows/build.yml` del branch `feature/universe-sync-tx`. I comandi **non sono stati
> eseguiti su Windows** dall'autore della guida: se qualcosa non torna, fanno fede README e CI.

## Prerequisiti
- Visual Studio 2022 (anche Community) con il carico di lavoro C++.
- Qt 6.8.3 o successivo, kit `msvc2022_64`, con il modulo Qt Multimedia (la CI usa 6.8.3).
- CMake e Git.
- Solo per l'installer: NSIS e il plugin SimpleFW (Simple Firewall Plugin, versione Unicode).
  Se `makensis` è nel `PATH`, l'installer viene creato dalla build.

## Passi
1. Clonare con i submodule:
   ```
   git clone --recurse-submodules -b feature/universe-sync-tx https://github.com/FedericoBagnalasta/sacnview.git
   cd sacnview
   ```
   Già clonato senza submodule: `git submodule update --init libs/blake2 themes/Qt-Frameless-Window-DarkStyle`
   (`tools/depot_tools` non serve).
2. Impostare la cartella di Qt: `set QT_ROOT_DIR=C:\Qt\6.8.3\msvc2022_64`
3. Aprire **x64 Native Tools Command Prompt for VS 2022** (non la versione x86 o senza etichetta:
   Qt non verrebbe trovato).
4. Configurare con un preset: `cmake --preset windows-x64-release-installer`
   (alternative: `windows-x64-release`, `windows-x64-debug`).
5. Compilare:
   ```
   cd out\build\windows-x64-release-installer
   cmake --build .
   ```
   Installer: `install\win64\sACNView_<versione>.exe` (solo con NSIS nel `PATH`).

## Problemi frequenti
- "Qt non trovato": `QT_ROOT_DIR` errata o prompt x86.
- Non tentare la build macOS (regola del progetto).
