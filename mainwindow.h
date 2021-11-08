#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "FITelectronics.h"
#include "switch.h"
#include <QMainWindow>
#include <QtWidgets>
#include <QDateTime>

#include "qcustomplot.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    FITelectronics FEE;
    QSettings settings;
    QActionGroup *controlsGroup;
    QPixmap
        Green0 = QPixmap(":/0G.png"), //OK
        Green1 = QPixmap(":/1G.png"), //OK
        Red0 = QPixmap(":/0R.png"), //not OK
        Red1 = QPixmap(":/1R.png"); //not OK
    QList<QWidget *> allWidgets;
    QRegExp validIPaddressRE {"([1-9][0-9]?|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.(([1-9]?[0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){2}([1-9][0-9]?|1[0-9][0-9]|2[0-4][0-9]|25[0-5])"};
    bool ok;
    int sz;

public:

    explicit MainWindow(QWidget *parent = nullptr):
        QMainWindow(parent),
        settings(QCoreApplication::applicationName() + ".ini", QSettings::IniFormat),
        ui(new Ui::MainWindow)
    {
        ui->setupUi(this);

//initialization
        setWindowTitle(QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion());
        allWidgets = ui->centralwidget->findChildren<QWidget *>();

//initial scaling (a label with fontsize 10 (Calibri) has pixelsize of 13 without system scaling and e.g. 20 at 150% scaling so widgets size should be recalculated)
        sz = ui->label_trigger->fontInfo().pixelSize();
        resize(lround(width()*sz/13.), lround(height()*sz/13.)); //mainWindow
        setMinimumSize(size());
        if (sz > 13) { //not default pixelsize for font of size 10
            foreach (QWidget *w, allWidgets) w->setGeometry(lround(w->x()*sz/13.), lround(w->y()*sz/13.), lround(w->width()*sz/13.), lround(w->height()*sz/13.));
        }

//menus
        QMenu *networkMenu = menuBar()->addMenu("&Network");
        networkMenu->addAction(QIcon(":/recheck.png"), "&Recheck and default", this, SLOT(recheckTarget()), QKeySequence::Refresh);
        networkMenu->addAction("&Change target IP address...", this, SLOT(changeIP()));
//        networkMenu->addAction("Test qd", &FEE, &FITelectronics::testSpeed);

//signal-slot conections
        connect(&FEE, &IPbusTarget::error, this, [=](QString message, errorType et) {
            QMessageBox::warning(this, errorTypeName[et], message);
            statusBar()->showMessage(message);
            ui->centralwidget->setDisabled(true);
        });
        connect(&FEE, &IPbusTarget::noResponse, this, [=](QString message) {
            statusBar()->showMessage(statusBar()->currentMessage() == "" ? FEE.IPaddress + ": " + message : "");
            ui->centralwidget->setDisabled(true);
        });
        connect(&FEE, &FITelectronics::linksStatusReady, this, [=](quint32 mask) {
            ui->comboBox->clear();
            if (mask == 0) {
                ui->comboBox->addItem("N/A");
                FEE.updateTimer->stop();
                statusBar()->showMessage(FEE.IPaddress + ": no PM available");
                ui->centralwidget->setDisabled(true);
            } else {
                for (quint8 i=0; i<10; ++i, mask >>= 1) if (mask & 1) ui->comboBox->addItem(QString("A") + QChar('0' + i));
                for (quint8 i=0; i<10; ++i, mask >>= 1) if (mask & 1) ui->comboBox->addItem(QString("C") + QChar('0' + i));
                on_comboBox_activated(ui->comboBox->itemText(0));
            }
            ui->comboBox->setCurrentIndex(0);
        });
        connect(&FEE, &FITelectronics::statusReady, this, &MainWindow::updateActualValues);

        QString IPaddress = settings.value("IPaddress", FEE.IPaddress).toString();
        if (validIPaddressRE.exactMatch(IPaddress)) FEE.IPaddress = IPaddress;

// setup histograms and canvases
        InitHistograms();
        SetupView();

// histograms connection
        for(quint8 i=0; i<12; i++) {
            connect(chargeHist[i],&QCustomPlot::mouseDoubleClick,this,&MainWindow::hist_double_clicked);
            connect(timeHist[i],  &QCustomPlot::mouseDoubleClick,this,&MainWindow::hist_double_clicked);

            connect(ui->radio_0,&QRadioButton::clicked,this,[&](){
                for(quint8 i=0;i<12; i++) {
                    chargeBars[i]->setVisible(1);
                    chargeBars1[i]->setVisible(0);
                }
            });
            connect(ui->radio_0,&QRadioButton::clicked,this,&MainWindow::on_button_update_clicked);

            connect(ui->radio_1,&QRadioButton::clicked,this,[&](){
                for(quint8 i=0;i<12; i++) {
                    chargeBars1[i]->moveAbove(0);
                    chargeBars[i]->setVisible(0);
                    chargeBars1[i]->setVisible(1);
                }
            });
            connect(ui->radio_1,&QRadioButton::clicked,this,&MainWindow::on_button_update_clicked);

            connect(ui->radio_01,&QRadioButton::clicked,this,[&](){
                for(quint8 i=0;i<12; i++) {
                    chargeBars[i]->setVisible(1);
                    chargeBars1[i]->setVisible(1);
                    chargeBars1[i]->moveAbove(chargeBars[i]);
                }
            });
            connect(ui->radio_01,&QRadioButton::clicked,this,&MainWindow::on_button_update_clicked);
            connect(ui->radio_0,&QRadioButton::clicked,this,&MainWindow::on_button_update_clicked);
            connect(ui->radio_1,&QRadioButton::clicked,this,&MainWindow::on_button_update_clicked);

            connect(chargeHist[i]->xAxis,
                    static_cast<void (QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged),
                    this,&MainWindow::auto_rescale);

            connect(timeHist[i]->xAxis,
                    static_cast<void (QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged),
                    this,&MainWindow::auto_rescale);
        }

        FEE.reconnect();
    }

    ~MainWindow() {
        settings.setValue("IPaddress", FEE.IPaddress);

        for(quint8 i=0; i<12; i++) {
            chargeHist[i]->clearPlottables();
            timeHist[i]->clearPlottables();

            delete chargeHist[i];
            delete timeHist[i];
        }
        delete ui;
    }

    bool writeFiles() {
        QTextStream out;
        QFile ft("HistogramsTime.csv");
        if (ft.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
            out.setDevice(&ft);
            out << " bin ";
            for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":Ch%02dT", iCh + 1);
            out << Qt::endl;
            for (int iBin=-2048; iBin < 2048; ++iBin) {
                out << QString::asprintf("%5d", iBin);
                for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":%5d", FEE.data.Ch[iCh].time[iBin & 0xFFF]);
                out << Qt::endl;
            }
            ft.close();
        } else return false;
        QFile fa("HistogramsAmpl.csv");
        if (fa.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
            out.setDevice(&fa);
            out << " bin";
            for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":C%02dA0:C%02dA1", iCh + 1, iCh + 1);
            out << Qt::endl;
            for (int iBin=-256; iBin <    0; ++iBin) {
                out << QString::asprintf("%4d", iBin);
                for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":%5d:%5d", FEE.data.Ch[iCh].nADC0[-iBin - 1], FEE.data.Ch[iCh].nADC1[-iBin - 1]);
                out << Qt::endl;
            }
            for (int iBin=   0; iBin < 4096; ++iBin) {
                out << QString::asprintf("%4d", iBin);
                for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":%5d:%5d", FEE.data.Ch[iCh].pADC0[iBin], FEE.data.Ch[iCh].pADC1[iBin]);
                out << Qt::endl;
            }
            fa.close();
        } else return false;
        return true;
    }


    void InitHistograms(){
        chargeHist[0] = ui->ccharge01;
        chargeHist[1] = ui->ccharge02;
        chargeHist[2] = ui->ccharge03;
        chargeHist[3] = ui->ccharge04;
        chargeHist[4] = ui->ccharge05;
        chargeHist[5] = ui->ccharge06;
        chargeHist[6] = ui->ccharge07;
        chargeHist[7] = ui->ccharge08;
        chargeHist[8] = ui->ccharge09;
        chargeHist[9] = ui->ccharge10;
        chargeHist[10] = ui->ccharge11;
        chargeHist[11] = ui->ccharge12;

        timeHist[0] = ui->ctime01;
        timeHist[1] = ui->ctime02;
        timeHist[2] = ui->ctime03;
        timeHist[3] = ui->ctime04;
        timeHist[4] = ui->ctime05;
        timeHist[5] = ui->ctime06;
        timeHist[6] = ui->ctime07;
        timeHist[7] = ui->ctime08;
        timeHist[8] = ui->ctime09;
        timeHist[9] = ui->ctime10;
        timeHist[10] = ui->ctime11;
        timeHist[11] = ui->ctime12;

        //timeHist[1]->setLayout(new QGridLayout());
        QLabel* lbl = new QLabel(ui->lbl_Mean);
        ((QGridLayout*)timeHist[1]->layout())->addWidget(lbl,0,0);
        lbl->show();


        for(quint16 i=0; i<12; i++){
            chargeBars[i] = new QCPBars(chargeHist[i]->xAxis,chargeHist[i]->yAxis);
            chargeBars1[i] = new QCPBars(chargeHist[i]->xAxis,chargeHist[i]->yAxis);
            timeBars[i] = new QCPBars(timeHist[i]->xAxis,timeHist[i]->yAxis);
            chargeBars1[i]->moveAbove(chargeBars[i]);
        }
    }

    void SetupView(){
        QSharedPointer<QCPAxisTickerFixed> fixedTicker(new QCPAxisTickerFixed);
        fixedTicker->setTickStep(1.0);
        fixedTicker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);

        QPen pen;
        pen.setColor(QColor(Qt::darkBlue));
        pen.setColor(QColor(QColor(50,0,0)));

        for(quint8 i=0; i<12; i++) {
            QCPItemText *chargeLabel = new QCPItemText(chargeHist[i]);
            chargeLabel->setLayer("axes");
            chargeLabel->setPositionAlignment(Qt::AlignVCenter|Qt::AlignLeft);
            chargeLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
            chargeLabel->position->setCoords(1.03,1); // place position at center/top of axis rect
            chargeLabel->setRotation(270);
            chargeLabel->setClipToAxisRect(0);
            chargeLabel->setText("Charge (ADC units)");
            chargeLabel->setFont(QFont(font().family(),10)); // make font a bit larger
            chargeLabel->setBrush(Qt::white);

            QCPItemText *timeLabel = new QCPItemText(timeHist[i]);
            timeLabel->setLayer("axes");
            timeLabel->setPositionAlignment(Qt::AlignVCenter|Qt::AlignLeft);
            timeLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
            timeLabel->position->setCoords(1.03,1); // place position at center/top of axis rect
            timeLabel->setRotation(270);
            timeLabel->setClipToAxisRect(0);
            timeLabel->setText("Time (TDC units)");
            timeLabel->setFont(QFont(font().family(),10)); // make font a bit larger
            timeLabel->setBrush(Qt::white);

            QCPItemText *chCountsLabel = new QCPItemText(chargeHist[i]);
            chCountsLabel->setLayer("axes");
            chCountsLabel->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
            chCountsLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
            chCountsLabel->position->setCoords(0,0); // place position at center/top of axis rect
            chCountsLabel->setRotation(0);
            chCountsLabel->setClipToAxisRect(0);
            chCountsLabel->setText("Counts");
            chCountsLabel->setFont(QFont(font().family(),10)); // make font a bit larger
            chCountsLabel->setBrush(Qt::white);

            QCPItemText *tCountsLabel = new QCPItemText(timeHist[i]);
            tCountsLabel->setLayer("axes");
            tCountsLabel->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
            tCountsLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
            tCountsLabel->position->setCoords(0,0); // place position at center/top of axis rect
            tCountsLabel->setRotation(0);
            tCountsLabel->setClipToAxisRect(0);
            tCountsLabel->setText("Counts");
            tCountsLabel->setFont(QFont(font().family(),10)); // make font a bit larger
            tCountsLabel->setBrush(Qt::white);

            chargeHist[i]->yAxis->setTicker(fixedTicker);
            chargeHist[i]->xAxis->setTicker(fixedTicker);
            chargeHist[i]->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
            chargeHist[i]->axisRect()->setRangeDrag(Qt::Horizontal);
            chargeHist[i]->axisRect()->setRangeZoom(Qt::Horizontal);
            chargeHist[i]->axisRect()->setBackgroundScaled(false);

            chargeBars[i]->setPen(pen);
            chargeBars[i]->setBrush(QColor(QColor(100,0,0)));
            chargeBars1[i]->setPen(pen);
            chargeBars1[i]->setBrush(QColor(QColor(200,0,0)));
            chargeBars[i]->setWidth(1);
            chargeBars1[i]->setWidth(1);

            chargeBars[i]->setStackingGap(0);
            chargeBars1[i]->setStackingGap(0);
            timeBars[i]->setStackingGap(0);

            timeHist[i]->yAxis->setTicker(fixedTicker);
            timeHist[i]->xAxis->setTicker(fixedTicker);
            timeHist[i]->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
            timeHist[i]->axisRect()->setRangeDrag(Qt::Horizontal);
            timeHist[i]->axisRect()->setRangeZoom(Qt::Horizontal);

            timeBars[i]->setPen(pen);
            timeBars[i]->setBrush(QColor(QColor(100,0,0)));
            timeBars[i]->setWidth(1);

            chargeHist[i]->axisRect()->setBackground(QBrush(QColor(0,0,0,30)));
            timeHist[i]->axisRect()->setBackground(QBrush(QColor(0,0,0,30)));

            chargeHist[i]->xAxis->grid()->setPen(QPen(QColor(140, 140, 140,80), 1));
            chargeHist[i]->yAxis->grid()->setPen(QPen(QColor(140, 140, 140,80), 1));
            chargeHist[i]->xAxis->grid()->setZeroLinePen(Qt::NoPen);
            chargeHist[i]->yAxis->grid()->setZeroLinePen(Qt::NoPen);

            timeHist[i]->xAxis->grid()->setPen(QPen(QColor(140, 140, 140,80), 1));
            timeHist[i]->yAxis->grid()->setPen(QPen(QColor(140, 140, 140,80), 1));
            timeHist[i]->xAxis->grid()->setZeroLinePen(Qt::NoPen);
            timeHist[i]->yAxis->grid()->setZeroLinePen(Qt::NoPen);

            chargeHist[i]->xAxis->setRange(-256,4095);
            timeHist[i]->xAxis->setRange(-2048,2047);
        }
    }


    void HideZeroBars(){
        if(ui->doFixRanges->isChecked()) return;

        for(quint8 i=0; i<12; i++) {
            if(ui->radio_01->isChecked()){
                qint16 left,right,left1,right1;
                left=right=left1=right1=0;
                if(chargeBars[i]->data()->size()>0){
                    left = chargeBars[i]->data()->at(0)->key;
                    right = chargeBars[i]->data()->at(chargeBars[i]->data()->size()-1)->key;
                }
                if(chargeBars1[i]->data()->size()>0){
                    left1 = chargeBars1[i]->data()->at(0)->key;
                    right1 = chargeBars1[i]->data()->at(chargeBars1[i]->data()->size()-1)->key;
                }
//                qDebug() << left << right << left1 << right1;

                if(left != right && left1!= right1) chargeHist[i]->xAxis->setRange(qMin(left,left1)-chargeBars[i]->width(),qMax(right,right1)+chargeBars[i]->width());
                else if(left==right) chargeHist[i]->xAxis->setRange(left1-chargeBars[i]->width(),right1+chargeBars[i]->width());
                else chargeHist[i]->xAxis->setRange(left-chargeBars[i]->width(),right+chargeBars[i]->width());

                left=right=left1=right1=0;

                if(timeBars[i]->data()->size()>0){
                    left = timeBars[i]->data()->at(0)->key;
                    right = timeBars[i]->data()->at(timeBars[i]->data()->size()-1)->key;
                }
                if(left != right && left1!= right1) timeHist[i]->xAxis->setRange(qMin(left,left1)-timeBars[i]->width(),qMax(right,right1)+timeBars[i]->width());
                else if(left==right) timeHist[i]->xAxis->setRange(left1-timeBars[i]->width(),right1+timeBars[i]->width());
                else timeHist[i]->xAxis->setRange(left-timeBars[i]->width(),right+timeBars[i]->width());
            }
            else if(ui->radio_0->isChecked()){
                qint16 left,right;
                left=right=0;
                if(chargeBars[i]->data()->size()>0){
                    left = chargeBars[i]->data()->at(0)->key;
                    right = chargeBars[i]->data()->at(chargeBars[i]->data()->size()-1)->key;
                }
                chargeHist[i]->xAxis->setRange(left-chargeBars[i]->width(),right+chargeBars[i]->width());
                left=right=0;
                if(timeBars[i]->data()->size()>0){
                    left = timeBars[i]->data()->at(0)->key;
                    right = timeBars[i]->data()->at(timeBars[i]->data()->size()-1)->key;
                }
                timeHist[i]->xAxis->setRange(left-timeBars[i]->width(),right+timeBars[i]->width());

            }
            else if(ui->radio_1->isChecked()){
                qint16 left,right;
                left=right=0;
                if(chargeBars1[i]->data()->size()>0){
                    left = chargeBars1[i]->data()->at(0)->key;
                    right = chargeBars1[i]->data()->at(chargeBars1[i]->data()->size()-1)->key;
                }
                chargeHist[i]->xAxis->setRange(left-chargeBars1[i]->width(),right+chargeBars1[i]->width());
                left=right=0;
                if(timeBars[i]->data()->size()>0){
                    left = timeBars[i]->data()->at(0)->key;
                    right = timeBars[i]->data()->at(timeBars[i]->data()->size()-1)->key;
                }
                timeHist[i]->xAxis->setRange(left-timeBars[i]->width(),right+timeBars[i]->width());
            }

            chargeHist[i]->replot();
            timeHist[i]->replot();
        }
    }

    void ShowFullRange(){
        if(ui->doFixRanges->isChecked()) return;
        for(quint8 i=0; i<12; i++) {
            chargeHist[i]->xAxis->setRange(-256,4095);
            chargeHist[i]->replot();
            timeHist[i]->xAxis->setRange(-2048,2047);
            timeHist[i]->replot();
        }
    }

    void auto_rescale(const QCPRange &newRange){
        for(quint8 i=0; i<12; i++){
            bool fr=false;
            quint16 upper = 0;
            quint16 upper1 = 0;

            if(sender() == chargeHist[i]->xAxis){
                if(newRange.upper > 4095) chargeHist[i]->xAxis->setRangeUpper(4095);
                if(newRange.lower < -256) chargeHist[i]->xAxis->setRangeLower(-256);

                upper=chargeBars[i]->data().data()->valueRange(fr,QCP::sdBoth,newRange).upper;
                upper1=chargeBars1[i]->data().data()->valueRange(fr,QCP::sdBoth,newRange).upper;
                if(ui->radio_01->isChecked()){
                    if((upper+upper1) > chargeHist[i]->yAxis->range().upper){
                        chargeHist[i]->yAxis->rescale();
                        chargeHist[i]->yAxis->setRange(0,chargeHist[i]->yAxis->range().upper*1.1);
                    } else chargeHist[i]->yAxis->setRange(0,(upper+upper1)*1.1);
                }
                else if(ui->radio_1->isChecked())  chargeHist[i]->yAxis->setRange(0,upper1*1.1);
                else if(ui->radio_0->isChecked())  chargeHist[i]->yAxis->setRange(0,upper*1.1);
            }
            else if(sender() == timeHist[i]->xAxis){
                if(newRange.upper > 2047) timeHist[i]->xAxis->setRangeUpper(2047);
                if(newRange.lower < -2048) timeHist[i]->xAxis->setRangeLower(-2048);

                upper=timeBars[i]->data().data()->valueRange(fr,QCP::sdBoth,newRange).upper;
                    if(upper > timeHist[i]->yAxis->range().upper){
                        timeHist[i]->yAxis->rescale();
                        timeHist[i]->yAxis->setRange(0,timeHist[i]->yAxis->range().upper*1.1);
                    } else timeHist[i]->yAxis->setRange(0,upper*1.1);
            }
        }
    }

    void hist_double_clicked( QMouseEvent * event )
    {
//        if(ui->doFixRanges->isChecked()) return;

        QCustomPlot* hist = static_cast<QCustomPlot*>( QObject::sender() );

        if(event->buttons() & Qt::RightButton){
            hist->xAxis->setRangeUpper(qFloor(hist->xAxis->pixelToCoord(event->x())));
            hist->replot();
        }
        else if(event->buttons() & Qt::LeftButton){
            hist->xAxis->setRangeLower(qFloor(hist->xAxis->pixelToCoord(event->x())));
            hist->replot();
        }
    }



