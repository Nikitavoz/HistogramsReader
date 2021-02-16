#ifndef FITELECTRONICS_H
#define FITELECTRONICS_H

#include "IPbusInterface.h"

const quint32 datasize = 76800;

class FITelectronics: public IPbusTarget {
    Q_OBJECT
public:
    unsigned long curPM;
    quint32 curAddress = 0;
    struct {
        quint32
            BCID     :12,
            filterOn : 1,
                     : 2,
            histOn   : 1,
                     :16;
    } histStatus;
    struct {
        quint32
            PLLlocked : 4,
                      : 2,
            syncError : 3,
            resetting : 1,
                      :12;
    } boardStatus;
    struct HDMIlinkStatus {
        quint32
            line0delay          : 5,
            line0signalLost     : 1,
            line0signalStable   : 1,
                                : 1,
            line1delay          : 5,
            line1signalLost     : 1,
            line1signalStable	: 1,
                                : 1,
            line2delay          : 5,
            line2signalLost     : 1,
            line2signalStable	: 1,
            bitPositionsOK		: 1,
            line3delay          : 5,
            line3signalLost     : 1,
            line3signalStable	: 1,
            linkOK              : 1;
    } triggerLinkStatus;
    struct TypeHistData {
        struct TypeChannel {
            quint16
                pADC0[4096],
                pADC1[4096],
                time[4096],
                nADC0[256],
                nADC1[256];
        } Ch[12];
    } data;

    FITelectronics(): IPbusTarget(50011) {
        connect(this, &IPbusTarget::IPbusStatusOK, this, &FITelectronics::checkPMlinks);
    }

    quint32 readHistograms() {
        updateTimer->stop();
        curAddress = 0;
        addTransaction(RMWbits, (curPM+1)*0x200 + 0x7E, masks(0xFFFF7FFF, 0));
        addTransaction(write, (curPM+1)*0x200 + 0xF5, &curAddress);
        addTransaction(nonIncrementingRead, (curPM+1)*0x200 + 0xF6, (quint32 *)&data + curAddress, 180);
        curAddress += 180;
        addTransaction(nonIncrementingRead, (curPM+1)*0x200 + 0xF6, (quint32 *)&data + curAddress, 180);
        curAddress += 180;
        for (quint8 i=0; transceive() && i<210; ++i) {
            addTransaction(nonIncrementingRead, (curPM+1)*0x200 + 0xF6, (quint32 *)&data + curAddress, 182);
            curAddress += 182;
            addTransaction(nonIncrementingRead, (curPM+1)*0x200 + 0xF6, (quint32 *)&data + curAddress, 182);
            curAddress += 182;
        }
        if (histStatus.histOn) setBit(15, (curPM+1)*0x200 + 0x7E, false);
        updateTimer->start(updatePeriod_ms);
        return curAddress;
    }

signals:
    void linksStatusReady(quint32);
    void statusReady();

public slots:
    void checkPMlinks() {
        quint32 mask = readRegister(0x1E);
        for (quint8 i=0; i<20; ++i) {
            if (!(mask >> i & 1)) setBit(i, 0x1E, false);
            if (fabs(readRegister((i+1)*0x200 + 0xFD) * 3. / 65536 - 1.) > 0.2) {
                mask &= ~(1 << i);
                clearBit(i, 0x1E, false);
            }
		}
        writeRegister(mask & 0x3FF, 0x1A, false);
        writeRegister(mask >> 10, 0x3A, false);
        if (_BitScanForward(&curPM, mask)) emit linksStatusReady(mask);
    }

    void sync() { //read actual values
        addTransaction(read, (curPM + 1)*0x200 + 0xF5, &curAddress);
        addTransaction(read, (curPM + 1)*0x200 + 0x7E, (quint32 *)&histStatus);
        addTransaction(read, (curPM + 1)*0x200 + 0x7F, (quint32 *)&boardStatus);
        addTransaction(read, (curPM > 9 ? 0x30 : 0x10) + curPM % 10, (quint32 *)&triggerLinkStatus);
        if (transceive()) emit statusReady();
    }

    void reset() { setBit(9, (curPM+1)*0x200 + 0x7F); }

    void switchHist(bool on) { if (on) setBit(15, (curPM+1)*0x200 + 0x7E); else clearBit(15, (curPM+1)*0x200 + 0x7E); }
    void switchFilter(bool on) { if (on) setBit(12, (curPM+1)*0x200 + 0x7E); else clearBit(12, (curPM+1)*0x200 + 0x7E); }

    void setBCID(int BC) { if (BC >= 0 && BC < 0xDEC) writeNbits(BC, (curPM+1)*0x200 + 0x7E, 12); }
};

#endif // FITELECTRONICS_H
