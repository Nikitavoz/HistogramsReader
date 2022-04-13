#ifndef CALIBRATIONTASKS_H
#define CALIBRATIONTASKS_H

#include <QObject>
#include <QRunnable>
#include <QElapsedTimer>
#include "switch.h"
#include <QDebug>

#include "FITelectronics.h"

class CalibrationTasks : public QObject, public QRunnable
{
    Q_OBJECT
public:
    CalibrationTasks(QString ipAddress)
      : QObject(nullptr)
      , _abort(false)
      , _activeChannelMap()
      , _mode("TimeAlign")
      , _ADCpMIP(16)
      , _initialSteps(7200)
      , _ipAddress(ipAddress)
      , _iBd(0)
      , _timeDelays() {
        this->setAutoDelete(false);
    }
  void run() {
      _abort = false;
      if (_mode == "TimeAlign") {
          calibrateTimeOffsets();
          emit finished();
      } else if (_mode == "ADC_DELAY") {
          calibrateADC_DELAYs();
          emit finished();
      } else if (_mode == "CFD_ZERO") {
          calibrateCFD_ZERO();
          emit finished();
      }
  }

  void setAbort(bool value = true) {
      _abort = value;
  }
  void setIPAddress(QString ip) {
      _ipAddress = ip;
  }
  void setiBd(int i) { _iBd = i; }
  void setMode(QString mode) { _mode = mode; }
  void setADCpMIP(float value) {_ADCpMIP = value; }
  void setInitialSteps(int value) {_initialSteps = value; }
  void setActiveChannelMap(std::array<bool,12> map) { _activeChannelMap = map; }

signals:
    void finished();
    void updateStatus(int ch, int status);
    void logMessage(int tabIndex, QString msg);
    void addPointADCvSteps(int ch, float x, float y, bool);
    void clearCalPlots(int ch);
    void addHistLine(int ch, int i, int j, QVector<quint32> x);
    void setTitlesADC(int ch, std::array<double,3> adcs);
    void initialStepValue(int ch, int steps);

public slots:
    void onRestoreTimeDelays() {
        auto p = makeAndSetupFEE();
        if (_abort || !p) {
            return;
        }
        auto& FEE = *p;
        FEE.writeTimeAlignment(_timeDelays.data());
    }

protected slots:
    void onIPBusError(QString msg, errorType) {
        _abort = true;
        emit logMessage(0, "IPBus Error: "+msg+"\n");
    }
    void onIPBusNoResponse(QString msg) {
        _abort = true;
        emit logMessage(0, "IPBus unresponsive: "+msg+"\n");
    }

protected:
    // Time Alignment Calibration
    void calibrateTimeOffsets() {
        auto p = makeAndSetupFEE();
        if (_abort || !p) {
            return;
        }
        auto& FEE = *p;
        for (auto ch=0; ch<12; ++ch) {
            if (!_activeChannelMap[ch]) {
                continue;
            }
            emit updateStatus(ch, 2);
        }
        emit logMessage(0, "FEE RESET\n");
        FEE.reset();
        Sleep(10);
        FEE.switchHistogramming(true);
        Sleep(500);
        auto n = FEE.readHistograms(hTime);
        emit logMessage(0, QString::asprintf("Read Histograms(%d)\n\n",n.read));

        emit logMessage(0, "TIME OFFSET CALIBRATION\n\n");
        emit logMessage(0, " PM  CH  meanTime stdTime\n");

        std::array<double,12> meanTime, stdTime;
        std::array<int,12>    timeOK, nEntries;
        computeMeanStd(FEE, 0, meanTime,stdTime,timeOK,nEntries);
        for (auto ch=0; ch<12; ++ch) {
            if (!_activeChannelMap[ch]) {
                continue;
            }
            emit logMessage(0, QString::asprintf(" %s  CH%02d %7.2f %7.2f %s\n",
                                                 FEE.formatPM().c_str(),
                                                 1+ch, meanTime[ch], stdTime[ch],
                                                 timeOK[ch] == 1 ? "OK" : (timeOK[ch] == 0 ? "BAD" : "EMPTY")));
        }

        emit logMessage(0, "\nRegister Update\n\n");
        emit logMessage(0, " PM  CH   OLD -> NEW  STATUS\n");
        // update registers
        quint32 regs[12];
        auto success = FEE.readTimeAlignment(regs);
        for (int ch=0; ch<12; ++ch) {
            _timeDelays[ch] = regs[ch];
            if (!_activeChannelMap[ch]) {
                continue;
            }
            switch (timeOK[ch]) {
                case 1: {
                    quint32 regNew = int(regs[ch]&4095) + std::lround(meanTime[ch]);
                    emit logMessage(0, QString::asprintf(" %s  CH%02d %4d->%4d  %s\n",
                                                         FEE.formatPM().c_str(),
                                                         1+ch,
                                                         (regs[ch]&4095), regNew,
                                                         (regs[ch]&4095) == regNew ? "NO CHANGE": "UPDATED"));

                    regs[ch] = ((regs[ch] >> 12) << 12) | (regNew & 4095);
                    emit updateStatus(ch, 3);
                 break;
                }
                case -1: {
                    emit logMessage(0, QString::asprintf(" %s  CH%02d %4d        NO TIME MEASUREMENT\n",
                                                         FEE.formatPM().c_str(),
                                                         1+ch,
                                                         (regs[ch]&4095)));
                    emit updateStatus(ch, 0);
                    break;
                }
                case 0:
                default: {
                    emit logMessage(0, QString::asprintf(" %s  CH%02d %4d        BAD TIME MEASUREMENT\n",
                                                         FEE.formatPM().c_str(),
                                                         1+ch,
                                                         (regs[ch]&4095)));
                    emit updateStatus(ch, 4);
                }
            }
        }
        success = FEE.writeTimeAlignment(regs);
        emit logMessage(0, success ? "\nOK\n" : "\nFAILED\n");
    }