public slots:
    void recheckTarget() {
        statusBar()->showMessage(FEE.IPaddress + ": status requested...");
        FEE.reconnect();
    }

    void changeIP() {
        QString text = QInputDialog::getText(this, "Changing target", "Enter new target's IP address", QLineEdit::Normal, FEE.IPaddress, &ok);
        if (ok && !text.isEmpty()) {
            if (validIPaddressRE.exactMatch(text)) {
                FEE.IPaddress = text;
                FEE.reconnect();
            } else QMessageBox::warning(this, "Warning", text + ": invalid IP address. Continue with previous target");
        }
    }

    void updateActualValues() {
        ui->label_icon_trigger->setPixmap(FEE.triggerLinkStatus.linkOK ? Green1 : Red0);
        ui->label_icon_clock->setPixmap(FEE.boardStatus.PLLlocked == 0b1111 && FEE.boardStatus.syncError == 0 ? Green1 : Red0);
        ui->centralwidget->setDisabled(FEE.boardStatus.resetting);
        if (FEE.boardStatus.resetting)
            statusBar()->showMessage("PM is resetting");
        else
            statusBar()->showMessage(statusBar()->currentMessage() == "" ? FEE.IPaddress + ": online" : "");
        ui->switch_historgamming->setChecked(FEE.histStatus.histOn);
        ui->switch_filter->setChecked(FEE.histStatus.filterOn);
        if (ui->spinBox->value() != FEE.histStatus.BCID) ui->spinBox->setValue(FEE.histStatus.BCID);
        ui->label_historgamming->setToolTip(QString::number(FEE.curAddress));
        if (ui->doRepeat->isChecked()) on_button_read_clicked();
    }


