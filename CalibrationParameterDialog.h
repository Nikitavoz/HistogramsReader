#ifndef CALIBRATIONPARAMETERDIALOG_H
#define CALIBRATIONPARAMETERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFormLayout>
#include <QTextStream>

#include "FITelectronics.h"
#include "CalibrationTasks.h"
#include "CalibrationPlots.h"

#include "ui_calibration.h"

class CalibrationParameterDialog : public QDialog
{
    Q_OBJECT
public:
    CalibrationParameterDialog(QString ipAddress, float adcPerMip=16.0f, quint32 steps=7500, QWidget *parent = nullptr)
      : QDialog(parent)
      , _ui(new Ui::CalibrationWindow)
      , _channelSelect()
      , _channelStatus()
      , _calLogs()
      , _calPlots()
      , _colorStyles()
      , _calibrationTasks(new CalibrationTasks(ipAddress))
      , _timeAlignmentDone(false)
      , _lastiBd(-1)
    {
        _ui->setupUi(this);

        // colors indicating calibration and channel status
        _colorStyles.append("QCheckBox::indicator {background-color:gray;}");          // disabled
        _colorStyles.append("QCheckBox::indicator {background-color:blue;}");          // enabled
        _colorStyles.append("QCheckBox::indicator {background-color:yellow;}");        // calibration in progress
        _colorStyles.append("QCheckBox::indicator {background-color:rgb(0,180,0);}");  // green - calibration succeeded
        _colorStyles.append("QCheckBox::indicator {background-color:red;}");           // error

        // channel selection
        foreach (QWidget* w, _ui->groupChannelSelect->findChildren<QWidget *>(QRegularExpression("checkBoxCH*"))) {
            _channelSelect.append(qobject_cast<QCheckBox*>(w));
            connect(_channelSelect.back(), &QCheckBox::clicked, this, &CalibrationParameterDialog::onChannelSelect);
        }

        // channel calibration status
        foreach (QWidget* w, _ui->groupChannelStatus->findChildren<QWidget *>(QRegularExpression("statusCH*"))) {
            _channelStatus.append(qobject_cast<QCheckBox*>(w));
            _channelStatus.back()->setStyleSheet(_colorStyles.at(1));
        }

        // set up log tabs for each channel
        for (int ch=0; ch<12; ++ch) {
             QVBoxLayout *layout = new QVBoxLayout;
             _calPlots.append(new CalibrationPlots());
             _calPlots.back()->setObjectName(QString::asprintf("calPlotsCH%02d",1+ch));
             _calPlots.back()->clear();
             layout->addWidget(_calPlots.back());
             QPlainTextEdit* pte = new QPlainTextEdit();
             pte->setObjectName(QString::asprintf("plainTextEditCH%02d", 1+ch));
             pte->setReadOnly(true);
             pte->setUndoRedoEnabled(false);
             //pte->setMouseTracking(false);
             pte->setTextInteractionFlags(Qt::NoTextInteraction);
             layout->addWidget(pte);
            _ui->tabWidget->setCurrentIndex(1+ch);
            _ui->tabWidget->currentWidget()->setLayout(layout);
        }

        // log tabs
        QFont font("monospace");
        font.setStyleHint(QFont::TypeWriter);
        font.setFamily("courier");
        auto l = _ui->tabWidget->findChildren<QWidget *>(QRegularExpression("plainTextEdit*"));
        qSort(l.begin(), l.end(), [](QWidget*a, QWidget*b) { return a->objectName() < b->objectName(); });
        foreach (QWidget *w, l) {
            _calLogs.append(qobject_cast<QPlainTextEdit*>(w));
            _calLogs.back()->setFont(font);
        }
        _ui->tabWidget->setCurrentIndex(0);

        // buttons
        connect(_ui->pushButtonClose, &QPushButton::clicked, this, &QDialog::reject);
        connect(_ui->pushButtonStart, &QPushButton::clicked, this, &CalibrationParameterDialog::onStartCalibration);
        connect(_ui->pushButtonAbort, &QPushButton::clicked, this, &CalibrationParameterDialog::onAbortCalibration);
        connect(_ui->pushButtonRestoreTimeDelays, &QPushButton::clicked, _calibrationTasks.get(), &CalibrationTasks::onRestoreTimeDelays);

        _ui->groupParametersCFD_ZERO->setVisible(false);
        hideRestoreTimeDelays();

        // connections
        connect(_ui->radioButtonTimeAlign, &QRadioButton::clicked, this, [=]() { updateCalibrationStatus(); _ui->groupParametersCFD_ZERO->setVisible(false); });
        connect(_ui->radioButtonADC_DELAY, &QRadioButton::clicked, this, [=]() { updateCalibrationStatus(); _ui->groupParametersCFD_ZERO->setVisible(false); });
        connect(_ui->radioButtonCFD_ZERO,  &QRadioButton::clicked, this, [=]() { updateCalibrationStatus(); _ui->groupParametersCFD_ZERO->setVisible(true);  });

        connect(_calibrationTasks.get(), SIGNAL(finished()),              this, SLOT(onCalibrationFinished()),            Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(updateStatus(int,int)),   this, SLOT(onCalibrationStatusUpdate(int,int)), Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(logMessage(int,QString)), this, SLOT(onLogMessage(int,QString)),          Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(addPointADCvSteps(int,float,float,bool)), this, SLOT(onAddPointADCvSteps(int,float,float,bool)), Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(clearCalPlots(int)),      this, SLOT(onClearCalPlots(int)),               Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(addHistLine(int,int,int,QVector<quint32>)), this, SLOT(onAddHistLine(int,int,int,QVector<quint32>)), Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(setTitlesADC(int, std::array<double,3>)), this, SLOT(onSetTitlesADC(int, std::array<double,3>)), Qt::QueuedConnection);
        connect(_calibrationTasks.get(), SIGNAL(initialStepValue(int,int)), this, SLOT(onInitialStepValue(int,int)), Qt::QueuedConnection);

        // default parameters
        _ui->lineEditADCpMIP->setText(QString::asprintf("%g",adcPerMip));
        _ui->lineEditInitAttenSteps->setText(QString::asprintf("%d", steps));

        setWindowTitle(tr("Calibration"));
    }

