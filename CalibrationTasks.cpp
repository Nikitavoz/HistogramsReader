#include "CalibrationTasks.h"

std::unique_ptr<FITelectronics> CalibrationTasks::makeAndSetupFEE()
{
    auto p = std::unique_ptr<FITelectronics>(new FITelectronics);
    p->IPaddress = _ipAddress;
    p->iBd       = _iBd;
    p->reconnect();
    if (p->isOnline == false) {
        onIPBusError("Network ERROR: IP="+_ipAddress+"\n", networkError);
        p.reset();
        return p;
    }
    connect(p.get(), SIGNAL(error(QString,errorType)), this, SLOT(onIPBusError(QString,errorType)));
    connect(p.get(), SIGNAL(noResponse(QString)),      this, SLOT(onIPBusNoResponse(QString)));
    return p;
}

void CalibrationTasks::computeMeanStd(FITelectronics &_FEE,
                                     int typeHist,
                                     std::array<double,12>& mean,
                                     std::array<double,12>& std,
                                     std::array<int,12>& isOK,
                                     std::array<int,12>& nEntries)
{
    for (auto ch=0; ch<12; ++ch) {
        isOK[ch] = false;
        mean[ch] = 0.0;
        double sumW=0;
        for (qint32 i=0; i<4096; ++i) {
            if (typeHist == 0) { // time
                sumW += _FEE.DCh[ch].binValueTime(i);
            } else { // amplitude
                sumW += _FEE.DCh[ch].binValueAmpl(i-32);
            }
        }
        nEntries[ch] = unsigned(0.5+sumW);
        double sumWX = 0;
        double sumWXX = 0;
        qint32 maxBin = 0;
        double maxWeight = -1;
        for (qint32 i=0; i<4096; ++i) {
            double weight = 0;
            if (typeHist == 0) { // time
                auto const signedTime = (i>=2048 ? i-4096 : i);
                weight = double(_FEE.DCh[ch].binValueTime(i))/sumW;
                sumWX  += weight * signedTime;
                sumWXX += weight * signedTime * signedTime;
                if (weight > maxWeight) {
                    maxWeight = weight;
                    maxBin = signedTime;
                }
            } else { // amplitude
                weight = double(_FEE.DCh[ch].binValueAmpl(i-32))/sumW;
                sumWX  += weight * (i-32);
                sumWXX += weight * (i-32) * (i-32);
                if (weight > maxWeight) {
                    maxWeight = weight;
                    maxBin = i-32;
                }
             }
        }
        if (typeHist == 0) { // time
            sumWX = 0;
            sumWXX = 0;
            for (qint16 i=0; i<4096; ++i) {
                auto const signedTime = (i>=2048 ? i-4096 : i);
                if (abs(signedTime - maxBin) > 20) { // exclude hits outside +-20 TDC units from the mean
                    continue;
                }
                double const weight = double(_FEE.DCh[ch].binValueTime(i))/sumW;
                sumWX  += weight * signedTime;
                sumWXX += weight * signedTime * signedTime;
            }
        }
        isOK[ch] = false;
        if (sumW > 100) {
            mean[ch] = sumWX;
            std[ch]  = std::sqrt(sumWXX - sumWX*sumWX);
            isOK[ch] = (typeHist == 0 ? std[ch] < 80 : true);
        } else {
            mean[ch] = 0;
            std[ch]  = 0;
            isOK[ch] = -1;
        }
    }
}

