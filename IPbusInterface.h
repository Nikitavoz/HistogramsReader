#ifndef IPBUSINTERFACE_H
#define IPBUSINTERFACE_H

#include <QtNetwork>
#include "IPbusHeaders.h"

const quint16 maxPacket = 368; //368 words, limit from ethernet MTU of 1500 bytes

class IPbusTarget: public QObject {
    Q_OBJECT
    quint16 localport;
    QUdpSocket *qsocket = new QUdpSocket(this);
    StatusPacket statusPacket;
    QList<quint32 *> transactionsList; //will contain only pointers to data

protected:
    quint16 requestSize = 0, responseSize = 0; //values are measured in words
    quint32 request[maxPacket], response[maxPacket];
    char *pRequest = reinterpret_cast<char *>(request);
    char *pResponse = reinterpret_cast<char *>(response);
    quint32 dt[2]; //temporary data

public:
    QString IPaddress = "127.0.0.1";

    bool isOnline = false;
    QTimer *updateTimer = new QTimer(this);
    quint16 updatePeriod_ms = 500;

    IPbusTarget(quint16 lport = 0) : localport(lport) {
        connect(updateTimer, &QTimer::timeout, this, [=]() { if (isOnline) sync(); else requestStatus(); });
        connect(this, &IPbusTarget::networkError, this, &IPbusTarget::error);
		connect(this, &IPbusTarget::IPbusError, this, &IPbusTarget::error);
		connect(this, &IPbusTarget::logicError, this, &IPbusTarget::error);
        qsocket->bind(QHostAddress::AnyIPv4, localport);
    }

    ~IPbusTarget() {
        qsocket->disconnectFromHost();
    }

    quint32 readRegister(quint32 address) {
        quint32 data = 0xFFFFFFFF;
        addTransaction(read, address, &data, 1);
        return transceive(true) ? data : 0xFFFFFFFF;
    }

signals:
    void networkError(QString);
    void IPbusError(QString);
    void logicError(QString);
    void noResponse();
    void IPbusStatusOK();
    void successfulRead(quint8 nWords);
    void successfulWrite(quint8 nWords);

protected:
    quint32 *masks(quint32 mask0, quint32 mask1) { //for convinient adding RMWbit transaction
        dt[0] = mask0; //for writing 0's: AND term
        dt[1] = mask1; //for writing 1's: OR term
        return dt;
    }

    void addTransaction(TransactionType type, quint32 address, quint32 *data, quint8 nWords = 1) {
        request[requestSize++] = quint32(TransactionHeader(type, nWords, transactionsList.size()));
        request[requestSize++] = address;
        switch (type) {
            case                read:
            case nonIncrementingRead:
            case   configurationRead:
                responseSize += 1 + nWords;
                break;
            case                write:
            case nonIncrementingWrite:
            case   configurationWrite:
                for (quint8 i=0; i<nWords; ++i) request[requestSize++] = data[i];
                ++responseSize;
                break;
            case RMWbits:
                request[requestSize++] = data[0];
                request[requestSize++] = data[1];
                responseSize += 2;
                break;
            case RMWsum:
                request[requestSize++] = *data; //addend
                responseSize += 2;
        }
        if (requestSize > maxPacket || responseSize > maxPacket) {
            emit IPbusError("packet size exceeded");
            return;
        } else transactionsList.append(data);
    }

    void addWordToWrite(quint32 address, quint32 value) { addTransaction(write, address, &value, 1); }

	bool transceive(bool analyze = true) { //send request, wait for response, receive it and check correctness
		qint32 n = qint32(qsocket->write(pRequest, requestSize * wordSize));
		if (n < 0) {
			emit networkError("Socket write error: " + qsocket->errorString());
			return false;
		} else if (n != requestSize * wordSize) {
			emit networkError("Sending packet failed");
			return false;
        } else if (!qsocket->waitForReadyRead(250)) {
            isOnline = false;
            emit noResponse();
			return false;
		} else {
			n = qint32(qsocket->read(pResponse, responseSize * wordSize));
			qint32 m = qint32(qsocket->bytesAvailable());
			if (m > 0) qsocket->readAll();
			if (n < 0) {
				emit networkError("Socket read error: " + qsocket->errorString());
				return false;
			} else if (n == 0) {
				emit networkError("empty response, no IPbus");
				return false;
			} else if (response[0] != request[0] || n % wordSize > 0) {
                emit networkError(QString::asprintf("incorrect response (%d bytes)", n + m));
				return false;
			} else {
				responseSize = quint16(n / wordSize);
/*//debug print
				printf("request:\n");
				for (quint16 i=0; i<requestSize; ++i)  printf("%08X\n", request[i]);
				printf("        response:\n");
				for (quint16 i=0; i<responseSize; ++i) printf("        %08X\n", response[i]);
				printf("\n"); //*/
                bool result = true;
                if (analyze) result = analyzeResponse();
                resetTransactions();
                return result;
			}
		}
	}

