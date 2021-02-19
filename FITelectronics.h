#ifndef FITELECTRONICS_H
#define FITELECTRONICS_H

#include "IPbusInterface.h"

const quint32 datasize = 76800;
enum histType {hADC0 = 0, hADC1 = 1, hTime = 2};

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

    struct TypeStats {
        quint32 sum;
        double mean, RMS;
    } statsCh[12][3];

    FITelectronics(): IPbusTarget(50011) {
        connect(this, &IPbusTarget::IPbusStatusOK, this, &FITelectronics::checkPMlinks);
    }

    quint32 readHistograms() {
        updateTimer->stop();
        curAddress = 0;
        quint32 wordsRead = 0;
        addTransaction(RMWbits, (curPM+1)*0x200 + 0x7E, masks(0xFFFF7FFF, 0));
        addTransaction(write, (curPM+1)*0x200 + 0xF5, &curAddress);
        addTransaction(nonIncrementingRead, (curPM+1)*0x200 + 0xF6, (quint32 *)&data, 180);
        addTransaction(nonIncrementingRead, (curPM+1)*0x200 + 0xF6, (quint32 *)&data + 180, 180);
        quint8 qd=6;
        if (transceive()) wordsRead = 360 + readFast((curPM+1)*0x200 + 0xF6, (quint32 *)&data + 360, datasize - 360, qd);
        if (histStatus.histOn) setBit(15, (curPM+1)*0x200 + 0x7E, false);
        updateTimer->start(updatePeriod_ms);
        return wordsRead;
    }

    void calcStats() {
        quint8 iCh;
        qint16 iBin;
        quint16 v;
        quint32 s;
        double m, r;
        for (iCh=0; iCh<12; ++iCh) {
            s = 0; m = r = 0;
            for (iBin=-2048; iBin < 2048; ++iBin) { v = data.Ch[iCh].time[iBin & 0xFFF]; s += v; m += v*iBin; r += v*iBin*iBin; }
            statsCh[iCh][hTime].sum = s;
            statsCh[iCh][hTime].mean = m /= s;
            statsCh[iCh][hTime].RMS = sqrt(r/s - m*m);

            s = 0; m = r = 0;
            for (iBin= -256; iBin <    0; ++iBin) { v = data.Ch[iCh].nADC0[-iBin - 1];   s += v; m += v*iBin; r += v*iBin*iBin; }
            for (iBin=    0; iBin < 4096; ++iBin) { v = data.Ch[iCh].pADC0[iBin];        s += v; m += v*iBin; r += v*iBin*iBin; }
            statsCh[iCh][hADC0].sum = s;
            statsCh[iCh][hADC0].mean = m /= s;
            statsCh[iCh][hADC0].RMS = sqrt(r/s - m*m);

            s = 0; m = r = 0;
            for (iBin= -256; iBin <    0; ++iBin) { v = data.Ch[iCh].nADC1[-iBin - 1];   s += v; m += v*iBin; r += v*iBin*iBin; }
            for (iBin=    0; iBin < 4096; ++iBin) { v = data.Ch[iCh].pADC1[iBin];        s += v; m += v*iBin; r += v*iBin*iBin; }
            statsCh[iCh][hADC1].sum = s;
            statsCh[iCh][hADC1].mean = m /= s;
            statsCh[iCh][hADC1].RMS = sqrt(r/s - m*m);
        }
    }

signals:
    void linksStatusReady(quint32);
    void statusReady();

public slots:
//    void testSpeed() {
//        const quint32 blockSize = 364*100;
//        quint32 tdata[blockSize], stats[16] = {0}, n = 1000;
//        QDateTime start, finish;
//        for (quint8 i=1; i<=15; ++i) {
//            quint8 qd = i;
//            start = QDateTime::currentDateTime();
//            for (quint16 j=0; j<n; ++j) stats[i] += (blockSize - readFast(0x7, tdata, blockSize, qd))/364;
//            finish = QDateTime::currentDateTime();
//            qDebug() << QString::asprintf("QD=%2d (%2d used): %7.3f s (%5.1f Mbit/s), %3d ppm lost", i, qd, start.msecsTo(finish)/1000., blockSize*0.032*n/start.msecsTo(finish), stats[i]*10);
//        }
//    }

    void checkPMlinks() {
        quint32 maskSPI, maskTrgA, maskTrgC;// = readRegister(0x1E);
        addTransaction(read, 0x1E, &maskSPI);
        addTransaction(read, 0x1A, &maskTrgA);
        addTransaction(read, 0x3A, &maskTrgC);
        if (!transceive()) return;
        for (quint8 i=0; i<20; ++i) {
            if (!(maskSPI >> i & 1)) setBit(i, 0x1E, false);
            if (fabs(readRegister((i+1)*0x200 + 0xFD) * 3. / 65536 - 1.) < 0.2) {
                maskSPI |= 1 << i;
                if (i > 9) maskTrgC |= 1 << (i - 10);
                else       maskTrgA |= 1 << i;
            } else clearBit(i, 0x1E, false);
		}
        addWordToWrite(0x1A, maskTrgA);
        addWordToWrite(0x3A, maskTrgC);
        if (transceive() && _BitScanForward(&curPM, maskSPI)) emit linksStatusReady(maskSPI);
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
