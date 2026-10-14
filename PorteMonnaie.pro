QT       += core gui widgets
CONFIG   += c++17
TARGET    = PorteMonnaie
TEMPLATE  = app

SOURCES += main.cpp \
           mainwindow.cpp \
           simbackend.cpp

HEADERS += mainwindow.h \
           cardbackend.h \
           simbackend.h

# ---------------------------------------------------------------------------
# Real reader (default on Windows) : ODALID SDK in ./LIB
#   -> use a MinGW 64-bit kit (ODALID_Education.dll / libODALID_Education.a are 64-bit MinGW)
# To force the simulated card instead, add  CONFIG+=sim  in Projects > Build > qmake > Additional arguments
# ---------------------------------------------------------------------------
win32:!sim {
    DEFINES     += USE_ODALID
    SOURCES     += odalidbackend.cpp
    HEADERS     += odalidbackend.h
    INCLUDEPATH += $$PWD/LIB
    LIBS        += -L$$PWD/LIB -lODALID_Education
    LIBS        += -lws2_32

    # exe goes to <build>/bin and the DLL is copied next to it
    DESTDIR      = $$OUT_PWD/bin
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/LIB/ODALID_Education.dll)) $$shell_quote($$shell_path($$DESTDIR)) $$escape_expand(\\n\\t)
}