    // ADC_DELAY Calibration
    void calibrateADC_DELAYs() {
        auto p = makeAndSetupFEE();
        if (_abort || !p) {
            return;
        }
        auto& FEE = *p;
        emit logMessage(0, "FEE RESET\n");
        FEE.reset();
        Sleep(10);
        for (auto ch=0; ch<12; ++ch) {
            if (!_activeChannelMap[ch]) {
                continue;
            }
            emit updateStatus(ch, 2);
        }

        quint32 adcRegs[4*12];
        bool  success = FEE.readADCRegisters(adcRegs);
        quint32 adcRegsOld[4*12];
        std::copy(adcRegs, adcRegs+4*12, adcRegsOld);
        quint32 counters[24];
        quint32 countersOld[24];
        int const m = 20000/200;
        quint32 rates[12][m];
        for (int ch=0; ch<12; ++ch) {
            for (int i=0; i<m; ++i) {
                rates[ch][i] = 0;
            }
        }
        emit logMessage(0, "ADC DELAY CALIBRATION\n\n");
        emit logMessage(0, " Delay | ");
        for (int ch=0; ch<12; ++ch) {
            emit logMessage(0, QString::asprintf(" CH%02d", 1+ch));
        }
        emit logMessage(0,"\n");
        bool atLeastOneNonZeroRate = false;
        for (int i=0; i<m; ++i) {
            if (_abort) {
                emit logMessage(0, "\nAborting... restore old register values\n");
                FEE.writeADCRegisters(adcRegsOld);
            }
            int const ADCRegisters = 200*i;
            for (int ch=0; ch<12; ++ch) {
                if (!_activeChannelMap[ch]) {
                    continue;
                }
                if (_abort) {
                    emit updateStatus(ch, 4);
                } else {
                    adcRegs[4*ch+3] = ADCRegisters;
                }
            }
            if (_abort) {
                return;
            }
            FEE.writeADCRegisters(adcRegs);
            Sleep(10);
            success = FEE.readCounters(countersOld);
            Sleep(100);
            success = FEE.readCounters(counters);
            bool nonZeroRates[12] = {false};
            for (int ch=0; ch<12; ++ch  ) {
                if (!_activeChannelMap[ch]) {
                    continue;
                }
                rates[ch][i] = counters[2*ch+1]-countersOld[2*ch+1];
                atLeastOneNonZeroRate = (rates[ch][i]>0 ? true : atLeastOneNonZeroRate);
                nonZeroRates[ch] = rates[ch][i]>0;
            }
            emit logMessage(0,QString::asprintf("%6d | ", ADCRegisters));
            for (int ch=0; ch<12; ++ch) {
                emit logMessage(0, QString::asprintf("%5d", rates[ch][i]));
            }
            emit logMessage(0, "\n");
        }

        emit logMessage(0, "\n\nREGISTER UPDATE\n\n");
        emit logMessage(0, " PM  CH    old -> new   STATUS\n");

        for (int ch=0; ch<12; ++ch) {
            if (!_activeChannelMap[ch]) {
                continue;
            }
            double sumW  = 0.0;
            double sumWX = 0.0;
            for (int i=0; i<m; ++i) {
                sumW  += rates[ch][i] ;
                sumWX += (i*200) * rates[ch][i];
            }
            if (sumW != 0.0) {
                quint32 const off = std::lround(sumWX/sumW);
                emit logMessage(0, QString::asprintf(" %s  CH%02d %5d->%5d  %s\n",
                                               FEE.formatPM().c_str(),
                                               1+ch,
                                               adcRegsOld[4*ch+3], off,
                                               adcRegsOld[4*ch+3]== off ? "NO CHANGE" : "UPDATE"));
                emit updateStatus(ch, 3);
                adcRegs[4*ch+3] = off;
            } else {
                emit logMessage(0, QString::asprintf(" %s  CH%02d %5d         NO DATA\n",
                                               FEE.formatPM().c_str(),
                                               1+ch,
                                               adcRegsOld[4*ch+3]));
                emit updateStatus(ch, 0);
                // restore old settings
                adcRegs[4*ch+3] = adcRegsOld[4*ch+3];
            }
        }
        success = FEE.writeADCRegisters(adcRegs);
        emit logMessage(0, success ? "\nOK\n" : "\nFAILED\n");
    }

