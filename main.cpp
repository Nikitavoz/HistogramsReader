#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<quint32>>("QVector<quint32>");
    qRegisterMetaType<std::array<double, 3>>("std::array<double, 3>");

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("INR");
    QCoreApplication::setApplicationName("HistogramReader");
    QCoreApplication::setApplicationVersion("3.9 calibration");
    MainWindow w;
    w.show();
    return a.exec();
}