private slots:
    void on_comboBox_activated(const QString &PMname) {
        quint8 n = PMname.rightRef(1).toUInt();
        if (PMname[0] == 'C') n += 10;
        if (n < 20 && FEE.isOnline) {
            FEE.curPM = n;
            FEE.sync();
        }
    }

    QVector<qint16> getRanges(bool isTime, bool isUpper)
    {
        QVector<qint16> res;
        res.reserve(12);
        if(isTime) {
            res.push_back(isUpper? ui->ctime01->xAxis->range().upper : ui->ctime01->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime02->xAxis->range().upper : ui->ctime02->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime03->xAxis->range().upper : ui->ctime03->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime04->xAxis->range().upper : ui->ctime04->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime05->xAxis->range().upper : ui->ctime05->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime06->xAxis->range().upper : ui->ctime06->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime07->xAxis->range().upper : ui->ctime07->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime08->xAxis->range().upper : ui->ctime08->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime09->xAxis->range().upper : ui->ctime09->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime10->xAxis->range().upper : ui->ctime10->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime11->xAxis->range().upper : ui->ctime11->xAxis->range().lower);
            res.push_back(isUpper? ui->ctime12->xAxis->range().upper : ui->ctime12->xAxis->range().lower);
        } else {
            res.push_back(isUpper? ui->ccharge01->xAxis->range().upper : ui->ccharge01->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge02->xAxis->range().upper : ui->ccharge02->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge03->xAxis->range().upper : ui->ccharge03->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge04->xAxis->range().upper : ui->ccharge04->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge05->xAxis->range().upper : ui->ccharge05->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge06->xAxis->range().upper : ui->ccharge06->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge07->xAxis->range().upper : ui->ccharge07->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge08->xAxis->range().upper : ui->ccharge08->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge09->xAxis->range().upper : ui->ccharge09->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge10->xAxis->range().upper : ui->ccharge10->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge11->xAxis->range().upper : ui->ccharge11->xAxis->range().lower);
            res.push_back(isUpper? ui->ccharge12->xAxis->range().upper : ui->ccharge12->xAxis->range().lower);
        }
        return res;
    };

    void on_switch_historgamming_clicked(bool checked) { FEE.switchHist(!checked); }
    void on_switch_filter_clicked(bool checked) { FEE.switchFilter(!checked); }

    void on_spinBox_valueChanged(int BCID) { FEE.setBCID(BCID); }

    void on_button_reset_clicked() { FEE.reset(); }

    void on_button_read_clicked() {
        on_button_clear_screen_clicked();

        start = QDateTime::currentDateTime();
        quint32 n = FEE.readHistograms();
        if (n != datasize) {
            statusBar()->showMessage(QString::asprintf( "%d/%d words read (%.1f%% of data)", n, datasize, n*100./datasize ));
        } else {
            statusBar()->showMessage(QString::asprintf( "Data read in %.3f s", start.msecsTo(QDateTime::currentDateTime())/1000. ));
            start = QDateTime::currentDateTime();
            quint16 max = 0, *b = (quint16 *)&FEE.data, *e = b + datasize;
            for (quint16 *p=b; p<e; ++p) { if (*p > max) max = *p; }
            statusBar()->showMessage(statusBar()->currentMessage() + ", max: " + QString::number(max));

            // Fill histograms view
            for(quint8 i=0; i<12; i++){
                for(quint16 k=0; k<256; k++){
                    if(FEE.data.Ch[i].nADC0[255-k])
                        chargeBars[i]->data().data()->add(QCPBarsData(-256+k,FEE.data.Ch[i].nADC0[255-k]));
                    if(FEE.data.Ch[i].nADC1[255-k])
                        chargeBars1[i]->data().data()->add(QCPBarsData(-256+k,FEE.data.Ch[i].nADC1[255-k]));
                }
                for(qint32 k=0; k<4095; k++){
                    if(FEE.data.Ch[i].pADC0[k])
                        chargeBars[i]->data().data()->add(QCPBarsData(k,FEE.data.Ch[i].pADC0[k]));
                    if(FEE.data.Ch[i].pADC1[k])
                        chargeBars1[i]->data().data()->add(QCPBarsData(k,FEE.data.Ch[i].pADC1[k]));
                    if(FEE.data.Ch[i].time[(k-2048) & 0xFFF])
                        timeBars[i]->data().data()->add(QCPBarsData(-2048+k,FEE.data.Ch[i].time[(k-2048) & 0xFFF]));
                }
            }
            if(ui->doAutoReset->isChecked() && max >= ui->spin_auto_reset->value()) {
                FEE.reset();
            }
        }

        FEE.calcStats( getRanges(1,0).data(),getRanges(1,1).data()
                      ,getRanges(0,0).data(),getRanges(0,1).data());
        update_stats_labels();
        on_button_update_clicked();
    }

    void on_button_hide_show_zero_clicked(bool checked){
        if(checked){
            ui->button_hide_show_zero->setText("Show zero bars");
            HideZeroBars();
        } else {
            ui->button_hide_show_zero->setText("Hide zero bars");
            ShowFullRange();
        }
    }

    void on_button_update_clicked(){
        for(quint8 i=0; i<12; i++){
            chargeHist[i]->yAxis->rescale(1);
            chargeHist[i]->yAxis->setRange(0,chargeHist[i]->yAxis->range().upper*1.1);
            timeHist[i]->yAxis->rescale(1);
            timeHist[i]->yAxis->setRange(0,timeHist[i]->yAxis->range().upper*1.1);
            chargeHist[i]->replot();
            timeHist[i]->replot();
        }
    }

    void on_button_clear_screen_clicked(){
        for(quint8 i=0; i<12; i++) {
            chargeBars[i]->data().data()->clear();
            chargeBars1[i]->data().data()->clear();
            timeBars[i]->data().data()->clear();
        }
    }

    void on_button_read_file_clicked(){

        QFile f1("HistogramsTime.csv");
        if (!f1.open(QFile::ReadOnly | QFile::Text )) {
            this->statusBar()->showMessage("Can't open file for read");
        }

        QStringList wordList;
        QString line;
        for (quint32 j=0; !f1.atEnd(); j++) {
            line = f1.readLine();
            if(j==0) continue;
            wordList = line.split(':');
            for(quint8 i=0; i<12; i++){
                if(wordList[i+1].toInt() !=0 ) {
                    timeBars[i]->data().data()->add(QCPBarsData(wordList[0].toInt(),wordList[i+1].toInt()));
                }
            }
        }
        f1.close();

        QFile f2("HistogramsAmpl.csv");
        if (!f2.open(QFile::ReadOnly | QFile::Text )) {
            this->statusBar()->showMessage("Can't open file for read");
        }

        for (quint32 j=0; !f2.atEnd(); j++) {
            line = f2.readLine();
            if(j==0) continue;
            wordList = line.split(':');
            for(quint8 i=0; i<12; i++){
                if(wordList[2*i+1].toInt() !=0 ) chargeBars[i]->data().data()->add(QCPBarsData(wordList[0].toInt(),wordList[2*i+1].toInt()));
                if(wordList[2*i+2].toInt() !=0 ) chargeBars1[i]->data().data()->add(QCPBarsData(wordList[0].toInt(),wordList[2*i+2].toInt()));
            }
        }

        f2.close();

        ui->button_update->clicked();
    }

    void on_button_save_clicked(){
        start = QDateTime::currentDateTime();
        end = QDateTime::currentDateTime();
        statusBar()->showMessage(QString::asprintf("Data:  %.3fs", start.msecsTo(end)/1000.));
        start = end;
        bool success = writeFiles();
        end = QDateTime::currentDateTime();
        if (success) statusBar()->showMessage(statusBar()->currentMessage() + QString::asprintf(", files:  %.3fs", start.msecsTo(end)/1000.));
        start = end;
        quint16 max = 0, *b = (quint16 *)&FEE.data, *e = b + datasize;
        for (quint16 *p=b; p<e; ++p) if (*p > max) max = *p;
        FEE.calcStats( getRanges(1,0).data(),getRanges(1,1).data()
                      ,getRanges(0,0).data(),getRanges(0,1).data());
        end = QDateTime::currentDateTime();
        statusBar()->showMessage(statusBar()->currentMessage() + QString::asprintf(", stats: %.3fs, max: %d", start.msecsTo(end)/1000., max));
//        qDebug() << QString::asprintf("Ch |Time: sum    mean    RMS |ADC0: sum   mean    RMS |ADC1: sum   mean    RMS |Ampl: sum   mean    RMS");
////          qDebug() << QString::asprintf("00 |268431360 -2048.0 2047.0 |285208320 4096.0 2176.0 |285208320 4096.0 2176.0 |570416640 4096.0 2176.0");
//        for (quint8 iCh=0; iCh<12; ++iCh) {
//            qDebug() << QString::asprintf("%02d |%9d % 7.1f %6.1f |%9d %6.1f %6.1f |%9d %6.1f %6.1f |%9d %6.1f %6.1f", iCh + 1,
//                FEE.statsCh[iCh][hTime].sum, FEE.statsCh[iCh][hTime].mean, FEE.statsCh[iCh][hTime].RMS,
//                FEE.statsCh[iCh][hADC0].sum, FEE.statsCh[iCh][hADC0].mean, FEE.statsCh[iCh][hADC0].RMS,
//                FEE.statsCh[iCh][hADC1].sum, FEE.statsCh[iCh][hADC1].mean, FEE.statsCh[iCh][hADC1].RMS,
//                FEE.statsCh[iCh][hAmpl].sum, FEE.statsCh[iCh][hAmpl].mean, FEE.statsCh[iCh][hAmpl].RMS
//            );
//        }
/*
        QTextStream out;
        QFile ft("HistogramsTime.csv");
        if (ft.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
            out.setDevice(&ft);
            out << " bin ";
            for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":Ch%02dT", iCh + 1);
            out << Qt::endl;
            for (int iBin=-2048; iBin < 2048; ++iBin) {
                out << QString::asprintf("%5d", iBin);
                for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":%5d", FEE.data.Ch[iCh].time[iBin & 0xFFF]);
                out << Qt::endl;
            }
            ft.close();
        } else return;
        QFile fa("HistogramsAmpl.csv");
        if (fa.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
            out.setDevice(&fa);
            out << " bin";
            for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":C%02dA0:C%02dA1", iCh + 1, iCh + 1);
            out << Qt::endl;
            for (int iBin=-256; iBin <    0; ++iBin) {
                out << QString::asprintf("%4d", iBin);
                for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":%5d:%5d", FEE.data.Ch[iCh].nADC0[-iBin - 1], FEE.data.Ch[iCh].nADC1[-iBin - 1]);
                out << Qt::endl;
            }
            for (int iBin=   0; iBin < 4096; ++iBin) {
                out << QString::asprintf("%4d", iBin);
                for (int iCh=0; iCh<12; ++iCh) out << QString::asprintf(":%5d:%5d", FEE.data.Ch[iCh].pADC0[iBin], FEE.data.Ch[iCh].pADC1[iBin]);
                out << Qt::endl;
            }
            fa.close();
        } else return;
        statusBar()->showMessage(statusBar()->currentMessage() + QString::asprintf(", dumped to file in %.3f s", start.msecsTo(QDateTime::currentDateTime())/1000.));
 */
    }

    void on_pushButton_clicked(bool checked){
        if(checked) {
            ui->pushButton->setText("to linear scale");
            for(quint8 i=0; i<12; i++) {
                QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);

                chargeHist[i]->yAxis->grid()->setSubGridVisible(true);
                chargeHist[i]->xAxis->grid()->setSubGridVisible(true);
                chargeHist[i]->yAxis->setScaleType(QCPAxis::stLogarithmic);
                chargeHist[i]->yAxis->setTicker(logTicker);

                timeHist[i]->yAxis->grid()->setSubGridVisible(true);
                timeHist[i]->xAxis->grid()->setSubGridVisible(true);
                timeHist[i]->yAxis->setScaleType(QCPAxis::stLogarithmic);
                timeHist[i]->yAxis->setTicker(logTicker);
            }
        } else {
            ui->pushButton->setText("to log scale");

            for(quint8 i=0; i<12; i++) {
                QSharedPointer<QCPAxisTicker> linTicker(new QCPAxisTicker);

                chargeHist[i]->yAxis->grid()->setSubGridVisible(true);
                chargeHist[i]->xAxis->grid()->setSubGridVisible(true);
                chargeHist[i]->yAxis->setScaleType(QCPAxis::stLinear);
                chargeHist[i]->yAxis->setTicker(linTicker);

                timeHist[i]->yAxis->grid()->setSubGridVisible(true);
                timeHist[i]->xAxis->grid()->setSubGridVisible(true);
                timeHist[i]->yAxis->setScaleType(QCPAxis::stLinear);
                timeHist[i]->yAxis->setTicker(linTicker);
            }
        }

        on_button_update_clicked();
    }

