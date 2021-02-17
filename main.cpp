#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("INR");
    QCoreApplication::setApplicationName("HistogramReader");
    QCoreApplication::setApplicationVersion("1.3");
    QApplication::setStyle(QStyleFactory::create("windowsvista"));
    MainWindow w;
    w.show();
    return a.exec();
}
