# Cambio della modalità di priorità durante la trasmissione

## Cosa è stato aggiunto

Nella finestra **Transmit** si può passare da **Per-Source** a **Per-Address** (priorità per
indirizzo, pacchetti con start code 0xDD) e viceversa **mentre la sorgente trasmette**, senza
fermarla. Durante la trasmissione restano modificabili anche:
- il pulsante delle priorità per indirizzo (`000-000...`): i nuovi valori escono subito;
- il valore **Priority** della sorgente: il nuovo valore esce subito nei pacchetti DMX.

Tutte le altre opzioni della sorgente restano bloccate durante la trasmissione, come prima.

## Come si usa

- Premi **Start** in una modalità qualsiasi.
- Cambia **Priority Mode** quando vuoi:
  - **Per-Source → Per-Address**: il flusso 0xDD parte subito con le priorità per indirizzo
    impostate.
  - **Per-Address → Per-Source**: i ricevitori passano subito alla priorità della sorgente; il
    flusso 0xDD continua per circa 3 secondi con valori tutti uguali alla priorità della sorgente,
    poi si ferma.
- Se torni a Per-Address entro quei 3 secondi, il flusso 0xDD continua senza interruzioni con le
  priorità per indirizzo.

## Come funziona

### Per-Source → Per-Address

`sACNSentUniverse::setPriorityMode(PER_ADDRESS)` crea al volo l'universo 0xDD in `CStreamServer`
(stesso CID e stesso universo dei dati DMX) con `createPriorityUniverse()`. Il buffer viene
riempito con le priorità richieste **prima** che parta il primo pacchetto.

I ricevitori adottano le priorità per indirizzo appena vedono arrivare gli 0xDD da una sorgente
che già conoscono (il ricevitore di sACNView lo fa al primo pacchetto 0xDD).

### Per-Address → Per-Source: il punto delicato

Chiudere il flusso 0xDD come avviene allo Stop non va bene a trasmissione in corso:
- `DestroyUniverse()` invia pacchetti 0xDD con il bit **Stream_Terminated**. Secondo E1.31 quel
  bit fa entrare il ricevitore in perdita dati per **l'intera sorgente su quell'universo**, non solo
  per il flusso 0xDD (il ricevitore di sACNView, in `sacnlistener.cpp`, si comporta così). Con i
  dati DMX ancora in corso i ricevitori potrebbero considerare persa la sorgente per un attimo.
- Smettere di inviare gli 0xDD senza avvisare è corretto, ma i ricevitori continuerebbero a usare
  le vecchie priorità per indirizzo per circa 2,5 s (timeout di perdita dati) prima di passare alla
  priorità della sorgente.

Soluzione implementata ("handover"):
1. Al passaggio, il buffer 0xDD viene riempito con un valore unico uguale alla priorità della
   sorgente. Per un ricevitore è equivalente alla modalità Per-Source, quindi il cambio è
   **immediato**. Il valore minimo è 1, perché 0 in 0xDD significa "non usare questa sorgente per
   questo indirizzo".
2. I valori "piatti" restano per `sACNSentUniverse::PAP_HANDOVER_MS` = 3000 ms
   (`E131_NETWORK_DATA_LOSS_TIMEOUT` + 500 ms). Gli 0xDD escono a circa 1 Hz, quindi in questo
   intervallo ne partono 2-3.
3. Allo scadere del timer (`endPapHandover()`), l'universo 0xDD viene rimosso con il nuovo
   `CStreamServer::RemoveUniverse()`, che lo elimina **senza** Stream_Terminated. I ricevitori
   tengono gli ultimi valori piatti fino al loro timeout, poi usano la priorità della sorgente,
   che ha lo stesso valore: nessuna variazione visibile.

Durante l'handover:
- se cambia la priorità della sorgente, anche i valori piatti si aggiornano;
- nuove priorità per indirizzo vengono memorizzate ma non inviate;
- tornando a Per-Address il timer si ferma e il flusso 0xDD riprende con le priorità per indirizzo.

### Altri dettagli del motore (`src/sacn/sacnsender.h/.cpp`)

- `sACNSentUniverse` tiene una copia delle priorità per indirizzo (`m_papValues`), aggiornata da
  ogni `setPerChannelPriorities()` anche quando il flusso 0xDD non è attivo, così un flusso creato
  più tardi parte con i valori giusti.
- `m_priorityActive` dice se l'universo 0xDD esiste davvero; `stopSending()` lo chiude in base a
  questo flag e non alla modalità corrente (che durante la trasmissione può essere cambiata).
- `setPerSourcePriority()` marca l'universo DMX come dirty (il nuovo valore esce subito invece che
  al keep-alive successivo) e aggiorna anche l'intestazione dell'universo 0xDD.
