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
    serial.cpp \
    mythread.cpp \
    io_op.cpp \
    systemset.cpp \
    file_op.cpp \
    connectus.cpp \
    qcustomplot.cpp \
    uartthread.cpp \
    warn_sound_thread.cpp \
    database_op.cpp \
    radar_485.cpp \
    myqsqlrelationmodel_centerdisp.cpp \
    security.cpp \
    timer_pop.cpp \
    reoilgas_pop.cpp \
    protobuf/myserver_thread.cpp \
    protobuf/pb_common.c \
    protobuf/pb_decode.c \
    protobuf/pb_encode.c \
    protobuf/xielou.pb.c \
    myserver/des.cpp \
    myserver/myserver.cpp \
    airtightness_test.cpp \
    timer_thread.cpp \
    oilgas/fga1000_485.cpp \
    oilgas/reoilgasthread.cpp \
    network/ip_op.cpp \
    network/main_main.cpp \
    network/main_main_zhongyou.cpp \
    network/mytcpclient_zhongyou.cpp \
    network/net_isoosi.cpp \
    network/net_isoosi_cq.cpp \
    network/net_tcpclient_hb.cpp \
    network/post_webservice.cpp \
    network/post_webservice_hunan.cpp \
    network/udp.cpp \
    oilgas/one_click_sync.cpp

HEADERS  += mainwindow.h \
    keyboard.h \
    warn.h \
    login.h \
    history.h \
    config.h \
    serial.h \
    mythread.h \
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
    myqsqlrelationmodel_centerdisp.h \
    security.h \
    timer_pop.h \
    reoilgas_pop.h \
    protobuf/myserver_thread.h \
    protobuf/pb.h \
    protobuf/pb_common.h \
    protobuf/pb_decode.h \
    protobuf/pb_encode.h \
    protobuf/xielou.pb.h \
    myserver/des.h \
    myserver/myserver.h \
    airtightness_test.h \
    timer_thread.h \
    oilgas/fga1000_485.h \
    oilgas/reoilgasthread.h \
    network/ip_op.h \
    network/main_main.h \
    network/main_main_zhongyou.h \
    network/mytcpclient_zhongyou.h \
    network/net_isoosi.h \
    network/net_isoosi_cq.h \
    network/net_tcpclient_hb.h \
    network/post_webservice.h \
    network/post_webservice_hunan.h \
    network/udp.h \
    oilgas/one_click_sync.h

FORMS    += mainwindow.ui \
    keyboard.ui \
    warn.ui \
    login.ui \
    history.ui \
    systemset.ui \
    connectus.ui \
    reoilgas_pop.ui \
    airtightness_test.ui \
    oilgas/one_click_sync.ui

RESOURCES += \
    picture_.qrc

DISTFILES += \
    file_op.gcc

QMAKE_CXXFLAGS += -std=c++11
