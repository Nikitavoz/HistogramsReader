QT       += core gui network printsupport serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++latest

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CalibrationTasks.cpp \
        qcustomplot.cpp qcpdocumentobject.cpp \
    main.cpp

HEADERS += \
        CalibrationParameterDialog.h qcpdocumentobject.h \
    CalibrationPlots.h \
    CalibrationTasks.h \
	qcustomplot.h \
	mainwindow.h \
	IPbusHeaders.h \
	IPbusInterface.h \
	FITelectronics.h \
	switch.h

FORMS += \
    calibration.ui \
    mainwindow.ui

INCLUDEPATH += $$PWD/../DIM/dim
LIBS += -L"$$PWD/../DIM/bin" -ldim

RESOURCES += \
		../!images/img.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