private:

    void update_stats_labels(){

        //time

        ui->lbl_Nev->setText(QString::asprintf("Sum: %d",FEE.statsCh[0][hTime].sum));
        ui->lbl_Nev_2->setText(QString::asprintf("Sum: %d",FEE.statsCh[1][hTime].sum));
        ui->lbl_Nev_3->setText(QString::asprintf("Sum: %d",FEE.statsCh[2][hTime].sum));
        ui->lbl_Nev_4->setText(QString::asprintf("Sum: %d",FEE.statsCh[3][hTime].sum));
        ui->lbl_Nev_5->setText(QString::asprintf("Sum: %d",FEE.statsCh[4][hTime].sum));
        ui->lbl_Nev_6->setText(QString::asprintf("Sum: %d",FEE.statsCh[5][hTime].sum));
        ui->lbl_Nev_7->setText(QString::asprintf("Sum: %d",FEE.statsCh[6][hTime].sum));
        ui->lbl_Nev_8->setText(QString::asprintf("Sum: %d",FEE.statsCh[7][hTime].sum));
        ui->lbl_Nev_9->setText(QString::asprintf("Sum: %d",FEE.statsCh[8][hTime].sum));
        ui->lbl_Nev_10->setText(QString::asprintf("Sum: %d",FEE.statsCh[9][hTime].sum));
        ui->lbl_Nev_11->setText(QString::asprintf("Sum: %d",FEE.statsCh[10][hTime].sum));
        ui->lbl_Nev_12->setText(QString::asprintf("Sum: %d",FEE.statsCh[11][hTime].sum));

        ui->lbl_Mean->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[0][hTime].mean));
        ui->lbl_Mean_2->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[1][hTime].mean));
        ui->lbl_Mean_3->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[2][hTime].mean));
        ui->lbl_Mean_4->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[3][hTime].mean));
        ui->lbl_Mean_5->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[4][hTime].mean));
        ui->lbl_Mean_6->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[5][hTime].mean));
        ui->lbl_Mean_7->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[6][hTime].mean));
        ui->lbl_Mean_8->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[7][hTime].mean));
        ui->lbl_Mean_9->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[8][hTime].mean));
        ui->lbl_Mean_10->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[9][hTime].mean));
        ui->lbl_Mean_11->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[10][hTime].mean));
        ui->lbl_Mean_12->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[11][hTime].mean));

        ui->lbl_StdDev->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[0][hTime].RMS));
        ui->lbl_StdDev_2->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[1][hTime].RMS));
        ui->lbl_StdDev_3->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[2][hTime].RMS));
        ui->lbl_StdDev_4->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[3][hTime].RMS));
        ui->lbl_StdDev_5->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[4][hTime].RMS));
        ui->lbl_StdDev_6->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[5][hTime].RMS));
        ui->lbl_StdDev_7->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[6][hTime].RMS));
        ui->lbl_StdDev_8->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[7][hTime].RMS));
        ui->lbl_StdDev_9->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[8][hTime].RMS));
        ui->lbl_StdDev_10->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[9][hTime].RMS));
        ui->lbl_StdDev_11->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[10][hTime].RMS));
        ui->lbl_StdDev_12->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[11][hTime].RMS));

        histType type;

        if(ui->radio_0->isChecked()) {
            type = hADC0;
        } else if(ui->radio_0->isChecked()) {
            type = hADC1;
        } else {
            type = hAmpl;
        }


        ui->lbl_Nev_13->setText(QString::asprintf("Sum: %d",FEE.statsCh[0][type].sum));
        ui->lbl_Nev_14->setText(QString::asprintf("Sum: %d",FEE.statsCh[1][type].sum));
        ui->lbl_Nev_15->setText(QString::asprintf("Sum: %d",FEE.statsCh[2][type].sum));
        ui->lbl_Nev_16->setText(QString::asprintf("Sum: %d",FEE.statsCh[3][type].sum));
        ui->lbl_Nev_17->setText(QString::asprintf("Sum: %d",FEE.statsCh[4][type].sum));
        ui->lbl_Nev_18->setText(QString::asprintf("Sum: %d",FEE.statsCh[5][type].sum));
        ui->lbl_Nev_19->setText(QString::asprintf("Sum: %d",FEE.statsCh[6][type].sum));
        ui->lbl_Nev_20->setText(QString::asprintf("Sum: %d",FEE.statsCh[7][type].sum));
        ui->lbl_Nev_21->setText(QString::asprintf("Sum: %d",FEE.statsCh[8][type].sum));
        ui->lbl_Nev_22->setText(QString::asprintf("Sum: %d",FEE.statsCh[9][type].sum));
        ui->lbl_Nev_23->setText(QString::asprintf("Sum: %d",FEE.statsCh[10][type].sum));
        ui->lbl_Nev_24->setText(QString::asprintf("Sum: %d",FEE.statsCh[11][type].sum));

        ui->lbl_Mean_13->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[0][type].mean));
        ui->lbl_Mean_14->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[1][type].mean));
        ui->lbl_Mean_15->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[2][type].mean));
        ui->lbl_Mean_16->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[3][type].mean));
        ui->lbl_Mean_17->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[4][type].mean));
        ui->lbl_Mean_18->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[5][type].mean));
        ui->lbl_Mean_19->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[6][type].mean));
        ui->lbl_Mean_20->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[7][type].mean));
        ui->lbl_Mean_21->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[8][type].mean));
        ui->lbl_Mean_22->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[9][type].mean));
        ui->lbl_Mean_23->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[10][type].mean));
        ui->lbl_Mean_24->setText(QString::asprintf("Mean: %.2f",FEE.statsCh[11][type].mean));

        ui->lbl_StdDev_13->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[0][type].RMS));
        ui->lbl_StdDev_14->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[1][type].RMS));
        ui->lbl_StdDev_15->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[2][type].RMS));
        ui->lbl_StdDev_16->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[3][type].RMS));
        ui->lbl_StdDev_17->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[4][type].RMS));
        ui->lbl_StdDev_18->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[5][type].RMS));
        ui->lbl_StdDev_19->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[6][type].RMS));
        ui->lbl_StdDev_20->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[7][type].RMS));
        ui->lbl_StdDev_21->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[8][type].RMS));
        ui->lbl_StdDev_22->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[9][type].RMS));
        ui->lbl_StdDev_23->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[10][type].RMS));
        ui->lbl_StdDev_24->setText(QString::asprintf("RMS: %.2f",FEE.statsCh[11][type].RMS));
    }


    Ui::MainWindow *ui;

    QDateTime start, end;

    QCustomPlot* chargeHist[12];            // First hist
    QCPBars* chargeBars[12];
    QCPBars* chargeBars1[12];

    QCustomPlot* timeHist[12];              // Second hist
    QCPBars* timeBars[12];
};

#endif // MAINWINDOW_H
