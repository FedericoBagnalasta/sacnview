# Universe Synchronization in trasmissione (E1.31-2018, sezione 11)

## Cosa è stato aggiunto

sACNView può ora **trasmettere universi sincronizzati**: i dati di più universi vengono
applicati dai ricevitori nello stesso istante, quando arriva un pacchetto di sincronizzazione
sull'universo scelto dall'utente (Synchronization Address).

Prima di questa modifica la sincronizzazione era supportata solo in ricezione.

## Come si usa

**Finestra Transmit**
- Riga **Sync Universe**: spunta **Enable** e scegli l'universo di sincronizzazione (1-63999).
- È disponibile solo con protocollo **E1.31 (ratified)**; con Draft e Pathway Secure è disattivata.
- Come le altre opzioni della sorgente, si imposta prima di premere **Start**.
- Per sincronizzare più finestre, imposta in tutte lo stesso Sync Universe.
- Pulsante **Force Sync: Off / On** (sopra Start): imposta il bit Force_Synchronization della
  sorgente. È attivo solo con il sync abilitato e, a differenza delle altre opzioni, si può
  cambiare **anche durante la trasmissione**: il nuovo valore esce nei pacchetti dati successivi.

**Finestra Multi Universe**
- Nuova colonna **Sync Universe** per ogni riga: `Off` = non sincronizzato, altrimenti
  l'universo di sincronizzazione. Si può cambiare anche durante la trasmissione.

**Verifica**: un'altra istanza di sACNView in ricezione mostra il Synchronization Address
delle sorgenti e i pacchetti di sync ricevuti.

## Come funziona

Tutto avviene nel thread di trasmissione, `CStreamServer::TickLoop()` (`src/sacn/sacnsender.cpp`).
A ogni ciclo:

1. **Passo 1**: individua gli indirizzi di sync che hanno almeno un universo da inviare
   (dati cambiati, ripetizioni E1.31 6.6.2 o keep-alive).
