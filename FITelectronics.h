#ifndef FITELECTRONICS_H
#define FITELECTRONICS_H

#include "IPbusInterface.h"
enum TypeOfHistogram             { hTrig = 0, hTime = 1, hAmpl = 2, hADC0 = 3,  hADC1 = 4 };
constexpr quint16 dataSize     []{     0xDEC,      2048,    2*2048,      2048,       2048 };
constexpr quint16 dataOffset   []{         0,    2*2048,         0,         0,       2048 };
constexpr quint16 dataSizeNeg  []{         0,         0,     2*128,       128,        128 };
constexpr quint16 dataOffsetNeg[]{         0,         0,    3*2048,    3*2048, 3*2048+128 };
constexpr  qint16 minBin       []{         0,     -2048,      -256,      -256,       -256 };
constexpr  qint16 maxBin       []{     0xDEB,      2047,      4095,      4095,       4095 };
const quint32 histDataSizeCh = dataSize[hAmpl]+dataSize[hTime]+dataSizeNeg[hAmpl]/*6400*/, histDataSizePM = 12*histDataSizeCh/*76800*/, regBlockSizeCh = 0x2000, regBlockSizeTr = 0x1000;
const double TDCunit_ns = 1e3/40.0789639/30/64;
struct TypeStats {
    quint64 sum;
    quint32 max;
    double mean, RMS;
};
struct TypeResultSize { size_t read, expected; };

class FITelectronics: public IPbusTarget {
    Q_OBJECT
public:
    struct TypeTCMmode { quint32
        ADD_C_DELAY     : 1, //┐
        C_SC_TRG_MODE   : 2, //│
        EXTENDED_READOUT: 1, //│
        selectableHist  : 4, //│0E
        SC_EVAL_MODE    : 1, //│
        FV0_MODE        : 1, //│
                        :22; //┘
    } TCMmode;
    struct TypeTCMstatus { quint32
        PLLlockC        : 1, //┐
        PLLlockA        : 1, //│
        systemRestarted : 1, //│
        externalClock   : 1, //│
        GBTRxReady      : 1, //│
        GBTRxError      : 1, //│
        GBTRxPhaseError : 1, //│0F
        BCIDsyncLost    : 1, //│
        droppingHits    : 1, //│
        resetCounters   : 1, //│
        forceLocalClock : 1, //│
        resetSystem     : 1, //│
        PMstatusChanged :20; //┘
        TypeTCMstatus &operator= (quint32 v) { memcpy(this, &v, 4); return *this; }
    } TCMstatus;
    struct TypeBoardID { quint32
        boardType       : 2, //┐
                        : 6, //│
        SERIAL_NUM      : 8, //│07
                        :16; //┘
    } TCM;
    quint32 maskSPI = 0;
    quint8 iBd; //index of selected board, 0 for TCM, 1 for PMA0, 2 for PMA1, 11 for PMC0...
    struct {
        quint32
            BCid     :12,
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
    } PMstatus;
    struct TRGsyncStatus {
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

    struct TypeChannelHistogramData {
        quint16
            pADC0[2 * dataSize   [hADC0]] = {0}, //4096 bins
            pADC1[2 * dataSize   [hADC1]] = {0}, //4096 bins
            time [2 * dataSize   [hTime]] = {0}, //4096 bins
            nADC0[2 * dataSizeNeg[hADC0]] = {0}, // 256 bins
            nADC1[2 * dataSizeNeg[hADC1]] = {0}; // 256 bins
        quint32 binValueADC0(qint16 bin) { return bin < 0 ? nADC0[-bin - 1]                   : pADC0[bin]             ; }
        quint32 binValueADC1(qint16 bin) { return bin < 0 ?                   nADC1[-bin - 1] :              pADC1[bin]; }
        quint32 binValueAmpl(qint16 bin) { return bin < 0 ? nADC0[-bin - 1] + nADC1[-bin - 1] : pADC0[bin] + pADC1[bin]; }
        quint32 binValueTime(qint16 bin) { return time[bin & 0xFFF]; }
        void setBinValueADC0(qint16 bin, quint32 v) { (bin < 0 ? nADC0[-bin - 1] : pADC0[bin]) = v; }
        void setBinValueADC1(qint16 bin, quint32 v) { (bin < 0 ? nADC1[-bin - 1] : pADC1[bin]) = v; }
        void setBinValueTime(qint16 bin, quint32 v) { time[bin & 0xFFF] = v; }
    } DCh[12];
    struct TypeTriggerHistogramData {
        quint32 BC[regBlockSizeTr] = {0};
    } DTr[3];