bool CalibrationTasks::setAttenuator(FITelectronics& FEE, quint32 steps, bool verbose)
{
#if 0
        //p->clear();
        QSerialPort s(this);
        s.setPortName("COM3");
        s.setBaudRate(QSerialPort::Baud9600);
        s.setDataBits(QSerialPort::Data8);
        s.setParity(QSerialPort::NoParity);
        s.setStopBits(QSerialPort::OneStop);
        s.setFlowControl(QSerialPort::NoFlowControl);
        if (!s.open(QIODevice::ReadWrite)) {
            auto errMsg = s.errorString();
            emit logMessage(0, errMsg);
            return false;
        }
        emit logMessage(0, s.readAll());
        QByteArray const data = tr("A%1\r").arg(db,5,'f',2,QLatin1Char('0')).toLocal8Bit();
        s.write(data);
        while (s.waitForBytesWritten(100)) {
           ;
        }
        while (s.waitForReadyRead(100)) {
            emit logMessage(0, s.readAll());
        }
        if (s.isOpen()) {
            s.close();
        }
        return true;
#else
    int const maxNumBUSY = 1000;
    quint32 reg = FEE.readAtten();
    if ((reg & (1<<15)) == 1<<15) { // not found
        emit logMessage(0, "Attenuator not connected\n");
        return false;
    }
    int i = 0;
    for (; i<maxNumBUSY && (reg&(1<<14)); ++i) {
        Sleep(10);
        reg = FEE.readAtten();
    }
#if 0
    if (i) {
        emit logMessage(0, QString::asprintf("BUSY count=%3d\n",i));
    }
#endif
    if (i == maxNumBUSY) {
        emit logMessage(0, "BUSY timeout\n");
        return false;
    }
    if (verbose) {
        emit logMessage(0, QString::asprintf("Attenuator: steps=%d -> %d\n", (reg&((1<<14)-1)), steps));
    }
    reg = steps | (reg & ((1<<14)|(1<<15)));
    FEE.writeAtten(reg);
    reg = FEE.readAtten();
    for (i=0; i<maxNumBUSY && (reg&(1<<14)); ++i) {
        //emit logMessage(0, QString::asprintf("BUSY count=%3d\n",i));
        Sleep(10);
        reg = FEE.readAtten();
    }
    // emit logMessage(0, QString::asprintf("BUSY count=%3d\n",i));
    if (i == maxNumBUSY) {
        emit logMessage(0, "BUSY timeout\n");
        return false;
    }
    return true;
#endif
}

bool CalibrationTasks::adjustAttenuatorADC(FITelectronics& FEE, int ch0, float refADCValue, quint32& attenSteps)
{
    emit logMessage(0, QString::asprintf("adjustAttenuatorADC CH%02d refADCValue=%5.1f attenSteps=%d\n", ch0+1, refADCValue, attenSteps));
    if (attenSteps == 0) {
        return false;
    }
    float lastAmpl = 0.0f;
    qint32 deltaAttenSteps = -50;
    for (; attenSteps > 1000; attenSteps += deltaAttenSteps) {
        if (_abort) {
            return false;
        }
        setAttenuator(FEE, attenSteps);
        FEE.reset();
        Sleep(10);
        FEE.switchHistogramming(true);
        Sleep(200);
        FEE.readHistograms(hAmpl);

        std::array<double,12> meanAmpl, stdAmpl;
        std::array<int,12>    amplOK, nEntries;
        computeMeanStd(FEE, 1, meanAmpl, stdAmpl, amplOK, nEntries);
        emit logMessage(1+ch0, QString::asprintf("steps=%5d ADC=%6.1f+-%.1f (N=%d)\n", attenSteps, meanAmpl[ch0], stdAmpl[ch0], nEntries[ch0]));
        emit addPointADCvSteps(ch0, attenSteps, meanAmpl[ch0], false); //calPlots.addPoint(attenSteps, charge);

        if (meanAmpl[ch0] > refADCValue && lastAmpl > 0) {
            // linear interpolation
            float const slope = float(deltaAttenSteps) / (meanAmpl[ch0] - lastAmpl); // steps/ADC
            int const attenStepsI = std::lround(float(attenSteps) - float(deltaAttenSteps) + slope * (refADCValue - lastAmpl));
            emit logMessage(1+ch0, QString::asprintf("Interpolation lastAmpl=%f ampl=%f slope=%f steps=%d [%d,%d]\n",
                                                     lastAmpl, meanAmpl[ch0], slope, attenStepsI, attenSteps-deltaAttenSteps,attenSteps));
            emit addPointADCvSteps(ch0, attenStepsI, refADCValue, true); // calPlots.addPoint(attenStepsI, refADCValue, true);
            attenSteps = attenStepsI;
            setAttenuator(FEE, attenSteps, true);
            return true;
        }
        lastAmpl = meanAmpl[ch0];
    }
    return false;
}

