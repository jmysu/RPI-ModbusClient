QT += serialbus serialport widgets

TARGET = ../jcModbusClient
TEMPLATE = app
CONFIG += c++11

#Output
UI_DIR      = uic
MOC_DIR     = moc
OBJECTS_DIR = obj

SOURCES += main.cpp\
        mainwindow.cpp \
        settingsdialog.cpp \
        writeregistermodel.cpp \
        tablemodel.cpp \
        mainwindow_csv.cpp
unix:!macx {
    SOURCES +=  rpiCpuSerial.cpp
    }

HEADERS  += mainwindow.h \
        settingsdialog.h \
        writeregistermodel.h \
        tablemodel.h

FORMS    += mainwindow.ui \
         settingsdialog.ui

RESOURCES += \
    jcModbus.qrc

macx: {
    APP_CSV_FILES.files = ../ModbusMC0162.csv ../01ModbusTC100.csv ../01ModbusTC100Loop.csv ../00ModbusTC100INIT.csv
    APP_CSV_FILES.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += APP_CSV_FILES
    }
