#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <QMainWindow>
#include <QElapsedTimer>
#include "ui_mainwindow.h"
#include "switch.h"
#include "FITelectronics.h"
#include "qcustomplot.h"

static inline double roundDownOrder(double v) { return pow(10, floor( log10(fabs(v)) )); } //round down one decimal order of magnitude
static inline double roundUpOrder(double v) { return pow(10, floor( log10(fabs(v)) + (v > 0 ? 1 : 0) )); } //round up one decimal order of magnitude for positive values
static inline double roundUpPlace(double v) { double b = roundDownOrder(v); return (floor(v / b) + (v > 0 ? 1 : -1)) * b; } //round up one most significant decimal place of abs value
static inline double roundUpStep(double v) { double b = roundDownOrder(v), r = v/b; return r > 2 ? (r > 5 ? 10*b : 5*b) : 2*b; } //1, 2, 5, 10, 20, 50, 100...

namespace Ui {class MainWindow;}
class MainWindow : public QMainWindow {
    Q_OBJECT
    Ui::MainWindow *ui;
public:
    QPixmap
        Green0 = QPixmap(":/0G.png"), //OK
        Green1 = QPixmap(":/1G.png"), //OK
        Red0 = QPixmap(":/0R.png"), //not OK
        Red1 = QPixmap(":/1R.png"), //not OK
        b0icon = QPixmap(10,10), b1icon = b0icon;
    const QFont regularValueFont = QFont("Consolas", 10, QFont::Normal);
    QString
        OKstyle    = QString::asprintf("background-color: rgba(%d, %d, %d, 63)", OKcolor   .red(), OKcolor   .green(), OKcolor   .blue()),
        notOKstyle = QString::asprintf("background-color: rgba(%d, %d, %d, 63)", notOKcolor.red(), notOKcolor.green(), notOKcolor.blue());
    QRegExp validIPaddressRE {"([1-9][0-9]?|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.(([1-9]?[0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){2}([1-9][0-9]?|1[0-9][0-9]|2[0-4][0-9]|25[0-5])"};
    QAction *recheck;
    FITelectronics FEE;
    QSettings settings;
    int fontSize_px, axisLength_px;
    bool ok, FV0 = false, lg = false; //logarithmic scale for counts (y-axis)
    QList<QWidget *> allWidgets, PMwidgets, TCMwidgets;
    QElapsedTimer timer;
    TypeOfHistogram curType = hTrig;
    quint32 maxTCM = 0, maxPM = 0, *max = &maxTCM, threshold = 1;
    double ADCUperMIP = 16, mVperMIP = 7.5;
    template<typename T> struct yAxisTicker: public T {
        QString getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) { Q_UNUSED(locale) Q_UNUSED(formatChar) Q_UNUSED(precision)
            return fabs(tick) < 999.5 ? QString::asprintf("%3.0f", tick) : QString::asprintf("%.0e", tick).replace("e+0", "e");
        }
        double getTickStep(const QCPRange &range) { return roundDownOrder(range.upper * 0.9); }
        int getSubTickCount(double tickStep) { return tickStep > 1 ? 9 : 0; }
    };
    QSharedPointer<QCPAxisTicker> linTicker = QSharedPointer<QCPAxisTicker>(new yAxisTicker<QCPAxisTickerFixed>()), logTicker = QSharedPointer<QCPAxisTicker>(new yAxisTicker<QCPAxisTickerLog>());
    struct xAxisTicker: public QCPAxisTickerFixed {
        int &fontSize_px, &axisLength_px;
        xAxisTicker(int &_fontSize_px, int &_axisLength_px): fontSize_px(_fontSize_px), axisLength_px(_axisLength_px) {}
        double getTickStep(const QCPRange &range) { return range.size() > axisLength_px / fontSize_px ? roundUpStep(range.size() * fontSize_px / axisLength_px) : 1;  }
        int getSubTickCount(double tickStep) { return tickStep/roundDownOrder(tickStep) - 1; }
    };
    QSharedPointer<QCPAxisTicker> binTicker = QSharedPointer<xAxisTicker>(new xAxisTicker(fontSize_px, axisLength_px)), floatTicker = QSharedPointer<QCPAxisTicker>(new QCPAxisTicker);;
    struct Histogram {
        QCustomPlot *plot;
        const quint8 iCh;
        TypeOfHistogram type;
        QString name;
        QList<QCPBars *> bars;
        QCPItemText *title = new QCPItemText(plot);
        TypeStats stats;
        void addBars(QColor color = OKcolor.darker(200)) {
            QCPBars *b = new QCPBars(plot->xAxis, plot->yAxis);
            for (qint16 bin=minBin[type]; bin<=maxBin[type]; ++bin) b->addData(bin, 0);
            b->setBrush(color);
            b->setPen(OKcolor.darker(400));
            b->setWidth(1);
            b->setStackingGap(0);
            if (!bars.isEmpty()) b->moveAbove(bars.last());
            bars.append(b);
        }
        Histogram(QCustomPlot *p, quint8 index, TypeOfHistogram ht): plot(p), iCh(index), type(ht)  { addBars(); }
        void setFullRange() { plot->xAxis->setRange((minBin[type]-0.5)*bars[0]->width(), (maxBin[type]+0.5)*bars[0]->width()); }
    };
    QVector<QList<Histogram *>> H {3};

    explicit MainWindow(QWidget *parent = nullptr):
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        settings(QCoreApplication::applicationName() + ".ini", QSettings::IniFormat)
    {
        ui->setupUi(this);
        recheck = new QAction(this);
        connect(recheck, &QAction::triggered, this, &MainWindow::recheckTarget);
        recheck->setShortcut(QKeySequence::Refresh);
        recheck->setShortcutContext(Qt::ApplicationShortcut);
        ui->tabWidget->addAction(recheck);
//initial scaling (a label with fontsize 10 (Calibri) has pixelsize of 13 without system scaling and e.g. 20 at 150% scaling so widgets size should be recalculated)
        fontSize_px = ui->labelValueIPaddress->fontInfo().pixelSize();
        if (fontSize_px > 13) { //not default pixelsize for font of size 10
            double f = fontSize_px / 13.;
            resize(size()*f); //mainWindow
            setMinimumSize(minimumSize()*f);
            foreach (QWidget *w, allWidgets) { w->resize(w->size()*f); w->move(w->pos()*f); }
            ui->groupBoxControl->setMinimumSize(ui->groupBoxControl->minimumSize()*f);
            ui->groupBoxTarget ->setMinimumSize(ui->groupBoxTarget ->minimumSize()*f);
            ui->groupBoxFile   ->setMinimumSize(ui->groupBoxFile   ->minimumSize()*f);
        }
//initialization
        setWindowTitle(QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion());
        allWidgets = ui->groupBoxControl->findChildren<QWidget *>() + ui->groupBoxView->findChildren<QWidget *>();
        PMwidgets = ui->groupBoxControl->findChildren<QWidget *>(QRegularExpression(".*PM"));
        TCMwidgets = ui->groupBoxControl->findChildren<QWidget *>(QRegularExpression(".*TCM"));
        foreach(QCustomPlot *p, ui->Triggers->findChildren<QCustomPlot *>()) H[hTrig].append(new Histogram(p, H[hTrig].length(), hTrig));
        foreach(QCustomPlot *p, ui->Time    ->findChildren<QCustomPlot *>()) H[hTime].append(new Histogram(p, H[hTime].length(), hTime));
        foreach(QCustomPlot *p, ui->Charge  ->findChildren<QCustomPlot *>()) {
            H[hAmpl].append(new Histogram(p, H[hAmpl].length(),  hAmpl));
            H[hAmpl].last()->addBars(OKcolor);
        }
        b0icon.fill(H[hAmpl].first()->bars[0]->brush().color()); ui->radioADC0->setIcon(b0icon);
        b1icon.fill(H[hAmpl].first()->bars[1]->brush().color()); ui->radioADC1->setIcon(b1icon);
        foreach(QList<Histogram *> group, H) foreach(Histogram *h, group) {
            h->name = h->plot->objectName().remove(0, 8);
            QCPItemText *t = h->title;
                t->setText(h->name + " counts");
                t->setLayer("axes");
                t->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
                t->position->setType(QCPItemPosition::ptAbsolute);
                t->position->setCoords(1.6 * fontSize_px, fontSize_px);
                t->setClipToAxisRect(false);
                t->setFont(regularValueFont);
                t->setBrush(Qt::white);
            h->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
            QCPAxisRect *r = h->plot->axisRect();
                r->setRangeDrag(Qt::Horizontal);
                r->setRangeZoom(Qt::Horizontal);
                r->setBackgroundScaled(false);
                r->setBackground(QBrush(QColor(0, 0, 0, 30)));
                r->setAutoMargins(QCP::msNone);
                r->setMargins(QMargins(1.6 * fontSize_px, fontSize_px + 2, 0.4 * fontSize_px, 2.2 * fontSize_px));
            QCPAxis *y = h->plot->yAxis;
                y->setTicker(linTicker);
                y->grid()->setPen(QPen(QColor(140, 140, 140, 80), 1));
                y->grid()->setZeroLinePen(Qt::NoPen);
                y->setNumberFormat("f");
                y->setTickLength(0,1);
                y->setSubTickLength(0,1);
                y->setTickLabelPadding(1);
                y->setLabelPadding(0);
                y->setPadding(0);
            QCPAxis *x = h->plot->xAxis;
                x->setTicker(binTicker);
                x->grid()->setPen(QPen(QColor(140, 140, 140, 80), 1));
                x->grid()->setZeroLinePen(Qt::NoPen);
                x->setTickLength(0,0);
                x->setSubTickLength(0,1);
                x->setTickLabelPadding(0);
                x->setLabelPadding(0);
                x->setRange(minBin[h->type]-0.5, maxBin[h->type]+0.5);
                x->setTickLabelRotation(-90);
            connect(h->plot->xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, [=](const QCPRange &newRange) {
                if (!ui->radioAdjustable->isChecked()) return;
                h->plot->xAxis->setRange( qMax(newRange.lower, (minBin[h->type]-0.5) * h->bars[0]->width()), qMin(newRange.upper, (maxBin[h->type]+0.5) * h->bars[0]->width()) );
                updatePlot(h);
            });
            connect(h->plot, &QCustomPlot::mouseDoubleClick, this, [=](QMouseEvent * event) {
                if (!ui->radioAdjustable->isChecked()) return;
                if      (event->button() == Qt::LeftButton )  h->plot->xAxis->setRangeUpper(h->plot->xAxis->pixelToCoord(event->x()));
                else if (event->button() == Qt::RightButton)  h->plot->xAxis->setRangeLower(h->plot->xAxis->pixelToCoord(event->x()));
                else if (event->button() == Qt::MiddleButton) h->setFullRange();
                updatePlot(h);
            });
        }
//signal-slot conections
        connect(&FEE, &IPbusTarget::IPbusStatusOK, this, [=]() {
           ui->labelStatus->setStyleSheet(OKstyle);
           FV0 = (FEE.readRegister(0x7) & 0b11) == 2;
           QComboBox *b = ui->comboBoxSelectableHistogramTCM;
           b->setItemData(0x0, "disabled"                    , Qt::DisplayRole); b->setItemData(0x0, ""                                   , Qt::ToolTipRole);
           b->setItemData(0x1,                  "OrA"        , Qt::DisplayRole); b->setItemData(0x1, ""                                   , Qt::ToolTipRole);
           b->setItemData(0x2, FV0?"OuterRings":"OrC"        , Qt::DisplayRole); b->setItemData(0x2, ""                                   , Qt::ToolTipRole);
           b->setItemData(0x3, FV0?"Nchannels" :"SemiCentral", Qt::DisplayRole); b->setItemData(0x3, ""                                   , Qt::ToolTipRole);
           b->setItemData(0x4, FV0?"Charge"    :"Central"    , Qt::DisplayRole); b->setItemData(0x4, ""                                   , Qt::ToolTipRole);
           b->setItemData(0x5, FV0?"InnerRings":"Vertex"     , Qt::DisplayRole); b->setItemData(0x5, ""                                   , Qt::ToolTipRole);
           b->setItemData(0x6,             "NoiseA"          , Qt::DisplayRole); b->setItemData(0x6, "A-side out-of-gate hit AND NOT OrA" , Qt::ToolTipRole);
           b->setItemData(0x7,             "NoiseC"          , Qt::DisplayRole); b->setItemData(0x7, "C-side out-of-gate hit AND NOT OrC" , Qt::ToolTipRole);
           b->setItemData(0x8,             "Total noise"     , Qt::DisplayRole); b->setItemData(0x8, "NoiseA OR NoiseC"                   , Qt::ToolTipRole);
           b->setItemData(0x9,             "True OrA"        , Qt::DisplayRole); b->setItemData(0x9, "bunch in both beams AND OrA"        , Qt::ToolTipRole);
           b->setItemData(0xA,             "True OrC"        , Qt::DisplayRole); b->setItemData(0xA, "bunch in both beams AND OrC"        , Qt::ToolTipRole);
           b->setItemData(0xB,             "Interaction"     , Qt::DisplayRole); b->setItemData(0xB, "both sides OR (OrA & OrC)"          , Qt::ToolTipRole);
           b->setItemData(0xC,             "True Interaction", Qt::DisplayRole); b->setItemData(0xC, "bunch in both beams AND OrA AND OrC", Qt::ToolTipRole);
           b->setItemData(0xD,             "True Vertex"     , Qt::DisplayRole); b->setItemData(0xD, "bunch in both beams AND Vertex"     , Qt::ToolTipRole);
           b->setItemData(0xE,             "Beam-gas A"      , Qt::DisplayRole); b->setItemData(0xE, "bunch ONLY in beam 1 AND OrC"       , Qt::ToolTipRole);
           b->setItemData(0xF,             "Beam-gas C"      , Qt::DisplayRole); b->setItemData(0xF, "bunch ONLY in beam 2 AND OrA"       , Qt::ToolTipRole);
           ui->radButADC->setText(FV0 ? "ADC (≈83fC)" : "ADC (≈43fC)");
        });
        connect(&FEE, &IPbusTarget::error, this, [=](QString message, errorType et) {
            ui->labelStatus->setText(message);
            ui->labelStatus->setStyleSheet(notOKstyle);
            ui->groupBoxControl->setDisabled(true);
            QMessageBox::warning(this, errorTypeName[et], message);
        });
        connect(&FEE, &IPbusTarget::noResponse, this, [=](QString message) {
            ui->labelStatus->setText(ui->labelStatus->text() == "" ? message : "");
            ui->labelStatus->setStyleSheet(notOKstyle);
            ui->groupBoxControl->setDisabled(true);
        });
        connect(&FEE, &FITelectronics::SPIlinksStatusUpdated, this, &MainWindow::updateBoardsList);
        connect(&FEE, &FITelectronics::statusReady, this, &MainWindow::updateStatus);
        ui->labelValueTime->setText("0.0");
        ui->labelValueSpeed->setText("0.0");
        ui->labelValueMax->setText("0");
//restore settings from .ini
        QString IPaddress = settings.value("IPaddress", FEE.IPaddress).toString();
        if (validIPaddressRE.exactMatch(IPaddress)) { FEE.IPaddress = IPaddress; ui->labelValueIPaddress->setText(IPaddress); }
        ui->autoRead->setChecked(settings.value("AutoRead", true).toBool());
        ui->spinBoxMax->setValue(settings.value("AutoResetValue", UINT16_MAX).toUInt());
        ui->doAutoReset->setChecked(settings.value("AutoReset", false).toBool());
        if (settings.value("logScale", false).toBool()) if (!ui->radioLogScale->isChecked()) ui->radioLogScale->click();
        QString s = settings.value("Range", "adjustable").toString();
        if (s == "full") ui->radioFullRange->setChecked(true); else if (s == "thresholded") ui->radioThreshold->setChecked(true);
        ui->spinBoxThreshold->setValue(settings.value("Threshold", 1).toUInt());
        if (settings.value("TimeUnit", "TDC").toString() == "ns") ui->radBut_ns->setChecked(true);
        mVperMIP = settings.value("MIP/mV", 7.5).toDouble();
        ADCUperMIP = settings.value("MIP/ADCunit", 16).toDouble();
        s = settings.value("ChargeUnit", "ADC").toString(); if (s == "mV") ui->radBut_mV->setChecked(true); else if (s == "MIP") ui->radButMIP->setChecked(true);
        resize(settings.value("WindowSize", minimumSize()).toSize());
        if (settings.value("MaximizedWindow", false).toBool()) showMaximized();
        ui->labelStatus->clear();
        updateBoardsList(0);
        on_tabWidget_currentChanged(0);
        FEE.reconnect();
    }

    ~MainWindow() {
        settings.setValue("IPaddress", FEE.IPaddress);
        settings.setValue("AutoRead",  ui->autoRead->isChecked());
        settings.setValue("AutoResetValue",  ui->spinBoxMax->value());
        settings.setValue("AutoReset",  ui->doAutoReset->isChecked());
        settings.setValue("logScale",  lg);
        settings.setValue("Range", ui->radioAdjustable->isChecked() ? "adjustable" : (ui->radioFullRange->isChecked() ? "full" : "thresholded"));
        settings.setValue("Threshold", threshold);
        settings.setValue("TimeUnit", ui->radButTDC->isChecked() ? "TDC" : "ns");
        settings.setValue("MIP/mV", mVperMIP);
        settings.setValue("MIP/ADCunit", ADCUperMIP);
        settings.setValue("ChargeUnit", ui->radButADC->isChecked() ? "ADC" : (ui->radBut_mV->isChecked() ? "mV" : "MIP"));
        settings.setValue("WindowSize", size());
        settings.setValue("MaximizedWindow", isMaximized());
        foreach(QList<Histogram *> group, H) foreach(Histogram *h, group) delete h;
        delete ui;
    }
    void resizeEvent(QResizeEvent *event) { Q_UNUSED(event)
        axisLength_px = H[curType].first()->plot->width() - 2 * fontSize_px;
        foreach(Histogram *h, H[curType]) h->plot->replot();
    }
    void refillBars(Histogram *h, TypeOfHistogram hType) {
        quint8 iBar = hType==hADC1 ? 1 : 0;
        QCPBarsData *i = h->bars[iBar]->data()->begin();
        for(qint16 bin=minBin[hType]; bin<=maxBin[hType]; ++bin, ++i) {
            quint32 v = FEE.binValue[hType](h->iCh, bin);
            i->key = bin * h->bars[iBar]->width();
            i->value = v;
        }
    }
    void refillPlot(Histogram *h) {
        if (h->type == hAmpl) {
            refillBars(h, hADC0);
            refillBars(h, hADC1);
        } else refillBars(h, h->type);
    }
    void updatePlot(Histogram *h) {
        double w = h->bars[0]->width();
        if (ui->radioFullRange->isChecked()) h->setFullRange();
        else if (ui->radioThreshold->isChecked()) {
            qint16 first = minBin[h->type], last = maxBin[h->type];
            for (qint16 bin=minBin[h->type]; bin<=maxBin[h->type]; ++bin) if (FEE.binValue[h->type](h->iCh, bin) >= threshold) { first = bin; break; }
            for (qint16 bin=maxBin[h->type]; bin>=minBin[h->type]; --bin) if (FEE.binValue[h->type](h->iCh, bin) >= threshold) { last  = bin; break; }
            h->plot->xAxis->setRange((first-0.5)*w, (last+0.5)*w);
        }
        h->stats = FEE.calcStats(h->type, h->iCh, h->plot->xAxis->range().lower/w, h->plot->xAxis->range().upper/w);
        h->title->setText(h->name + (h->stats.sum ? QString::asprintf(" Σ=%d μ=%.2f σ=%.2f", h->stats.sum, h->stats.mean * w, h->stats.RMS * w) : " Σ=0"));
        h->plot->yAxis->setRange(lg ? 0.8 : 0, (h->stats.max ? (lg ? roundUpOrder(h->stats.max) : roundUpPlace(h->stats.max)) : 5));
        h->plot->replot();
    }
    void readData() {
        timer.start();
        TypeResultSize size = FEE.readHistograms(curType);
        double ms = timer.nsecsElapsed() / 1e6;
        if (size.read != size.expected) {
            emit FEE.error(QString::asprintf("%.1f%% data lost!", 100 - size.read * 100. / size.expected ), networkError);
            return;
        }
        ui->labelValueTime ->setText(ms > 0.1 ? QString::asprintf("%.1f", ms                    ) :                   "<1"                       );
        ui->labelValueSpeed->setText(ms > 0.1 ? QString::asprintf("%.1f", size.read * 0.032 / ms) : QString::asprintf(">%.1f", size.read * 0.032));   
    }
    void showData() {
        *max = FEE.maxBinValue(curType == hTrig);
        ui->labelValueMax->setText(QString::asprintf("%u", *max));
        foreach(Histogram *h, H[curType]) { refillPlot(h); updatePlot(h); }
    }
    void checkBoardSelection() {
        bool wrongBoard = (curType == hTrig) != (FEE.iBd == 0); //Trigger histograms only for TCM, other histograms only for PMs
        ui->groupBoxMax     ->setEnabled(!wrongBoard);
        ui->groupBoxDataRead->setEnabled(!wrongBoard);
        ui->autoRead        ->setVisible(!wrongBoard);
        foreach(QWidget *w,  PMwidgets) w->setVisible(FEE.iBd != 0);
        foreach(QWidget *w, TCMwidgets) w->setVisible(FEE.iBd == 0);
    }