2. **Invio dati**: oltre agli universi normalmente in scadenza, vengono inviati anche gli altri
   universi dello stesso gruppo di sync (se rispettano l'intervallo minimo). Così un singolo
   pacchetto di sync applica un frame coerente su tutto il gruppo.
3. **Passo 2**: solo dopo tutti i dati del ciclo viene inviato **un solo pacchetto di sync per
   ogni indirizzo di sync** (e per ogni destinazione, vedi sotto) che ha trasmesso, qualunque sia
   il numero di sorgenti sincronizzate su quell'indirizzo.

Dettagli:
- I pacchetti di sync escono dallo **stesso socket dei dati**, così l'ordine "dati poi sync"
  sul cavo è garantito.
- Destinazione: in multicast il gruppo dell'universo di sync; se la sorgente trasmette in
  unicast, il sync va allo stesso indirizzo unicast.
- **Un flusso di sync per indirizzo, condiviso da tutte le sorgenti.** La sincronizzazione
  riguarda gli universi, non le sorgenti: il primo pacchetto di sync rilascia tutti i dati in
  attesa sull'indirizzo, quindi altri pacchetti nello stesso ciclo sarebbero solo traffico in più
  (con la Multi Universe ogni riga ha un CID diverso: N righe sullo stesso indirizzo producevano
  N sync per ciclo).
- La chiave del flusso è (indirizzo di sync, destinazione): un flusso per il gruppo multicast,
  più uno per ogni destinazione unicast distinta usata dalle sorgenti del gruppo.
- **CID dedicato al sync**: tutti i flussi usano un CID proprio del server di trasmissione
  (`CStreamServer::m_syncCid`, generato all'avvio), diverso da quelli delle sorgenti dati. Così
  l'identità del flusso e il numero di sequenza non dipendono da quali sorgenti sono attive:
  fermare o avviare una sorgente non cambia il CID del sync né fa ripartire la sequenza.
  I ricevitori conformi applicano il sync in base all'indirizzo, non al CID (anche il ricevitore
  di sACNView si comporta così); ricevitori che accettano il sync solo dal CID della sorgente dati
  non sono compatibili con questa scelta.
- I sender di sync (`sACNSynchronizationTX`) restano attivi per tutta la vita del server, così
  il numero di sequenza non riparte da zero.
- Il Synchronization Address è inserito solo nei pacchetti E1.31 ratified con start code 0x00;
  per Draft, Pathway Secure e i dati di priorità per indirizzo (0xDD) resta 0, altrimenti i
  ricevitori attenderebbero un sync che non arriva.

### Force Synchronization

Bit 5 (0x20) del campo Options dei pacchetti dati (E1.31-2018 §6.2.6). Indica ai ricevitori
cosa fare quando la sincronizzazione si perde, cioè quando smettono di arrivare i pacchetti di sync:
- **0 (Off, default)**: i ricevitori che stavano lavorando sincronizzati non aggiornano le uscite
  con i nuovi pacchetti finché la sincronizzazione non riprende;
- **1 (On)**: i ricevitori tornano al funzionamento non sincronizzato e applicano subito i dati
  successivi, senza attendere un nuovo pacchetto di sync.

(Definizione riportata dal testo dello standard come ricordato durante lo sviluppo: verificarla
sul documento ESTA se serve un riferimento formale.)

Implementazione:
- `sACNSentUniverse::setForceSync(bool)` / `forceSync()`, segnale `forceSyncChange()`. All'avvio
  il bit è passato nelle options della `CreateUniverse` del solo universo DMX (0x00); durante la
  trasmissione chiama `CStreamServer::setForceSynchronization()`.
- `CStreamServer` memorizza la richiesta per universo (`universe::force_sync`) e scrive il bit con
  `applyForceSync()`, che lo imposta solo se l'universo è sincronizzato: E1.31 ratified, start code
  0x00, Synchronization Address diverso da 0. In tutti gli altri casi il bit resta a 0: sorgenti
  senza sync, Draft, Pathway Secure, pacchetti di priorità per indirizzo (0xDD).
- `applyForceSync()` viene richiamata anche da `setSynchronizationAddress()`: portando il sync a
  `Off` durante la trasmissione il bit si azzera, riattivandolo torna al valore richiesto.
- Ogni modifica marca l'universo come dirty, quindi il nuovo valore esce al ciclo successivo.
- Interfaccia (`ui/transmitwindow.ui`, `src/ui/transmitwindow.*`): pulsante checkable
  `btnForceSync` posto fuori dal riquadro `frSourceOpts`, che durante la trasmissione viene
  disattivato. Abilitazione in `updateSyncControls()` (ratified + sync abilitato), slot
  `on_btnForceSync_toggled()`.
- La finestra Multi Universe non ha ancora un comando per questo bit.

## Correzioni a codice esistente

- `CStreamServer::setSynchronizationAddress()` scriveva un solo byte nel campo a 16 bit
  (indirizzo errato in rete): ora usa `PackBUint16` in network byte order.
- Il pacchetto di sync era lungo 48 byte invece di **49** (il campo Reserved finale è di 2 byte):
  corretto `E131_UNIVERSE_SYNCHRONIZATION_SIZE_MIN`.

## File modificati

| File | Modifica |
|---|---|
| `src/sacn/sacnsender.h/.cpp` | campo `sync_address` per universo, gruppi di sync nel `TickLoop`, `sendSynchronization()` con un flusso per (indirizzo, destinazione) e CID dedicato `m_syncCid`, fix byte order |
| `src/sacn/sacnsynchronization.h/.cpp` | `sACNSynchronizationTX` accetta socket e destinazione esterni; lunghezza 49 byte |
| `src/sacn/sacnsender.h/.cpp` (Force Sync) | `setForceSync()`, `setForceSynchronization()`, `applyForceSync()`, campo `force_sync` |
| `src/ui/transmitwindow.*`, `ui/transmitwindow.ui` | riga Sync Universe, pulsante Force Sync |
| `src/ui/multiuniverse.*`, `ui/multiuniverse.ui` | colonna Sync Universe |

## Predisposizione per la funzione B ("sync master")

`sACNSynchronizationTX` può essere usata da sola: costruita senza socket crea il proprio socket
sull'interfaccia di rete scelta, e `sendSynchronizationPacket()` invia un pacchetto con sequenza
incrementale. Una futura modalità che invia solo pacchetti di sync per altre sorgenti richiede
quindi soltanto:

```cpp
auto * master = new sACNSynchronizationTX(CID::CreateCid(), syncUniverse); // socket proprio
connect(timer, &QTimer::timeout, master, &sACNSynchronizationTX::sendSynchronizationPacket);
timer->start(1000 / rateHz);
```

più una piccola finestra con universo e frequenza. Nessuna modifica al motore di trasmissione dati.

## Test eseguito

Test a livello di pacchetti (unicast su loopback, 3 sorgenti: universi 1 e 2 con sync su 100,
universo 3 senza sync):
- dopo ogni gruppo di dati degli universi 1-2 arrivano i pacchetti di sync (49 byte, indirizzo 100);
- variando solo l'universo 1, anche l'universo 2 viene inviato nello stesso ciclo;
- l'universo 3 riporta Synchronization Address 0 e non genera sync;
- portando a `Off` il sync dell'universo 2 durante la trasmissione, l'indirizzo nei suoi pacchetti
  passa a 0 e il suo sync non viene più inviato.

### Test del flusso unico per indirizzo

Test a livello di pacchetti (unicast su loopback, 6 sorgenti con CID diversi come nella Multi
Universe: universi 1, 2, 3 con sync 100 verso 127.0.0.1, universo 4 con sync 200, universo 5 senza
sync, universo 6 con sync 100 verso 127.0.0.2):
- un solo pacchetto di sync 100 per ciclo verso 127.0.0.1, dopo i dati di tutti gli universi 1-3
  (con il codice precedente erano 3 per ciclo);
- flussi separati per sync 100 verso 127.0.0.2 e per sync 200;
- tutti i pacchetti di sync hanno lo stesso CID, diverso da quelli delle sorgenti dati;
- sequenza incrementata di uno per pacchetto;
- fermando la sorgente dell'universo 1 il flusso continua con lo stesso CID e la sequenza
  non riparte;
- portando a `Off` il sync dell'universo 2, il suo indirizzo passa a 0 e il sync 100 continua per
  l'universo 3.

### Test del bit Force Synchronization

Stesso test a livello di pacchetti, con due sorgenti aggiuntive (universo 10 con sync 300, Force
Sync attivo dall'avvio e priorità per indirizzo; universo 11 con Force Sync ma senza sync):
- universo 10: bit 0x20 impostato nei pacchetti DMX, assente nei pacchetti 0xDD;
- universo 11: bit assente (nessun sync);
- Force Sync spento e riacceso durante la trasmissione: il bit segue il comando;
- sync portato a `Off` durante la trasmissione: bit azzerato; sync riattivato: bit di nuovo presente;
- un'altra sorgente sincronizzata senza Force Sync: bit assente.

Interfaccia verificata con il solo form generato da `uic` (non l'applicazione completa): pulsante
disattivato con sync spento, attivo con sync abilitato, utilizzabile durante la trasmissione.

## Limitazioni note

- Le impostazioni di sync non vengono salvate tra un avvio e l'altro.
- Il Force Sync è disponibile solo nella finestra Transmit, non nella Multi Universe.
- Il CID del sync cambia a ogni avvio di sACNView e non è visibile nell'interfaccia.

## Storico dei commit (branch `feature/universe-sync-tx`)

| Commit | Contenuto |
|---|---|
| `6d6f5d7` | Prima implementazione della trasmissione sincronizzata: Synchronization Address nei pacchetti dati, gruppi di sync nel `TickLoop`, pacchetti di sync dopo i dati, UI Transmit e Multi Universe, correzioni byte order e lunghezza 49 byte. Un flusso di sync per ogni sorgente (CID). |
| `57bcff5` | CI: nome dell'installer Windows allineato a `install.nsi`. |
| `f22ac21` | Un solo flusso di sync per (sync address, destinazione), condiviso da tutte le sorgenti, con CID dedicato `m_syncCid`. |
| *(questo commit)* | Bit Force_Synchronization: pulsante **Force Sync** nella finestra Transmit, modificabile anche durante la trasmissione; il bit esce solo per universi sincronizzati. |
