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
      , _colorMapsTime()
      , _axisRects()
      , _x0()
      , _y0()
      , _x1()
      , _y1() {
        auto axr = axisRect();
        axr->axis(QCPAxis::atBottom)->setLabel("attenuator steps");
        axr->axis(QCPAxis::atLeft)->setLabel("ADC");
        axr->axis(QCPAxis::atLeft)->setScaleType(QCPAxis::stLogarithmic);

        axr->axis(QCPAxis::atTop)->setLabel("ADC vs. attenuator steps");
        axr->axis(QCPAxis::atLeft)->setRange(10, 210);
        axr->axis(QCPAxis::atBottom)->setRange(5000, 7400);
        _axisRects.append(axr);
        // ADC vs. steps
        this->addGraph(axr->axis(QCPAxis::atBottom),
                       axr->axis(QCPAxis::atLeft));
        this->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

        this->addGraph(axr->axis(QCPAxis::atBottom),
                       axr->axis(QCPAxis::atLeft));
        // ADC vs. steps for interpolated step values for which CFD_ZERO scans take place
        this->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, Qt::red, Qt::red, 5));
        this->graph(1)->setLineStyle(QCPGraph::lsNone);
        this->plotLayout()->insertRow(1);
        this->plotLayout()->setRowStretchFactor(1,0.05);
        for (int i=0; i<3; ++i) {
            // title
            this->plotLayout()->addElement(1, i, new QCPTextElement(this, "", QFont("sans", 12, QFont::Bold)));

            // 2D histogram
            axr = new QCPAxisRect(this,true);
            axr->setupFullAxesBox(true);
            _axisRects.append(axr);

            //axr->setRangeZoom(Qt::Horizontal);
            //axr->setRangeDrag(Qt::Horizontal);
            axr->axis(QCPAxis::atBottom)->setLabel("time (TDC units)");
            axr->axis(QCPAxis::atLeft)->setLabel("CFD_ZERO");
            axr->axis(QCPAxis::atTop)->setLabel("CFD_ZERO vs. time");

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
        this->setMinimumHeight(400);
    }
    void clear() {
        _x0.clear();
        _y0.clear();
        this->graph(0)->setData(_x0, _y0);

        _x1.clear();
        _y1.clear();
        this->graph(1)->setData(_x1, _y1);

        for (int i=0; i<3; ++i) {
            auto cm = _colorMapsTime.at(i);
            cm->data()->clear();
            cm->data()->setSize(401, 41);
            cm->data()->setRange(QCPRange(-200,200), QCPRange(-500, 500));
            qobject_cast<QCPTextElement*>(this->plotLayout()->element(1,i))->setText("");
        }
        setAxisRange(-50, 50, -500, 500);
        rescaleDataRanges();
    }
    void setAxisRange(double xMin, double xMax, double yMin, double yMax) {
        for (int i=0; i<3; ++i) {
            //_colorMapsTime.at(i)->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
            _axisRects.at(1+i)->axis(QCPAxis::atBottom)->setRange(xMin, xMax);
            _axisRects.at(1+i)->axis(QCPAxis::atLeft)->setRange(yMin, yMax);
        }
        this->replot();
    }
    void setTitles(const std::array<double, 3>& adcs) {
        for (int i=0; i<3; ++i) {
            auto p = qobject_cast<QCPTextElement*>(this->plotLayout()->element(1,i));
            p->setText(QString::asprintf("%.0fADC", adcs[i]));
            //this->plotLayout()->addElement(1, i, new QCPTextElement(this, QString::asprintf("%.0fADC", adcs[i]), QFont("sans", 12, QFont::Bold)));
        }
        _axisRects.at(0)->axis(QCPAxis::atLeft)->setRange(0.8*adcs.front(), 1.2*adcs.back());
    }
    void rescaleDataRanges() {
        for (auto i=0; i<3; ++i) {
            _colorMapsTime.at(i)->rescaleDataRange(true);
        }
    }
    void addPoint(double x, double y, bool dots = false) {
        if (dots) {
            _x1.push_back(x);
            _y1.push_back(y);
            this->graph(1)->setData(_x1, _y1);
        } else {
            _x0.push_back(x);
            _y0.push_back(y);
            this->graph(0)->setData(_x0, _y0);
        }
    }
   void setHistogramLine(int i, int j, QVector<quint32> x) {
        for (auto k=0; k<401; ++k) {
            _colorMapsTime.at(i)->data()->setCell(k, j, x[k]);
        }
   }
   void setInitialSteps(int steps) {
        _axisRects.at(0)->axis(QCPAxis::atBottom)->setRange(steps-1850, steps+50);
   }

protected:
private:
    QList<QCPColorMap* > _colorMapsTime;
    QList<QCPAxisRect* > _axisRects;
    QVector<double> _x0;
    QVector<double> _y0;
    QVector<double> _x1;
    QVector<double> _y1;
};

#endif // CALIBRATIONPLOTS_H
