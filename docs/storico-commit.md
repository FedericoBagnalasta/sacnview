# Storico dei commit — branch `feature/universe-sync-tx`

Modifiche di questo fork rispetto a sACNView upstream (`docsteer/sacnview`, ultimo commit upstream
incluso: `9eac6e0` "#390 Correct MacOS Signing"). Dal più vecchio al più recente.

| Commit | Contenuto | Documentazione |
|---|---|---|
| `6d6f5d7` | Prima implementazione della trasmissione sincronizzata (E1.31-2018 sez. 11): Synchronization Address nei pacchetti dati, gruppi di sync nel `TickLoop`, pacchetti di sync dopo i dati, UI Transmit e Multi Universe, correzioni byte order e lunghezza 49 byte. Un flusso di sync per ogni sorgente (CID). | [universe-synchronization.md](universe-synchronization.md) |
| `57bcff5` | CI: nome dell'installer Windows allineato a `install.nsi` (`sACNView_<version>.exe`). | — |
| `f22ac21` | Un solo flusso di sync per (sync address, destinazione), condiviso da tutte le sorgenti, con CID dedicato `m_syncCid`. | [universe-synchronization.md](universe-synchronization.md) |
| `e64e9de` | Bit Force_Synchronization: pulsante **Force Sync** nella finestra Transmit, modificabile anche durante la trasmissione; il bit esce solo per universi sincronizzati. | [universe-synchronization.md](universe-synchronization.md) |
| `8b21d8c` | Cambio Per-Source ↔ Per-Address durante la trasmissione, con passaggio a Per-Source senza Stream_Terminated (valori 0xDD piatti per 3 s, poi chiusura silenziosa); priorità per indirizzo e priorità della sorgente modificabili durante la trasmissione. Correzioni: `stopSending()` su sorgente ferma terminava un'altra sorgente; primo pacchetto 0xDD a zero; copia di 512 byte in un buffer 0xDD più corto. | [priority-mode-runtime.md](priority-mode-runtime.md) |
| *(questo commit)* | Aggiunto `CLAUDE.md` (struttura, regole, verifica e procedura per le modifiche future). | [../CLAUDE.md](../CLAUDE.md) |

## Stato attuale (riepilogo)

- **Trasmissione sincronizzata**: Sync Universe nelle finestre Transmit e Multi Universe; un flusso
  di sync per indirizzo e destinazione, con CID dedicato; pulsante Force Sync nella Transmit.
- **Priorità**: modalità Per-Source / Per-Address, priorità per indirizzo e priorità della sorgente
  modificabili durante la trasmissione nella finestra Transmit.
- **Verifica**: test a livello di pacchetti su loopback (non incluso nel repository); interfaccia
  controllata generando il form con `uic`. L'applicazione completa non è stata compilata in questo
  ambiente (Qt 6.4 troppo vecchio per le macro di traduzione del progetto). La build macOS non va
  tentata.