    std::function<const quint32(quint8, qint16)> binValue[5] = {
        [&](quint8 iCh, qint16 bin) { return DTr[iCh].          BC[bin]; },
        [&](quint8 iCh, qint16 bin) { return DCh[iCh].binValueTime(bin); },
        [&](quint8 iCh, qint16 bin) { return DCh[iCh].binValueAmpl(bin); },
        [&](quint8 iCh, qint16 bin) { return DCh[iCh].binValueADC0(bin); },
        [&](quint8 iCh, qint16 bin) { return DCh[iCh].binValueADC1(bin); }
    };
    std::function<void(quint8, qint16, quint32)> setBinValue[5] = {
        [&](quint8 iCh, qint16 bin, quint32 v) { DTr[iCh].           BC[bin] = v ; },
        [&](quint8 iCh, qint16 bin, quint32 v) { DCh[iCh].setBinValueTime(bin, v); },
        {}                                                                          ,
        [&](quint8 iCh, qint16 bin, quint32 v) { DCh[iCh].setBinValueADC0(bin, v); },
        [&](quint8 iCh, qint16 bin, quint32 v) { DCh[iCh].setBinValueADC1(bin, v); }
    };

    TypeStats calcStats(TypeOfHistogram hType, quint8 iCh, double lower, double upper) {
        if (lower > upper) qSwap(lower, upper);
        qint16 lo = lower > minBin[hType] && lower <= maxBin[hType] ? qRound(lower) : minBin[hType];
        qint16 hi = upper < maxBin[hType] && upper >= minBin[hType] ? qRound(upper) : maxBin[hType];
        TypeStats s = {0};
        for (qint16 bin=lo; bin<=hi; ++bin) {
            qint64 v = binValue[hType](iCh, bin);
            if (v == 0) continue;
            s.sum  += v;
            s.mean += v * bin;
            s.RMS  += v * bin * bin;
            if (v > s.max) s.max = v;
        }
        s.mean /= s.sum;
        s.RMS = sqrt(s.RMS/s.sum - pow(s.mean, 2));
        return s;
    }

    FITelectronics(): IPbusTarget(50011) {}

    TypeResultSize readHistograms(TypeOfHistogram hType) {
        size_t wordsRead = 0;
        if (hType == hTrig) {
            quint32 start = TCMmode.selectableHist ? 0x3000 : 0x4000, end = 0x5000 + dataSize[hTrig];//would read extra empty words (for BCid 0xDEE..0xFFF) but it's faster than splitting range
            wordsRead = readFast(start, DTr[TCMmode.selectableHist ? 0 : 1].BC, end - start, false, 6);
            return {wordsRead, end - start};
        }
        clearBit(15, iBd*0x200 + 0x7E, false); //set PM histogramming off
        if (hType == hTime)
            for (quint8 iCh=0; iCh<12; ++iCh) {
                writeRegister                      (iBd*0x200+0xF5,                  iCh*regBlockSizeCh + dataOffset[hTime], false);
                wordsRead += readFast              (iBd*0x200+0xF6, (quint32*)&DCh + iCh*histDataSizeCh + dataOffset[hTime], dataSize[hTime]);
            }
        else if (hType == hADC0 || hType == hADC1) {
            writeRegister                          (iBd*0x200+0xF5,                    0                + dataOffset   [hType], false);
            for (quint8 iCh=0; iCh<12;) {
                wordsRead += readFast              (iBd*0x200+0xF6, (quint32*)&DCh + iCh*histDataSizeCh + dataOffset   [hType], dataSize[hType]);
                addWordToWrite                     (iBd*0x200+0xF5,                  iCh*regBlockSizeCh + dataOffsetNeg[hType]);
                addTransaction(nonIncrementingRead, iBd*0x200+0xF6, (quint32*)&DCh + iCh*histDataSizeCh + dataOffsetNeg[hType], dataSizeNeg[hType]);
                addWordToWrite                     (iBd*0x200+0xF5,                ++iCh*regBlockSizeCh + dataOffset   [hType]);
                if (transceive()) wordsRead += dataSizeNeg[hType]; else break;
            }
        } else if (hType == hAmpl) { //negative bins of i-th channel and positive bins of (i+1)-th channel can be read in one block
            writeRegister                          (iBd*0x200+0xF5,                    0                + dataOffset   [hAmpl], false);
            wordsRead += readFast                  (iBd*0x200+0xF6, (quint32*)&DCh +   0                + dataOffset   [hAmpl], dataSize[hAmpl]);
            for (quint8 iCh=0; iCh<11; ++iCh) {
                writeRegister                      (iBd*0x200+0xF5,                  iCh*regBlockSizeCh + dataOffsetNeg[hAmpl], false);
                wordsRead += readFast              (iBd*0x200+0xF6, (quint32*)&DCh + iCh*histDataSizeCh + dataOffsetNeg[hAmpl], dataSizeNeg[hAmpl]+dataSize[hAmpl]);
            }
            addWordToWrite                         (iBd*0x200+0xF5,                   11*regBlockSizeCh + dataOffsetNeg[hAmpl]);
            addTransaction(nonIncrementingRead,     iBd*0x200+0xF6, (quint32*)&DCh +  11*histDataSizeCh + dataOffsetNeg[hADC0], dataSizeNeg[hADC0]);
            addTransaction(nonIncrementingRead,     iBd*0x200+0xF6, (quint32*)&DCh +  11*histDataSizeCh + dataOffsetNeg[hADC1], dataSizeNeg[hADC1]);
            if (transceive()) wordsRead += dataSizeNeg[hAmpl];
        }
        if (histStatus.histOn) setBit(15, iBd*0x200 + 0x7E, false); //restore histogramming state
        return { wordsRead, 12U * (dataSize[hType] + dataSizeNeg[hType]) };
    }