- Il numero di sequenza è condiviso tra 0x00 e 0xDD per (CID, universo), quindi resta continuo
  attraverso tutti i passaggi.
- `m_preview` ricorda l'opzione Blind della trasmissione corrente, usata per un flusso 0xDD creato
  durante la trasmissione.
- `PAP_HANDOVER_MS` è dichiarata nella sezione `public` (non in `public slots`, dove `moc` non
  accetta dichiarazioni che non siano slot).

### Correzioni a codice esistente

- **`stopSending()` su una sorgente già ferma terminava un'altra sorgente.** Dopo lo stop
  `m_handle` vale 0, e gli handle di `CStreamServer` vengono riusati. Chiamare di nuovo
  `stopSending()` (lo fa per esempio il distruttore, quando si chiude una finestra Transmit ferma)
  inviava Stream_Terminated sull'universo che nel frattempo aveva preso l'handle 0, cioè su
  un'altra sorgente attiva. Ora `stopSending()` non fa nulla se la sorgente non sta trasmettendo.
- **Primo pacchetto 0xDD con tutte le priorità a 0.** L'universo 0xDD veniva creato e marcato da
  inviare prima che la finestra impostasse le priorità; poteva quindi partire un pacchetto con
  tutti 0 ("non usare questa sorgente"). Ora il buffer viene riempito alla creazione e la finestra
  passa le priorità prima di `startSending()`.
- `setPerChannelPriorities()` copiava sempre 512 byte nel buffer 0xDD, che è lungo quanto lo Slot
  Count: ora copia `m_slotCount` byte.

### Interfaccia (`src/ui/transmitwindow.*`)

- `setUniverseOptsEnabled()`: invece di disattivare tutto il riquadro `frSourceOpts` (che
  disattiverebbe anche i controlli delle priorità), disattiva uno per uno i suoi figli diretti,
  tranne `label_4`/`cbPriorityMode` (Priority Mode), `btnEditPerChan` e `label_3`/`sbPriority`
  (Priority). Riattivandoli, `updateSyncControls()` ripristina le condizioni proprie dei controlli
  di sync (prima le conservava il riquadro).
- `updateSyncControls()` non riattiva i controlli di sync durante la trasmissione.
- `on_cbPriorityMode_currentIndexChanged()`: durante la trasmissione invia prima le priorità
  per indirizzo e poi chiama `setPriorityMode()`.
- Nuovo `applyPerAddressPriorities()`: invia le priorità per indirizzo tenendo conto delle schede
  Channel Check ed Effects, che con le loro opzioni PAP inviano solo una parte delle priorità.
  Usato al cambio di modalità e alla chiusura della finestra delle priorità per indirizzo.
- Nuovo slot `on_sbPriority_valueChanged()`: invia la priorità della sorgente durante la
  trasmissione.

## Test eseguito

Test a livello di pacchetti (unicast su loopback, universo 20, priorità 100), 5 esecuzioni
consecutive superate:
- Per-Source: nessun pacchetto 0xDD.
- Per-Source → Per-Address: 0xDD subito, ogni pacchetto con le priorità richieste (nessun pacchetto
  a 0), nessuno Stream_Terminated.
- Nuove priorità per indirizzo durante la trasmissione: inviate subito.
- Per-Address → Per-Source: 0xDD tutti a 100; cambiando la priorità a 120 durante l'handover i
  valori piatti diventano 120 e l'intestazione DMX riporta 120; ultimo 0xDD tra 2 e 3 s dopo il
  cambio, poi nessuno; nessuno Stream_Terminated su 0x00 o 0xDD; flusso DMX continuo.
- Per-Address → Per-Source → Per-Address entro l'handover: 0xDD continuo con le priorità per
  indirizzo, nessuno Stream_Terminated.
- Numero di sequenza continuo (+1 per pacchetto, 0x00 e 0xDD insieme) attraverso tutti i passaggi.
- Stop in Per-Address: Stream_Terminated su 0x00 e su 0xDD (3 pacchetti ciascuno).
- Stop durante l'handover e nuova sorgente che riusa gli handle: il timer non tocca la nuova
  sorgente.
- `stopSending()` su una sorgente già ferma mentre un'altra occupa l'handle 0: l'altra sorgente
  non riceve Stream_Terminated. Con il codice precedente questo controllo fallisce.

Interfaccia verificata con il solo form generato da `uic` (non l'applicazione completa, che qui non
si compila: serve un Qt più recente della 6.4 disponibile).

## Limitazioni note

- Disponibile solo nella finestra Transmit (la Multi Universe non ha la modalità Per-Address).
- Gli altri parametri della sorgente (universo, nome, protocollo, sync, unicast...) si cambiano
  ancora solo da ferma.
