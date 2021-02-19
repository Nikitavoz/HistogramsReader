#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "FITelectronics.h"
#include "switch.h"
#include <QMainWindow>
#include <QtWidgets>
#include <QDateTime>

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
        setMaximumSize(size());
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
        connect(&FEE, &IPbusTarget::networkError, this, [=](QString message) { QMessageBox::warning(this, "Network Error", message); statusBar()->showMessage(message); });
        connect(&FEE, &IPbusTarget::IPbusError  , this, [=](QString message) { QMessageBox::warning(this, "IPbus error"  , message); statusBar()->showMessage(message); });
        connect(&FEE, &IPbusTarget::logicError  , this, [=](QString message) { QMessageBox::warning(this, "Error"        , message); statusBar()->showMessage(message); });
        connect(&FEE, &IPbusTarget::IPbusStatusOK, this, [=]() {
            statusBar()->showMessage(FEE.IPaddress + ": online");
            ui->centralwidget->setEnabled(true);
        });
        connect(&FEE, &IPbusTarget::noResponse, this, [=]() {
            statusBar()->showMessage(statusBar()->currentMessage() == "" ? FEE.IPaddress + ": no response" : "");
            ui->centralwidget->setDisabled(true);
        });
        connect(&FEE, &FITelectronics::linksStatusReady, this, [=](quint32 mask) {
            ui->comboBox->clear();
            for (quint8 i=0; i<10; ++i, mask >>= 1) if (mask & 1) ui->comboBox->addItem(QString("A") + QChar('0' + i));
            for (quint8 i=0; i<10; ++i, mask >>= 1) if (mask & 1) ui->comboBox->addItem(QString("C") + QChar('0' + i));
            ui->comboBox->setCurrentIndex(0);
        });
        connect(&FEE, &FITelectronics::statusReady, this, &MainWindow::updateActualValues);

        QString IPaddress = settings.value("IPaddress", FEE.IPaddress).toString();
        if (validIPaddressRE.exactMatch(IPaddress)) FEE.IPaddress = IPaddress;

        ui->centralwidget->setDisabled(true);
        FEE.reconnect();
    }

    ~MainWindow() {
        settings.setValue("IPaddress", FEE.IPaddress);
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
        else if (statusBar()->currentMessage() == "PM is resetting")
            statusBar()->showMessage(FEE.IPaddress + ": online");
        ui->switch_historgamming->setChecked(FEE.histStatus.histOn);
        ui->switch_filter->setChecked(FEE.histStatus.filterOn);
        if (ui->spinBox->value() != FEE.histStatus.BCID) ui->spinBox->setValue(FEE.histStatus.BCID);
        ui->label_historgamming->setToolTip(QString::number(FEE.curAddress));
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

    void on_switch_historgamming_clicked(bool checked) { FEE.switchHist(!checked); }
    void on_switch_filter_clicked(bool checked) { FEE.switchFilter(!checked); }

    void on_spinBox_valueChanged(int BCID) { FEE.setBCID(BCID); }

    void on_button_reset_clicked() { FEE.reset(); }

    void on_button_read_clicked() {
        QDateTime start = QDateTime::currentDateTime(), end;
        quint32 n = FEE.readHistograms();
        if (n != datasize) {
            statusBar()->showMessage(QString::asprintf( "%d/%d words read (%.1f%% of data)", n, datasize, n*100./datasize ));
        } else {
            end = QDateTime::currentDateTime();
            statusBar()->showMessage(QString::asprintf("Data:  %.3fs", start.msecsTo(end)/1000.));
            start = end;
            bool success = writeFiles();
            end = QDateTime::currentDateTime();
            if (success) statusBar()->showMessage(statusBar()->currentMessage() + QString::asprintf(", files:  %.3fs", start.msecsTo(end)/1000.));
            start = end;
            quint16 max = 0, *b = (quint16 *)&FEE.data, *e = b + datasize;
            for (quint16 *p=b; p<e; ++p) if (*p > max) max = *p;
            FEE.calcStats();
            end = QDateTime::currentDateTime();
            statusBar()->showMessage(statusBar()->currentMessage() + QString::asprintf(", stats: %.3fs, max: %d", start.msecsTo(end)/1000., max));
            qDebug() << QString::asprintf("Ch |Time: sum    mean    RMS |ADC0: sum   mean    RMS |ADC1: sum   mean    RMS");
//          qDebug() << QString::asprintf("00 |268431360 -2048.0 2047.0 |285208320 4096.0 2176.0 |285208320 4096.0 2176.0");
            for (quint8 iCh=0; iCh<12; ++iCh) {
                qDebug() << QString::asprintf("%02d |%9d % 7.1f %6.1f |%9d %6.1f %6.1f |%9d %6.1f %6.1f", iCh,
                    FEE.statsCh[iCh][hTime].sum, FEE.statsCh[iCh][hTime].mean, FEE.statsCh[iCh][hTime].RMS,
                    FEE.statsCh[iCh][hADC0].sum, FEE.statsCh[iCh][hADC0].mean, FEE.statsCh[iCh][hADC0].RMS,
                    FEE.statsCh[iCh][hADC1].sum, FEE.statsCh[iCh][hADC1].mean, FEE.statsCh[iCh][hADC1].RMS
                );
            }
        }
    }

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
