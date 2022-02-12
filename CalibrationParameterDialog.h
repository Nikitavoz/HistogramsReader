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

class CalibrationParameterDialog : public QDialog
{
    Q_OBJECT
public:
    CalibrationParameterDialog(float adcPerMip=16.0f, quint32 steps=7500, QWidget *parent = nullptr)
      : QDialog(parent)
      , _adcLineEdit()
      , _startStepsLineEdit()
      , _channelSelectCheckBoxes()
      , _buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Vertical, this)
    {
        _adcLineEdit.setText(QString::asprintf("%g",adcPerMip));
        _startStepsLineEdit.setText(QString::asprintf("%d", steps));

        connect(&_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(&_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto channelLayout = new QHBoxLayout;
        for (int ch=0; ch<12; ++ch) {
            auto b = new QCheckBox(QString::asprintf("CH%02d",1+ch));
             b->setChecked(true);
            _channelSelectCheckBoxes.append(b);
            channelLayout->addWidget(b);
        }

        auto formLayout = new QFormLayout;
        formLayout->addRow(tr("ADC/MIP:"), &_adcLineEdit);
        formLayout->addRow(tr("Initial attenuator steps:"), &_startStepsLineEdit);

        auto spacer = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto mainLayout = new QGridLayout;
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        mainLayout->addLayout(channelLayout,  0, 0, 1, 4);
        mainLayout->addLayout(formLayout,     1, 2, 2, 1);
        mainLayout->addItem  (spacer,         1, 3, 2, 1);
        mainLayout->addWidget(&_buttonBox,    1, 0, 2, 1);
        mainLayout->setRowStretch(2, 1);
        setLayout(mainLayout);

        setWindowTitle(tr("Calibration"));
    }
    float getADCPerMip() const {
        auto s = _adcLineEdit.text();
        float adcPerMip = 16.0f;
        QTextStream(&s) >> adcPerMip;
        return adcPerMip;
    }
    quint32 getInitialSteps() const {
        auto s = _startStepsLineEdit.text();
        quint32 steps = 7100;
        QTextStream(&s) >> steps;
        return steps;
    }
    bool isChannelSelected(int ch) const {
        return _channelSelectCheckBoxes.at(ch)->isChecked();
    }
public slots:
protected:
private:
    QLineEdit _adcLineEdit;
    QLineEdit _startStepsLineEdit;
    QList<QCheckBox*> _channelSelectCheckBoxes;
    QDialogButtonBox _buttonBox;
};

#endif // CALIBRATIONPARAMETERDIALOG_H
