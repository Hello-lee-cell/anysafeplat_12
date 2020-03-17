#-------------------------------------------------
#
# Project created by QtCreator 2017-10-24T09:52:19
#
#-------------------------------------------------

QT       += core gui
QT       += multimedia
QT       += sql
QT       += xml
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = anysafe_plat
TEMPLATE = app
target.path = /opt
INSTALLS += target

SOURCES += main.cpp\
        mainwindow.cpp \
    keyboard.cpp \
    warn.cpp \
    login.cpp \
    history.cpp \
    main_main.cpp \
    ip_op.cpp \
    serial.cpp \
    mythread.cpp \
    udp.cpp \
    io_op.cpp \
    systemset.cpp \
    file_op.cpp \
    connectus.cpp \
    qcustomplot.cpp \
    uartthread.cpp \
    warn_sound_thread.cpp \
    database_op.cpp \
    radar_485.cpp \
    reoilgasthread.cpp \
    myqsqlrelationmodel_centerdisp.cpp \
    fga1000_485.cpp \
    security.cpp \
    post_webservice.cpp \
    net_isoosi.cpp \
    net_tcpclient_hb.cpp \
    timer_pop.cpp \
    reoilgas_pop.cpp \
    net_isoosi_cq.cpp \
    post_webservice_hunan.cpp \
    main_main_zhongyou.cpp \
    mytcpclient_zhongyou.cpp \
    protobuf/myserver_thread.cpp \
    protobuf/pb_common.c \
    protobuf/pb_decode.c \
    protobuf/pb_encode.c \
    protobuf/xielou.pb.c \
    myserver/des.cpp \
    myserver/myserver.cpp

HEADERS  += mainwindow.h \
    keyboard.h \
    warn.h \
    login.h \
    history.h \
    config.h \
    main_main.h \
    ip_op.h \
    serial.h \
    mythread.h \
    udp.h \
    io_op.h \
    systemset.h \
    file_op.h \
    connectus.h \
    radar_485.h \
    uartthread.h \
    qcustomplot.h \
    warn_sound_thread.h \
    database_set.h \
    database_op.h \
    reoilgasthread.h \
    myqsqlrelationmodel_centerdisp.h \
    fga1000_485.h \
    security.h \
    post_webservice.h \
    net_isoosi.h \
    net_tcpclient_hb.h \
    timer_pop.h \
    reoilgas_pop.h \
    net_isoosi_cq.h \
    post_webservice_hunan.h \
    main_main_zhongyou.h \
    mytcpclient_zhongyou.h \
    protobuf/myserver_thread.h \
    protobuf/pb.h \
    protobuf/pb_common.h \
    protobuf/pb_decode.h \
    protobuf/pb_encode.h \
    protobuf/xielou.pb.h \
    myserver/des.h \
    myserver/myserver.h

FORMS    += mainwindow.ui \
    keyboard.ui \
    warn.ui \
    login.ui \
    history.ui \
    systemset.ui \
    connectus.ui \
    reoilgas_pop.ui

RESOURCES += \
    picture_.qrc

DISTFILES += \
    file_op.gcc

QMAKE_CXXFLAGS += -std=c++11
