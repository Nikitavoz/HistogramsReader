#ifndef CALIBRATIONPLOTS_H
#define CALIBRATIONPLOTS_H

#include <QList>
#include "qcustomplot.h"

class CalibrationPlots: public QCustomPlot
{
    Q_OBJECT
public:
    CalibrationPlots()
      : QCustomPlot()
      , _graphAdcVsSteps()
      , _colorMapsTime()
      , _axisRects()
      , _x()
      , _y() {
        auto axr = axisRect();
        axr->axis(QCPAxis::atBottom)->setLabel("attenuator steps");
        axr->axis(QCPAxis::atLeft)->setLabel("ADC");
        axr->axis(QCPAxis::atLeft)->setScaleType(QCPAxis::stLogarithmic);

        axr->axis(QCPAxis::atTop)->setLabel("ADC vs. attenuator steps");
        axr->axis(QCPAxis::atLeft)->setRange(5, 250);
        axr->axis(QCPAxis::atBottom)->setRange(3000, 8000);
        _graphAdcVsSteps = this->addGraph(axr->axis(QCPAxis::atBottom),
                                          axr->axis(QCPAxis::atLeft));

        this->plotLayout()->insertRow(1);
        for (int i=0; i<3; ++i) {
            axr = new QCPAxisRect(this,true);
            axr->setupFullAxesBox(true);
            _axisRects.append(axr);

            //axr->setRangeZoom(Qt::Horizontal);
            //axr->setRangeDrag(Qt::Horizontal);
            axr->axis(QCPAxis::atBottom)->setLabel("time (TDC units)");
            axr->axis(QCPAxis::atLeft)->setLabel("ADC_ZERO");
            axr->axis(QCPAxis::atTop)->setLabel("ADC_ZERO vs. time");

            //axr->axis(QCPAxis::atBottom)->setRange(-40, 40);
            QCPColorMap *colorMapTime = new QCPColorMap(axr->axis(QCPAxis::atBottom),
                                                        axr->axis(QCPAxis::atLeft));
            //colorMapTime->setInterpolate(false);
            colorMapTime->data()->setSize(401, 41);
            colorMapTime->data()->setRange(QCPRange(-200,200), QCPRange(-500, 500));
            colorMapTime->setDataScaleType(QCPAxis::stLinear);
            colorMapTime->setGradient(QCPColorGradient::gpJet);
            colorMapTime->rescaleDataRange(true);
            _colorMapsTime.append(colorMapTime);
            this->plotLayout()->addElement(2,i,axr);
        }
        this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    }
    void clear() {
        _x.clear();
        _y.clear();
        for (int i=0; i<3; ++i) {
            _colorMapsTime.at(i)->data()->clear();
        }
    }
    void setAxisRange(double xMin, double xMax, double yMin, double yMax) {
        for (int i=0; i<3; ++i) {
            //_colorMapsTime.at(i)->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
            _axisRects.at(i)->axis(QCPAxis::atBottom)->setRange(xMin, xMax);
            _axisRects.at(i)->axis(QCPAxis::atLeft)->setRange(yMin, yMax);
        }
        this->replot();
    }
    void setTitles(const std::array<double, 3>& adcs) {
        for (int i=0; i<3; ++i) {
            this->plotLayout()->addElement(1, i, new QCPTextElement(this, QString::asprintf("%.0fADC", adcs[i]), QFont("sans", 12, QFont::Bold)));
        }
    }
    void rescaleDataRanges() {
        for (auto i=0; i<3; ++i) {
            _colorMapsTime.at(i)->rescaleDataRange(true);
        }
    }
    QCPColorMapData* getDataTime(int i) { return _colorMapsTime.at(i)->data(); }

    void addPoint(double x, double y) {
        _x.push_back(x);
        _y.push_back(y);
        _graphAdcVsSteps->setData(_x, _y);
        this->replot();
    }

protected:
private:
    QCPGraph *_graphAdcVsSteps;
    QList<QCPColorMap* > _colorMapsTime;
    QList<QCPAxisRect* > _axisRects;
    QVector<double> _x;
    QVector<double> _y;
};

#endif // CALIBRATIONPLOTS_H
