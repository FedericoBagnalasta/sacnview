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
3. **Passo 2**: solo dopo tutti i dati del ciclo viene inviato un pacchetto di sync per ogni
   coppia (sorgente CID, indirizzo di sync) che ha trasmesso.

Dettagli:
- I pacchetti di sync escono dallo **stesso socket dei dati**, così l'ordine "dati poi sync"
  sul cavo è garantito.
- Destinazione: in multicast il gruppo dell'universo di sync; se la sorgente trasmette in
  unicast, il sync va allo stesso indirizzo unicast.
- Ogni sorgente (CID) invia i propri pacchetti di sync: è il comportamento sicuro anche per
  ricevitori che accettano il sync solo dalla sorgente dei dati. Con più sorgenti sullo stesso
  indirizzo si hanno più pacchetti di sync per ciclo, tutti dopo i dati.
- I sender di sync (`sACNSynchronizationTX`) restano attivi per tutta la vita del server, così
  il numero di sequenza non riparte da zero.
- Il Synchronization Address è inserito solo nei pacchetti E1.31 ratified con start code 0x00;
  per Draft, Pathway Secure e i dati di priorità per indirizzo (0xDD) resta 0, altrimenti i
  ricevitori attenderebbero un sync che non arriva.

## Correzioni a codice esistente

- `CStreamServer::setSynchronizationAddress()` scriveva un solo byte nel campo a 16 bit
  (indirizzo errato in rete): ora usa `PackBUint16` in network byte order.
- Il pacchetto di sync era lungo 48 byte invece di **49** (il campo Reserved finale è di 2 byte):
  corretto `E131_UNIVERSE_SYNCHRONIZATION_SIZE_MIN`.

## File modificati

| File | Modifica |
|---|---|
| `src/sacn/sacnsender.h/.cpp` | campo `sync_address` per universo, gruppi di sync nel `TickLoop`, `sendSynchronization()`, fix byte order |
| `src/sacn/sacnsynchronization.h/.cpp` | `sACNSynchronizationTX` accetta socket e destinazione esterni; lunghezza 49 byte |
| `src/ui/transmitwindow.*`, `ui/transmitwindow.ui` | riga Sync Universe |
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

## Limitazioni note

- Le impostazioni di sync non vengono salvate tra un avvio e l'altro.
- Il bit Force_Synchronization non è esposto nell'interfaccia.