    void setADCPerMip(float value) {
        _ui->lineEditADCpMIP->setText(QString::asprintf("%g", value));
    }
    float getADCPerMip() const {
        auto s = _ui->lineEditADCpMIP->text();
        float adcPerMip = 16.0f;
        QTextStream(&s) >> adcPerMip;
        return adcPerMip;
    }
    void setRefRatekHz(float value) {
        _ui->lineEditRefRatekHz->setText(QString::asprintf("%g", value));
    }
    float getRefRatekHz() const {
        auto s = _ui->lineEditRefRatekHz->text();
        float refRatekHz = 1.0f;
        QTextStream(&s) >> refRatekHz;
        return refRatekHz;
    }
    quint32 getInitialSteps() const {
        auto s = _ui->lineEditInitAttenSteps->text();
        quint32 steps = 7100;
        QTextStream(&s) >> steps;
        return steps;
    }
    bool isChannelSelected(int ch) const {
        return _channelSelect.at(ch)->isChecked();
    }
    void hideRestoreTimeDelays() {
        _timeAlignmentDone = false;
        _ui->pushButtonRestoreTimeDelays->setVisible(false);
    }

    void setIPAddress(QString ip) {
        _calibrationTasks->setIPAddress(ip);
    }
    void setiBd(int iBd, QString pmName) {
        if (_lastiBd != iBd) {
            _lastiBd = iBd;
            reset();
        }
        _calibrationTasks->setiBd(iBd);
        _ui->groupChannelSelect->setTitle("Channel Selection - (PM" + pmName + ")");
        setWindowTitle("PM Calibration");
    }
    void reset() {
        for (int ch=0; ch<13; ++ch) { // ch==12 ... ALL
            _channelSelect.at(ch)->setChecked(true);
            _calLogs.at(ch)->clear();
            if (ch != 12) {
                _calPlots.at(ch)->clear();
                _channelStatus.at(ch)->setStyleSheet(_colorStyles.at(1));
            }
        }
        _ui->radioButtonTimeAlign->setChecked(true);
        _ui->groupChannelStatus->setTitle("Calibration Status");
    }
public slots:

protected:
    void resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);
        auto const newHeight = std::max(minimumHeight(), event->size().height());
        auto const newWidth = std::max(minimumWidth(), event->size().width());
        _ui->tabWidget->resize(newWidth - 38,
                               newHeight - 249);
    }
    void updateCalibrationStatus() {
        for (auto ch=0; ch<12; ++ch) {
            _channelStatus.at(ch)->setStyleSheet(_colorStyles.at(_channelSelect.at(ch)->isChecked()));
        }
    }