    // CFD_ZERO Calibration
    void calibrateCFD_ZERO() {
        auto p = makeAndSetupFEE();
        if (_abort || !p) {
            return;
        }
        auto& FEE = *p;
        for (int ch=0; ch<12; ++ch) {
            if (!_activeChannelMap[ch]) {
                continue;
            }
            emit updateStatus(ch, 2);
            emit clearCalPlots(ch);
            emit initialStepValue(ch, _initialSteps);
            setAttenuator(FEE, _initialSteps, true);
            std::array<quint32,3> attenSteps;
            emit logMessage(0, QString::asprintf("\n======================\nCOARSE SCAN for CH=%02d\n======================\n\n", ch+1));
            auto result = calCFDThreshold(FEE, ch, attenSteps, _ADCpMIP, true);
            if (_abort) {
                setAttenuator(FEE, _initialSteps, true);
                emit updateStatus(ch, 4);
                return;
            }
            int newCFDValue = result.first;
            if (newCFDValue < -500 || result.second == false) {
                setAttenuator(FEE, _initialSteps, true);
                emit logMessage(0, "COARSE SCAN has failed\n");
                emit updateStatus(ch, 4);
                return;
            }
            emit logMessage(0, QString::asprintf("\n========================================\nFINE  SCAN for CH=%02d centered at %5d\n========================================\n\n",
                                                 ch+1, newCFDValue));
            result = calCFDThreshold(FEE, ch, attenSteps, _ADCpMIP, false, newCFDValue);
            if (_abort) {
                setAttenuator(FEE, _initialSteps, true);
                emit updateStatus(ch, 4);
                return;
            }
            newCFDValue = result.first;
            if (newCFDValue < -500 || result.second == false) {
                setAttenuator(FEE, attenSteps[0], true);
                emit logMessage(0, QString::asprintf("FINE  SCAN for CH=%02d has failed\n",  ch+1));
                emit updateStatus( ch, 4);
                return;
            }
            quint32 adcRegs[4*12];
            bool  success = FEE.readADCRegisters(adcRegs);
            emit logMessage(0, success ? "\nOK\n" : "\nFAILED\n");

            int oldCFDValue = adcRegs[4* ch+1] - (1<<16) * (adcRegs[4* ch+1] > (1<<15)); // unsigned to signed conversion
            emit logMessage(0, QString::asprintf("new CFD_ZERO setting for CH=%02d: %5d (was: %5d)\n",
                                                 ch+1, newCFDValue,
                                                 oldCFDValue));
            adcRegs[4* ch+1] = newCFDValue;
            success = FEE.writeADCRegisters(adcRegs);
            emit logMessage(0, success ? "\nOK\n" : "\nFAILED\n");
            setAttenuator(FEE, attenSteps[0], true);
            emit updateStatus(ch, 3);
        }
    }
private:
    void computeMeanStd(FITelectronics&,
                        int typeHist,
                        std::array<double,12>& meanTime,
                        std::array<double,12>& stdTime,
                        std::array<int,12>& timeOK,
                        std::array<int,12>& nEntries);
    bool setAttenuator(FITelectronics& FEE, quint32 steps, bool verbose = false);
    bool adjustAttenuatorADC(FITelectronics& FEE, int ch, float refADCValue, quint32& attenSteps, bool isFirst);
    std::pair<int,bool> calCFDThreshold(FITelectronics& FEE, int ch0, std::array<quint32,3>& attenSteps, float adcPerMIP, bool coarse=true, int startCFDOffset=0);
    std::unique_ptr<FITelectronics> makeAndSetupFEE();

    bool _abort;
    std::array<bool, 12> _activeChannelMap;
    QString _mode;
    float _ADCpMIP;
    int _initialSteps;
    QString _ipAddress;
    int _iBd;
    std::array<quint32,12> _timeDelays;

};

#endif // CALIBRATIONTASKS_H
