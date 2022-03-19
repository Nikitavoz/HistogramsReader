#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<quint32> >("QVector<quint32>");
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("INR");
    QCoreApplication::setApplicationName("HistogramReader");
    QCoreApplication::setApplicationVersion("3.31 calibration");
    MainWindow w;
    w.show();
    return a.exec();
}