    quint32 maxBinValue(bool TCM) {
        quint32 max = 0;
        if (TCM) { for(quint8 iTr=0; iTr<3; ++iTr) for(quint32 *i=DTr[iTr].BC , *end=i+   dataSize[hTrig]; i<end; ++i) if (*i > max) max = *i; }
        else     {                                 for(quint16 *i=DCh[0].pADC0, *end=i+(2*histDataSizePM); i<end; ++i) if (*i > max) max = *i; }
        return max;
    }

signals:
    void SPIlinksStatusUpdated(quint32);
    void statusReady();

public slots:
    void checkPMlinks() {
        quint32 maskTrgA, maskTrgC, voltage;
        addTransaction(read, 0x0F, (quint32 *)&TCMstatus);
        addTransaction(read, 0x1E, &maskSPI);
        addTransaction(read, 0x1A, &maskTrgA);
        addTransaction(read, 0x3A, &maskTrgC);
        if (!transceive() || TCMstatus.resetSystem) return;
        for (quint8 iPM=0; iPM<20; ++iPM) {
            if (!(maskSPI >> iPM & 1)) setBit(iPM, 0x1E, false);
            addTransaction(read, 0x200*(iPM+1) + 0xFE, &voltage);
            if (!transceive()) return;
            if (voltage == 0xFFFFFFFF) { //SPI error
                clearBit(iPM, 0x1E, false);
            } else {
                maskSPI |= 1 << iPM;
                if (iPM > 9) maskTrgC |= 1 << (iPM - 10);
                else         maskTrgA |= 1 << iPM;
            }
        }
        addWordToWrite(0x1A, maskTrgA);
        addWordToWrite(0x3A, maskTrgC);
        if (!transceive()) return;
        emit SPIlinksStatusUpdated(maskSPI);
    }

    void sync() { //read actual values
        if (!isOnline) return;
        TCMstatus = readRegister(0x0F);
        if (TCMstatus.resetSystem) emit noResponse("system is resetting");
        else {
            quint32 newMaskSPI = readRegister(0x1E);
            if (newMaskSPI == 0xFFFFFFFF) return;
            else if (newMaskSPI != maskSPI) {
                maskSPI = newMaskSPI;
                emit SPIlinksStatusUpdated(maskSPI);
            }
            if (iBd) { //not TCM
                addTransaction(read, iBd*0x200 + 0x7E, (quint32*)&histStatus);
                addTransaction(read, iBd*0x200 + 0x7F, (quint32*)&PMstatus);
                addTransaction(read, (iBd > 10 ? 0x30 : 0x10) + (iBd-1) % 10, (quint32*)&triggerLinkStatus);
            } else addTransaction(read, 0x0E, (quint32*)&TCMmode);
            if (transceive()) emit statusReady();
        }
    }

    void reset() { setBit(9, iBd ? iBd*0x200 + 0x7F : 0x0F); }
    void switchHistogramming(bool on) { if (on) setBit(15, iBd*0x200 + 0x7E); else clearBit(15, iBd*0x200 + 0x7E); }
    void switchBCfilter     (bool on) { if (on) setBit(12, iBd*0x200 + 0x7E); else clearBit(12, iBd*0x200 + 0x7E); }
    void setBC(int id) { if (id >= 0 && id < 0xDEC) writeNbits(iBd*0x200 + 0x7E, id, 12); }
    void selectTriggerHistogram(quint8 n) { writeNbits(0x0E, n, 4, 4, false); if (n && TCMmode.selectableHist != n) reset(); }

    bool readTimeAlignment(quint32* data) {
        addTransaction(read, iBd*0x200 + 1, data, 12);
        return transceive();
    }
    bool writeTimeAlignment(quint32* data) {
        addTransaction(write, iBd*0x200 + 1, data, 12);
        return transceive();
    }

    bool readADCRegisters(quint32 *data) {
        addTransaction(read, iBd*0x200+0x7F+1,data, 12*4);
        return transceive();
    }

    bool writeADCRegisters(quint32 *data) {
        addTransaction(write, iBd*0x200+0x7F+1,data, 12*4);
        return transceive();
    }

    bool readCounters(quint32 *data) {
        addTransaction(read, iBd*0x200+0xC0, data, 24);
        return transceive();
    }

};

#endif // FITELECTRONICS_H