std::pair<int,bool> CalibrationTasks::calCFDThreshold(FITelectronics& FEE, int ch0, std::array<quint32,3>& attenSteps, float adcPerMIP, bool coarse, int startCFDOffset) {
    std::pair<int, bool> result = {-1000, false};

    // save register values
    quint32 adcRegs[4*12];
    bool  success = FEE.readADCRegisters(adcRegs);
    quint32 adcRegsOld[4*12];
    std::copy(adcRegs, adcRegs+4*12, adcRegsOld);

    std::array<double,12> meanTime, stdTime;
    std::array<int,12> timeOK, nEntries;

    // counter register values
    quint32 counters[24];
    quint32 countersOld[24];

    std::array<double,3> attenuation = {adcPerMIP,adcPerMIP*sqrt(10.0),adcPerMIP*10.0};
    std::array<int,41> cfdZERO;
    for (int i=0; i<cfdZERO.size(); ++i) {
        if (coarse) {  // coarse scan: -500:25:500
            cfdZERO[i] = -500 + 25*i;
        } else {       //  fine  scan: startCFDOffset [-100:5:100]
            cfdZERO[i] = startCFDOffset - 100 + 5*i;
        }
    }
    emit setTitlesADC(ch0, attenuation);

    struct MeasurementValue {
        MeasurementValue(double tm=0, double tr=0, int ok=0, double r=0, double cm=0, double cr=0)
            : tMean(tm)
            , tRMS(tr)
            , tOK(ok)
            , rate(r)
            , cMean(cm)
            , cRMS(cr) {}
        ~MeasurementValue() {}
         double tMean = 0.0;
         double tRMS  = 0.0;
         int    tOK   = false;
         double rate  = 0.0;
         double cMean = 0.0;
         double cRMS  = 0.0;
         std::string format() const {
             return QString::asprintf("%6.1f±%-3.0f %4.0f", tMean, tRMS, rate).toStdString();
         }
         bool operator<(const MeasurementValue& v) const {
             return tMean < v.tMean;
         }
    };

    std::array<std::array<std::array<MeasurementValue, attenuation.size()>, cfdZERO.size()>, 12> times = {};
    quint32 steps = FEE.readAtten() &((1<<14)-1);
    quint32 attenStepsOrig = steps;
    emit logMessage(0, QString::asprintf("steps=%d\n", steps));

    for (int i=0; i<attenuation.size(); ++i) {
        if (coarse) {
            if (!adjustAttenuatorADC(FEE, ch0, attenuation[i], steps)) {
                result.second = false;
                return result;
            }
            attenSteps[i] = steps;
        } else { // fine calibration
            if (!setAttenuator(FEE, attenSteps[i], true)) {
                result.second = false;
                return result;
            }
        }
        Sleep(100);
        emit logMessage(0, QString::asprintf("CFD_ZERO scan [%d,%d]: ", cfdZERO.front(), cfdZERO.back()));
        for (int j=0; j<cfdZERO.size(); j+= 1+coarse) {
            if (_abort) {
                FEE.writeADCRegisters(adcRegsOld);
                emit logMessage(0, "ABORT\n");
                return result;
            }
            for (int ch=0; ch<12; ++ch) {
                adcRegs[4*ch+1] = cfdZERO[j];
            }
            emit logMessage(0, ".");
            emit logMessage(1+ch0, QString::asprintf("%5d", cfdZERO[j]));
            FEE.reset();
            success = FEE.writeADCRegisters(adcRegs);
            Sleep(20);

            FEE.switchHistogramming(true);
            success = FEE.readCounters(countersOld);
            Sleep(200);
            success = FEE.readCounters(counters);
            FEE.readHistograms(hTime);
            computeMeanStd(FEE, 0, meanTime,stdTime,timeOK,nEntries);
            QVector<quint32> histLine(401);
            for (int k=0; k<4096; ++k) {
                int const signedTime = (k>=2048 ? k-4096 : k);
                int idx = 200 + signedTime;
                if (idx < 0 || idx > 400) {
                    continue;
                }
                histLine[idx] = FEE.DCh[ch0].binValueTime(k);
            }
            if (coarse) {
                emit addHistLine(ch0, i, j, histLine);
                if (j+1 != cfdZERO.size()) {
                    emit addHistLine(ch0, i, j+1, histLine);
                }

            }
            for (int ch=0; ch<12; ++ch) {
                times[ch][j][i] = {meanTime[ch], stdTime[ch], timeOK[ch],
                                   double(counters[2*ch]-countersOld[2*ch])/0.2,
                                   0.0, 0.0};
            }
        }
        emit logMessage(0, "\n");
        emit logMessage(1+ch0, "\n");

    }
    emit logMessage(0, "\n\n");
    setAttenuator(FEE, attenStepsOrig, true);

//    calPlots.rescaleDataRanges();
//    calPlots.rescaleAxes();
//    calPlots.setAxisRange(-50, 50, cfdZERO.front(), cfdZERO.back());
//    calPlots.replot();
//    double const width = 1200;
//    double const height = 600;
//    QTextCursor cursor = ui->calTextOutput->textCursor();
//    cursor.insertText(QString(QChar::ObjectReplacementCharacter), QCPDocumentObject::generatePlotFormat(&calPlots, width, height));
//    ui->calTextOutput->setTextCursor(cursor);

    result.first = -1000;
    int& newCFDValue = result.first; // set to invalid
    emit logMessage(0, "\n PM  CH  CFD_ZERO");
    for (int j=0; j<attenuation.size(); ++j) {
        emit logMessage(0, QString::asprintf("%6.0fADC      ", attenuation[j]));
    }
    emit logMessage(0, " | Time Variation\n");
    float timeDifferenceLast = 100;
    bool haveSeenNegativeTimeDifference = false;
    int const refAttenIndex = (coarse ? 0 : 0);
    for (int i=0; i<cfdZERO.size(); i+= 1+coarse) {
        emit logMessage(0, QString::asprintf(" %s  CH%02d %6d ",
                                       FEE.formatPM().c_str(),
                                       ch0, cfdZERO[i]));
        for (int j=0; j<attenuation.size(); ++j) {
            emit logMessage(0, QString::asprintf("%s", times[ch0][i][j].format().c_str()));
        }
        auto const tMinMax        = std::minmax_element(times[ch0][i].begin(), times[ch0][i].end());
        auto const timeDifference = times[ch0][i][refAttenIndex].tMean - times[ch0][i][2].tMean;
        if (!haveSeenNegativeTimeDifference && timeDifference < 0) {
            haveSeenNegativeTimeDifference = true;
        }
        emit logMessage(0, QString::asprintf(" | %7.2f", tMinMax.second->tMean - tMinMax.first->tMean));
        emit logMessage(0, QString::asprintf(" %+8.2f", timeDifference));
        if (coarse) {
            if (times[ch0][i][refAttenIndex].rate >= 500 && timeDifference > 0 && timeDifferenceLast <= 0 && newCFDValue == -1000) {
                // linear interpolation
                float slope = (timeDifference - timeDifferenceLast) / (cfdZERO[i]-cfdZERO[i-1]);
                newCFDValue = 5*std::lround((cfdZERO[i-1] - timeDifferenceLast / slope)/5);
                emit logMessage(0, QString::asprintf(" <----- ***** (%d)\n", newCFDValue));
                result.second = true;
           } else {
                emit logMessage(0, "\n");
           }
        } else { // fine adjustment
            if (times[ch0][i][refAttenIndex].rate >= 990 && timeDifference >= 1.5 && haveSeenNegativeTimeDifference && newCFDValue == -1000) {
                newCFDValue = cfdZERO[i];
                emit logMessage(0, " <----- *****\n");
                result.second = true;
            } else {
                emit logMessage(0, "\n");
            }
        }
        timeDifferenceLast = timeDifference;
    }
    emit logMessage(0, "\n\n");

    success = FEE.writeADCRegisters(adcRegsOld);
    emit logMessage(0, success ? "\nOK\n" : "\nFAILED\n");
    emit logMessage(1+ch0, success ? "\nOK\n" : "\nFAILED\n");
    return result;
}
