#ifndef SACNSYNCHRONIZATION_H
#define SACNSYNCHRONIZATION_H

#include "CID.h"
#include "fpscounter.h"
#include "sacnlistener.h"
#include "sacnsocket.h"
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThread>

// Root layer (38) + sync framing layer (flags/length 2, vector 4, sequence 1, sync address 2, reserved 2) = 49
#define E131_UNIVERSE_SYNCHRONIZATION_SIZE_MIN 49
#define E131_UNIVERSE_SYNCHRONIZATION_SIZE_MAX E131_UNIVERSE_SYNCHRONIZATION_SIZE_MIN

typedef quint16 tsyncAddress;

class sACNSynchronizationTX : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief sACNSynchronizationTX - builds and sends E1.31 Universe Synchronization packets
     * for one source (CID) on one synchronization address.
     * @param cid - CID of the source sending the packets
     * @param syncAddress - synchronization address (universe), 1-63999
     * @param socket - socket to send with. If nullptr a dedicated socket is created and owned
     * by this object (standalone use, e.g. a future sync-only "sync master" source).
     * CStreamServer passes its own socket so sync packets follow the data packets in order.
     * @param destination - destination address. If null the multicast address of syncAddress is used.
     */
    explicit sACNSynchronizationTX(
        CID cid,
        tsyncAddress syncAddress,
        sACNTxSocket * socket = nullptr,
        const QHostAddress & destination = QHostAddress());
    ~sACNSynchronizationTX();

    void sendSynchronizationPacket();
    tsyncAddress syncAddress() const { return m_syncAddress; }

signals:

public slots:

private slots:

private:

    static sACNSynchronizationTX * m_instance;

    void createSynchronizationPacket();

    QMutex m_mutex;
    QByteArray m_pbuf;
    sACNTxSocket * m_sendSock;

    CID m_cid;
    tsyncAddress m_syncAddress;
    QHostAddress m_destination;
    quint8 m_sequence = 0;
};

class sACNSynchronizationRX : public QObject
{
    Q_OBJECT
public:

    explicit sACNSynchronizationRX();
    ~sACNSynchronizationRX();

    static void start();
    static sACNSynchronizationRX * getInstance();

    struct sourceSequence
    {

        sourceSequence(quint8 sequence)
            : lastNum(sequence)
        {}

        quint8 lastNum = 0;
        unsigned int jumps = 0;
        unsigned int seqErr = 0;

        // Check sequence number
        bool checkSeq(quint8 newNum)
        {
            qint8 seqRes = newNum - lastNum;

            if (seqRes != 1) jumps++;

            if ((seqRes <= 0) && (seqRes > -20))
            {
                seqErr++;
                return false; // Drop packet
            }
            else
                lastNum = newNum;

            return true; // Packet ok
        }
    };

    struct sACNSourceDetail
    {
        sACNSourceDetail()
            : sequence(0), fps(new FpsCounter()) {};
        sACNSourceDetail(QHostAddress sender, quint8 sequence)
            : sender(sender), sequence(sequence), fps(new FpsCounter())
        {}
        QHostAddress sender;
        ttimer dataLoss;
        sourceSequence sequence;
        bool operator==(const sACNSourceDetail & detail) const { return (detail.sender == sender); }
        bool operator!=(const sACNSourceDetail & detail) const { return !operator==(detail); }
        FpsCounter * fps;
    };
    typedef QHash<CID, sACNSourceDetail> tCIDDetails;
    typedef QHash<tsyncAddress, tCIDDetails> tSynchronizationSources;

    tCIDDetails getSynchronizationSources(tsyncAddress syncAddress) const
    {
        return m_synchronizationSources.value(syncAddress);
    }
    QList<quint16> getSynchronizationAddresses() const { return m_synchronizationSources.keys(); }

    void processPacket(
        const quint8 * pbuf,
        size_t buflen,
        const QHostAddress & destination,
        const QHostAddress & sender);

signals:
    void newSyncAddress(tsyncAddress syncAddress);
    void newSource(tsyncAddress syncAddress, CID cid);

    void expiredSyncAddress(tsyncAddress syncAddress);
    void expiredSource(tsyncAddress syncAddress, CID cid);

    void synchronize(tsyncAddress syncAddress);

public slots:

private slots:
    void timeoutSyncAddresses();

private:

    static sACNSynchronizationRX * m_instance;

    QMutex m_mutex;
    QThread * m_thread;
    QTimer * m_expiredTimer;

    tSynchronizationSources m_synchronizationSources;
    void addSource(tsyncAddress syncAddress, CID cid, QHostAddress sender, quint8 sequence);
    void removeSource(tsyncAddress syncAddress, CID cid);
};

#endif // SACNSYNCHRONIZATION_H