public slots:
    void recheckTarget() {
        ui->labelStatus->setText("reconnecting");
        ui->labelStatus->setStyleSheet(notOKstyle);
        FEE.reconnect();
    }
    void on_buttonChangeIPaddress_clicked() {
        QString text = QInputDialog::getText(this, "Changing target", "Enter new target's IP address", QLineEdit::Normal, FEE.IPaddress, &ok);
        if (ok && !text.isEmpty()) {
            if (validIPaddressRE.exactMatch(text)) {
                FEE.IPaddress = text;
                ui->labelValueIPaddress->setText(text);
                FEE.reconnect();
            } else QMessageBox::warning(this, "Warning", text + ": invalid IP address. Continue with previous target");
        }
    }
    void updateStatus() {
        ui->groupBoxControl->setDisabled(FEE.TCMstatus.resetSystem);
        if (FEE.iBd) { //PM
            ui->labelIconTriggerSyncPM->setPixmap(FEE.triggerLinkStatus.linkOK ? Green1 : Red0);
            ui->labelIconClockSyncPM->setPixmap(FEE.PMstatus.PLLlocked == 0b1111 && FEE.PMstatus.syncError == 0 ? Green1 : Red0);
            ui->switchHistorgammingPM->setChecked(FEE.histStatus.histOn);
            ui->switchFilterPM->setChecked(FEE.histStatus.filterOn);
            if (ui->spinBoxBCfilterPM->value() != FEE.histStatus.BCid) ui->spinBoxBCfilterPM->setValue(FEE.histStatus.BCid);
        } else { //TCM
            ui->comboBoxSelectableHistogramTCM->setCurrentIndex(FEE.TCMmode.selectableHist);
            ui->comboBoxSelectableHistogramTCM->setToolTip(ui->comboBoxSelectableHistogramTCM->currentData(Qt::ToolTipRole).toString());
            H[hTrig].first()->name = ui->comboBoxSelectableHistogramTCM->currentText();
        }
        ui->labelStatus->setText(ui->labelStatus->text() == "" ? "online" : "");
        if (ui->autoRead->isChecked() && ui->buttonRead->isEnabled()) on_buttonRead_clicked();
    }
    void updateBoardsList(quint32 mask) {
        QString selectedItem = ui->comboBoxBoard->currentText();
        ui->comboBoxBoard->clear();
        ui->comboBoxBoard->addItem("TCM");
        for (quint8 i=0; i<10; ++i, mask >>= 1) if (mask & 1) ui->comboBoxBoard->addItem(QString("PMA") + QChar('0' + i));
        for (quint8 i=0; i<10; ++i, mask >>= 1) if (mask & 1) ui->comboBoxBoard->addItem(QString("PMC") + QChar('0' + i));
        ui->comboBoxBoard->addItem("rescan");
        int i = selectedItem == "rescan" || selectedItem == "TCM" ? -1 : ui->comboBoxBoard->findText(selectedItem);
        ui->comboBoxBoard->setCurrentIndex(i == -1 ? 0 : i);
        if (i == -1) on_comboBoxBoard_textActivated("TCM");
    }
    void on_comboBoxBoard_textActivated(const QString &text) {
        if      (text == "rescan") { FEE.checkPMlinks(); return; }
        else if (text == "TCM")                           FEE.iBd = 0;
        else if (QRegExp("PM[AC][0-9]").exactMatch(text)) FEE.iBd = (text[2].toLatin1() == 'A' ? 1 : 11) + text[3].toLatin1() - '0';
        else return;
        ui->buttonReset->setText("RESET (" + text + ")");
        checkBoardSelection();
        FEE.sync();
    }
    void on_comboBoxSelectableHistogramTCM_activated(int n) { FEE.selectTriggerHistogram(n); }
    void on_switchHistorgammingPM_clicked(bool checked) { FEE.switchHistogramming(!checked); }
    void on_switchFilterPM_clicked(bool checked) { FEE.switchBCfilter(!checked); }
    void on_spinBoxBCfilterPM_valueChanged(int id) { FEE.setBC(id); }
    void on_buttonReset_clicked() { FEE.reset(); }
    void on_buttonRead_clicked() {
        readData();
        showData();
        if (ui->doAutoReset->isChecked() && *max >= ui->spinBoxMax->value()) FEE.reset();
    }
    void on_buttonSave_clicked() {
        QString hName = ui->tabWidget->currentWidget()->objectName();
        QFile f(QFileDialog::getSaveFileName(this, "Save " + hName + " histograms data", "./Histograms" + hName + ".csv", "Colon-separated values (*.csv)"));
        f.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
        QTextStream out(&f);
        int width = curType == hTrig ? qMax(10, H[hTrig].first()->name.length()) : (curType == hAmpl ? 6 : 5);
        out << (curType == hTrig ? "   BC" : "  bin");
        if (curType == hAmpl) foreach(Histogram *h, H[hAmpl  ]) out << ":" + h->name + "A0:" + h->name + "A1";
        else                  foreach(Histogram *h, H[curType]) out << h->name.rightJustified(width).prepend(":");
        out << Qt::endl;
        for (int iBin=minBin[curType]; iBin<=maxBin[curType]; ++iBin) {
            out << QString::asprintf("%5d", iBin);
            for (int iCh=0; iCh<H[curType].length(); ++iCh)
                if (curType == hAmpl) out << QString::asprintf(":%6d:%6d"   , FEE.binValue[hADC0](iCh, iBin), FEE.binValue[hADC1](iCh, iBin));
                else                  out << QString::asprintf(":%*d", width, FEE.binValue[curType](iCh, iBin));
            out << Qt::endl;
        }
        f.close();
        ui->labelStatus->setText(hName + " data written");
    }
    void on_buttonLoad_clicked() {
        QFile f(QFileDialog::getOpenFileName(this, "Open histograms data", ".", "Colon-separated values (*.csv)"));
        if (f.size() < 70) return;
        f.open(QFile::ReadOnly | QFile::Text);
        QString line;
        do line = f.readLine(); while (line.isEmpty());
        switch (line.split(":").length() - 1) {
            case  3: curType = hTrig; break;
            case 12: curType = hTime; break;
            case 24: curType = hAmpl; break;
            default: return;
        }
        while (!f.atEnd()) {
            line = f.readLine();
            if (line.isEmpty()) continue;
            QStringList wordList = line.split(":"); if (wordList.length() < 2) continue;
            qint16 iBin = wordList.takeFirst().toShort(&ok); if (!ok || iBin < minBin[curType] || iBin > maxBin[curType]) continue;
            if (curType == hAmpl) for (quint8 i=0; i<wordList.length() && i<24                 ; ++i) { quint32 v = wordList[i].toUInt(&ok); FEE.setBinValue[i&1 ? hADC1 : hADC0](i/2, iBin, ok ? v : 0); }
            else                  for (quint8 i=0; i<wordList.length() && i<H[curType].length(); ++i) { quint32 v = wordList[i].toUInt(&ok); FEE.setBinValue[            curType](i  , iBin, ok ? v : 0); }
        }
        f.close();
        ui->autoRead->setChecked(false);
        ui->tabWidget->setCurrentIndex(curType);
        showData();
    }
    void on_radioAdjustable_toggled(bool checked) { foreach(QList<Histogram *> group, H) foreach(Histogram *h, group) h->plot->setInteractions(checked ? QCP::iRangeDrag | QCP::iRangeZoom : QCP::iNone); }
    void on_radioFullRange_toggled() { if (ui->radioFullRange->isChecked()) { foreach(Histogram *h, H[curType]) updatePlot(h); } }
    void on_radioThreshold_toggled() { if (ui->radioThreshold->isChecked()) { foreach(Histogram *h, H[curType]) updatePlot(h); } }
    void on_spinBoxThreshold_valueChanged(double d) { threshold = d; on_radioThreshold_toggled(); }
    void on_radioLogScale_toggled() {
        lg = ui->radioLogScale->isChecked();
        foreach(QList<Histogram *> group, H) foreach(Histogram *h, group) {
            h->plot->yAxis->setScaleType(lg ? QCPAxis::stLogarithmic : QCPAxis::stLinear);
            h->plot->yAxis->setTicker(lg ? logTicker : linTicker);
        }
        foreach(Histogram *h, H[curType]) updatePlot(h);
    }
    void on_radioADC0_clicked() {
        foreach(Histogram *h, H[hAmpl]) {
            h->type = hADC0;
            h->bars[1]->moveAbove(h->bars[0]);
            h->bars[0]->setVisible(1);
            h->bars[1]->setVisible(0);
            updatePlot(h);
        }
    };
    void on_radioADC1_clicked() {
        foreach(Histogram *h, H[hAmpl]) {
            h->type = hADC1;
            h->bars[0]->moveAbove(h->bars[1]);
            h->bars[0]->setVisible(0);
            h->bars[1]->setVisible(1);
            updatePlot(h);
        }
    };
    void on_radioADCb_clicked() {
        foreach(Histogram *h, H[hAmpl]) {
            h->type = hAmpl;
            h->bars[1]->moveAbove(h->bars[0]);
            h->bars[1]->setStackingGap(0);
            h->bars[0]->setVisible(1);
            h->bars[1]->setVisible(1);
            updatePlot(h);
        }
    };
    void on_tabWidget_currentChanged(int index) {
        curType = TypeOfHistogram(index); //warning: tab indexes should correspond to histogram types for this to work
        ui->groupBoxADC        ->setVisible(index == hAmpl);
        ui->groupBoxChargeUnits->setVisible(index == hAmpl);
        ui->groupBoxTimeUnits  ->setVisible(index == hTime);
        max = curType == hTrig ? &maxTCM : &maxPM;
        ui->labelValueMax->setText(QString::asprintf("%u", *max));
        ui->spinBoxMax      ->setMaximum(curType == hTrig ? UINT32_MAX : UINT16_MAX);
        ui->spinBoxThreshold->setMaximum(curType == hTrig ? UINT32_MAX : UINT16_MAX);
        checkBoardSelection();
        axisLength_px = H[curType].first()->plot->width() - 2 * fontSize_px;
//        foreach(Histogram *h, H[curType]) updatePlot(h);
        foreach(Histogram *h, H[curType]) h->plot->replot();
    }
    void changeUnit(QList<MainWindow::Histogram *> &group, double newBinWidth) {
        bool defaultBin = newBinWidth == 1.;
        QMargins m = group.first()->plot->axisRect()->margins(); m.setBottom((defaultBin ? 2.2 : 1.) * fontSize_px);
        foreach(Histogram *h, group) {
            QCPRange newRange = h->plot->xAxis->range() * newBinWidth/h->bars[0]->width();
            foreach(QCPBars *b, h->bars) b->setWidth(newBinWidth);
            h->plot->xAxis->setTicker(defaultBin ? binTicker : floatTicker);
            h->plot->xAxis->setTickLabelRotation(defaultBin ? -90 : 0);
            h->plot->xAxis->setTickLength(0, defaultBin ? 0 : 1);
            h->plot->axisRect()->setMargins(m);
            refillPlot(h);
            if (ui->radioAdjustable->isChecked()) h->plot->xAxis->setRange(newRange); //this will also update plot
            else updatePlot(h);
        }
    }
    void on_radButTDC_toggled(bool checked) { if (checked) changeUnit(H[hTime],         1.); }
    void on_radBut_ns_toggled(bool checked) { if (checked) changeUnit(H[hTime], TDCunit_ns); }
    void on_radButADC_toggled(bool checked) { if (checked) changeUnit(H[hAmpl],       1.             ); ui->buttonTune->setVisible(!checked); }
    void on_radBut_mV_toggled(bool checked) { if (checked) changeUnit(H[hAmpl], mVperMIP / ADCUperMIP); }
    void on_radButMIP_toggled(bool checked) { if (checked) changeUnit(H[hAmpl],       1. / ADCUperMIP); }
    void on_buttonTune_clicked() {
        if (ui->radBut_mV->isChecked()) { mVperMIP   = QInputDialog::getDouble(this,      "MIP/mV ratio",        "Set amount of mV per MIP",   mVperMIP, 1., 100., 1); on_radBut_mV_toggled(true); }
        else                            { ADCUperMIP = QInputDialog::getDouble(this, "MIP/ADCunit ratio", "Set amount of ADC units per MIP", ADCUperMIP, 1., 100., 0); on_radButMIP_toggled(true); }
    }
};

#endif // MAINWINDOW_H