private slots:
    void onChannelSelect(int value) {
        QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
        auto const ch = _channelSelect.indexOf(cb);
        if (ch == 12) { // ALL
            for (auto i=0; i<12; ++i) {
                _channelSelect.at(i)->setChecked(value);
                _channelStatus.at(i)->setStyleSheet(_colorStyles.at(value));
            }
        } else {
            int sum = 0;
            for (auto i=0; i<12; ++i) {
                sum += _channelSelect.at(i)->isChecked();
            }
            _channelSelect.back()->setChecked(sum == 12);
            _channelStatus.at(ch)->setStyleSheet(_colorStyles.at(value));
        }
    }
    void onStartCalibration(bool) {
        _ui->pushButtonStart->setDisabled(true);
        _ui->pushButtonAbort->setEnabled(true);
        _ui->pushButtonClose->setDisabled(true);
        _ui->groupCalibrationTypes->setDisabled(true);
        _ui->groupChannelSelect->setDisabled(true);
        _ui->groupParametersCFD_ZERO->setDisabled(true);
        _ui->pushButtonRestoreTimeDelays->setDisabled(true);
        foreach(auto c, _calLogs) {
            c->clear();
        }
        std::array<bool, 12> activeChannelMap;
        for (auto i=0; i<12; ++i) {
            activeChannelMap[i] = _channelSelect.at(i)->isChecked();
            if (activeChannelMap[i]) {
                _channelStatus.at(i)->setStyleSheet(_colorStyles.at(1));
            }
        }
        _calibrationTasks->setActiveChannelMap(activeChannelMap);
        _calibrationTasks->setRefRatekHz(getRefRatekHz());
        if (_ui->radioButtonTimeAlign->isChecked()) {
            _calibrationTasks->setMode("TimeAlign");
            _ui->groupChannelStatus->setTitle("Calibration Status - Time Alignment");
        }
        if (_ui->radioButtonADC_DELAY->isChecked()) {
            _calibrationTasks->setMode("ADC_DELAY");
            _ui->groupChannelStatus->setTitle("Calibration Status - ADC_DELAY");
        }
        if (_ui->radioButtonCFD_ZERO->isChecked()) {
            _calibrationTasks->setMode("CFD_ZERO");
            _ui->groupChannelStatus->setTitle("Calibration Status - CFD_ZERO");
            _calibrationTasks->setADCpMIP(getADCPerMip());
            _calibrationTasks->setInitialSteps(getInitialSteps());
        }
        QThreadPool::globalInstance()->start(_calibrationTasks.get());
    }
    void onAbortCalibration(bool) {
        _calibrationTasks->setAbort();
        _ui->pushButtonStart->setEnabled(true);
        _ui->pushButtonAbort->setDisabled(true);
        _ui->pushButtonClose->setEnabled(true);
        _ui->groupCalibrationTypes->setEnabled(true);
        _ui->groupChannelSelect->setEnabled(true);
        _ui->groupParametersCFD_ZERO->setEnabled(true);
        _ui->pushButtonRestoreTimeDelays->setVisible(_timeAlignmentDone);
        _ui->pushButtonRestoreTimeDelays->setEnabled(true);
    }
    void onCalibrationFinished() {
        _ui->pushButtonStart->setEnabled(true);
        _ui->pushButtonAbort->setDisabled(true);
        _ui->pushButtonClose->setEnabled(true);
        _ui->groupCalibrationTypes->setEnabled(true);
        _ui->groupChannelSelect->setEnabled(true);
        _ui->groupParametersCFD_ZERO->setEnabled(true);
        if (_ui->radioButtonTimeAlign->isChecked()) {
            _timeAlignmentDone = true;
        }
        _ui->pushButtonRestoreTimeDelays->setVisible(_timeAlignmentDone);
        _ui->pushButtonRestoreTimeDelays->setEnabled(true);
    }
    void onCalibrationStatusUpdate(int ch, int status) {
        _channelStatus.at(ch)->setStyleSheet(_colorStyles.at(status));
        if (status == 0) {
            _channelSelect.at(ch)->setChecked(false);
            _channelSelect.at(12)->setChecked(false); // ALL
        }
    }
    void onLogMessage(int tabIndex, QString msg) {
        _calLogs.at(tabIndex)->insertPlainText(msg);
        QScrollBar *sb = _calLogs.at(tabIndex)->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
    void onAddPointADCvSteps(int ch, float x, float y, bool dots) {
        _calPlots.at(ch)->addPoint(x, y, dots);
        _calPlots.at(ch)->replot();
    }
    void onClearCalPlots(int ch) {
        _calPlots.at(ch)->clear();
    }
    void onAddHistLine(int ch, int i, int j, QVector<quint32> x) {
        _calPlots.at(ch)->setHistogramLine(i, j, x);
        _calPlots.at(ch)->setAxisRange(-50, 50, -500, 500);
        _calPlots.at(ch)->rescaleDataRanges();
        _calPlots.at(ch)->replot();
    }
    void onSetTitlesADC(int ch, std::array<double,3> adcs) {
        _calPlots.at(ch)->setTitles(adcs);
    }
    void onInitialStepValue(int ch, int steps) {
        _calPlots.at(ch)->setInitialSteps(steps);
    }
 private:
    std::unique_ptr<Ui::CalibrationWindow> _ui;
    QList<QCheckBox*> _channelSelect;
    QList<QCheckBox*> _channelStatus;
    QList<QPlainTextEdit*> _calLogs;
    QList<CalibrationPlots*> _calPlots;
    QStringList       _colorStyles;
    std::unique_ptr<CalibrationTasks> _calibrationTasks;
    bool _timeAlignmentDone;
    int _lastiBd;
};

#endif // CALIBRATIONPARAMETERDIALOG_H