    quint32 readFast(quint32 address, quint32 *data, quint32 dataSize, quint8 &qdmax) {
        quint32 *p = data, *e = data + dataSize, chunkSize = 182, *cp, *rp = data;
        quint8 qd = 0, i, max = 1;
        transactionsList.clear();
        request[0] = quint32(PacketHeader(control, 0));
        request[1] = quint32(TransactionHeader(nonIncrementingRead, chunkSize, 0));
        request[2] = address;
        request[3] = quint32(TransactionHeader(nonIncrementingRead, chunkSize, 1));
        request[4] = address;
        requestSize = 5;
        responseSize = chunkSize * 2 + 3;
        while (p != e) {
            if (qsocket->bytesAvailable() >= responseSize * wordSize) {
                qsocket->read(pResponse, responseSize * wordSize);
                for (i=0, cp=response+2; i<chunkSize; ++i, ++cp) *(p++) = *cp;
                ++cp; //skipping TA1 header
                for (i=0; i<chunkSize; ++i, ++cp) *(p++) = *cp;
                --qd;
            } else if (qd < qdmax && rp != e) {
                qsocket->write(pRequest, requestSize * wordSize);
                ++qd;
                rp += 2 * chunkSize;
                if (qd > max) max = qd;
            } else {
                if (!qsocket->waitForReadyRead(100)) break;
            }
        }
        resetTransactions();
        qdmax = max;
        return p - data;
    }

    bool analyzeResponse() { //check transactions successfulness and copy read data to destination
        for (quint16 j = 1, n = 0; !transactionsList.isEmpty() && j < responseSize; ++j, ++n) {
            TransactionHeader th = response[j];
            if (th.ProtocolVersion != 2 || th.TransactionID != n) {
                emit IPbusError(QString::asprintf("unexpected transaction header: %08X, expected: 2%03X??F0", th, n));
                return false;
            }
            quint8 nWords = th.Words;
            quint32 *data = transactionsList.takeFirst();
            bool ok = true;
            if (nWords > 0) {
                switch (th.TypeID) {
                    case                read:
                    case nonIncrementingRead:
                    case   configurationRead:
                        if (j + nWords >= responseSize) { //response too short to contain nWords values
                            ok = false;
                            nWords = quint8(responseSize - j - 1);
                        }
                        if (data != nullptr)
                            for (quint8 i=0; i<nWords; ++i) data[i] = response[++j];
                        else
                            j += nWords;
                        emit successfulRead(nWords);
                        if (!ok) {
                            emit IPbusError("read transaction truncated");
                            return false;
                        }
                        break;
                    case RMWbits:
                    case RMWsum :
                        if (th.Words != 1) {
                            emit IPbusError("wrong RMW transaction");
                            return false;
                        }
                        ++j;     //skipping received value
                        emit successfulRead(1);
                        /* fall through */ //[[fallthrough]];
                    case                write:
                    case nonIncrementingWrite:
                    case   configurationWrite:
                        emit successfulWrite(nWords);
                        break;
                    default:
                        emit IPbusError("unknown transaction type");
                        return false;
                }
            }
            if (th.InfoCode != 0) {
            emit IPbusError(th.infoCodeString());
            return false;
            }
        }
        return true;
    }

protected slots:
    void resetTransactions() { //return to initial (default) state
        request[0] = quint32(PacketHeader(control, 0));
        requestSize = 1;
        responseSize = 1;
        transactionsList.clear();
    }

	void error() {
		updateTimer->stop();
		resetTransactions();
	}

public slots:
    void reconnect() {
        updateTimer->stop();
        if (qsocket->state() == QAbstractSocket::ConnectedState)
            qsocket->close();
        qsocket->connectToHost(IPaddress, 50001, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);
        qsocket->waitForConnected();
        updateTimer->start(updatePeriod_ms);
        requestStatus();
    }

    void requestStatus() {
        requestSize = sizeof(statusPacket) / wordSize; //16 words
        memcpy(pRequest, &statusPacket, sizeof(statusPacket));
        responseSize = requestSize;
        if (transceive(false)) { //no transactions to process
            isOnline = true;
            updateTimer->stop();
            QTimer::singleShot(QRandomGenerator::global()->bounded(400, 500), this, [=]() {
                updateTimer->start(updatePeriod_ms);
                emit IPbusStatusOK();
                sync();
            });
        }
    }

    virtual void sync() =0;

	void writeRegister(quint32 data, quint32 address, bool syncOnSuccess = true) {
        addTransaction(write, address, &data, 1);
		if (transceive() && syncOnSuccess) sync();
    }

	void setBit(quint8 n, quint32 address, bool syncOnSuccess = true) {
		addTransaction(RMWbits, address, masks(0xFFFFFFFF, 1 << n));
		if (transceive() && syncOnSuccess) sync();
	}

	void clearBit(quint8 n, quint32 address, bool syncOnSuccess = true) {
		addTransaction(RMWbits, address, masks(~(1 << n), 0x00000000));
		if (transceive() && syncOnSuccess) sync();
	}

	void writeNbits(quint32 data, quint32 address, quint8 nbits = 16, quint8 shift = 0, bool syncOnSuccess = true) {
        quint32 mask = (1 << nbits) - 1; //e.g. 0x00000FFF for nbits==12
        addTransaction(RMWbits, address, masks( ~quint32(mask << shift), quint32((data & mask) << shift) ));
		if (transceive() && syncOnSuccess) sync();
    }
};

#endif // IPBUSINTERFACE_H
