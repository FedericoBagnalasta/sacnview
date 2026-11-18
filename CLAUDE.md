# CLAUDE.md — fork sACNView

Fork di `docsteer/sacnview`. Lavoro sul branch `feature/universe-sync-tx`. Documentazione in italiano, codice e commenti in inglese.

## Dove sta cosa
- `src/sacn/sacnsender.*`: `CStreamServer` (thread `TickLoop`, unico socket TX) e `sACNSentUniverse` (una sorgente).
- `src/sacn/sacnsynchronization.*`: pacchetti di sync TX/RX. `src/sacn/sacnlistener.*`: ricezione.
- `src/ui/transmitwindow.*` + `ui/transmitwindow.ui`, `src/ui/multiuniverse.*`.
- `docs/`: un file per funzione + `docs/storico-commit.md`.

## Regole da rispettare
- Sync e Force Sync solo con E1.31 ratified e start code 0x00; altrove indirizzo 0 e bit 0.
- Sync: un flusso per (indirizzo, destinazione) con `m_syncCid`, inviato dopo i dati sullo stesso socket.
- Mai Stream_Terminated su un flusso 0xDD a trasmissione in corso (manda in perdita dati l'intera sorgente): usa `RemoveUniverse()`.
- Gli handle di `CStreamServer` vengono riusati: non usarli dopo lo stop.
- Finestra Transmit: durante la trasmissione `setUniverseOptsEnabled()` disattiva i figli di `frSourceOpts` uno per uno; i controlli modificabili dal vivo vanno in `liveEditable`.
- `moc`: nelle sezioni `slots` solo slot; `slots` è una macro, non usarla come nome.

## Build e verifica
- Non tentare la build macOS.
- Servono Qt 6 recente (la 6.4 fallisce sulle macro di traduzione) e i submodule `libs/blake2` e `themes/Qt-Frameless-Window-DarkStyle` (`tools/depot_tools` non serve).
- Se la build completa non è possibile: `g++ -fsyntax-only` sui file modificati (con l'header da `uic` per le finestre), più un test a livello di pacchetti su loopback che usa `sACNSentUniverse`.

### Build Windows (guida generata, non verificata)
Guida generata da Claude leggendo `README.md`, `CMakePresets.json` e `.github/workflows/build.yml`; i comandi non sono stati eseguiti su Windows. Dettagli in `docs/build-windows.md`.
1. Prerequisiti: Visual Studio 2022 (C++), Qt 6.8.3+ `msvc2022_64` con Qt Multimedia, CMake, Git; NSIS + plugin SimpleFW solo per l'installer.
2. `git clone --recurse-submodules -b feature/universe-sync-tx https://github.com/FedericoBagnalasta/sacnview.git`
3. `set QT_ROOT_DIR=C:\Qt\6.8.3\msvc2022_64`, poi "x64 Native Tools Command Prompt for VS 2022" (non x86).
4. `cmake --preset windows-x64-release-installer` (oppure `windows-x64-release` / `windows-x64-debug`).
5. `cd out\build\windows-x64-release-installer` e `cmake --build .`; l'installer esce in `install\win64\sACNView_<versione>.exe` se `makensis` è nel `PATH`.

## A ogni commit
1. Aggiorna o crea `docs/<funzione>.md`: cosa cambia, come funziona, test eseguiti, limiti.
2. Aggiungi la riga in `docs/storico-commit.md`: il commit corrente è "(questo commit)", il commit successivo la sostituisce con l'hash.
3. Copia i documenti nel Project "sACN View" (`claude/*.md`, già con l'hash).
4. Aggiorna questo file se cambiano struttura, regole o procedura.
5. Commit con attribuzione, push su `feature/universe-sync-tx`.
