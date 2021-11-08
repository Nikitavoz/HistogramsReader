#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("INR");
    QCoreApplication::setApplicationName("HistogramReader");
    QCoreApplication::setApplicationVersion("2.7");
    MainWindow w;
    w.show();
    return a.exec();
}
