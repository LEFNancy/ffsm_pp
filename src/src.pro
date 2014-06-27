######################################################################
# Project file for the FFSM Forest Model
# http://www.ffsm-model.org
#
# You need the Qt GUI framework to use this file.
######################################################################

QT += xml
QT += widgets
DESTDIR = ..
#TARGET = ffsm

unix {
     #LIBS += -lipopt
     LIBS += -ladolc
     LIBS += -lz # needed in Qt5/ ubuntu 13.10 64bit
     #LIBS += -lColPack
     INCLUDEPATH += /usr/include/coin
     INCLUDEPATH += /usr/include/coin/ThirdParty
     INCLUDEPATH += /usr/include/adolc
     INCLUDEPATH += `PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/share/pkgconfig: /usr/bin/pkg-config --cflags ipopt` $(ADDINCFLAGS)
     LIBS += `PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/share/pkgconfig: /usr/bin/pkg-config --libs ipopt`
     #LIBS += -lcoinmetis -lcoinmumps -lblas -llapack
     #LIBS += -lpthread -lgfortran -lcoinmetis -lblas
     #QMAKE_CXXFLAGS_RELEASE += -O3 -pipe -DNDEBUG -pedantic-errors -Wparentheses -Wreturn-type -Wcast-qual -Wall -Wpointer-arith -Wwrite-strings -Wconversion -Wno-unknown-pragmas -Wno-long-long   -DIPOPT_BUILD
     #QMAKE_LFLAGS_RELEASE +=  -Wl,--rpath -Wl,/usr/lib
     #QMAKE_LFLAGS += -lz
     #QMAKE_LFLAGS_RELEASE += -lz
     #QMAKE_LFLAGS_DEBUG += -lz


}

win32 {

    INCLUDEPATH += ThirdParty/win32/include/coin
    INCLUDEPATH += ThirdParty/win32/include/coin/ThirdParty
    INCLUDEPATH += ThirdParty/win32/include
    INCLUDEPATH += ThirdParty/win32/include/adolc
    INCLUDEPATH += $$[QT_INSTALL_DATA]/src/3rdparty/zlib
    LIBS += -L ThirdParty/win32/lib -lipopt
    LIBS += -L ThirdParty/win32/lib -lcoinmetis
    LIBS += -L ThirdParty/win32/lib -lcoinmumps
    LIBS += -L ThirdParty/win32/lib -lcoinhsl
    LIBS += -L ThirdParty/win32/lib -lcoinblas
    LIBS += -L ThirdParty/win32/lib -lcoinlapack
    LIBS += -L ThirdParty/win32/lib -ladolc
    LIBS += -lpthread -lgfortran -lcoinmetis -lcoinblas
    #CONFIG +=  console
    CONFIG += exceptions
}

INCLUDEPATH += ThirdParty/allos/include


TEMPLATE = app
DEPENDPATH += ". agents"
CONFIG += warn_on \
          qt \
	    thread \
	    debug_and_release
#CONFIG -= release

QMAKE_CXXFLAGS += -std=c++0x

#QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE += -O3

#QMAKE_LFLAGS_RELEASE -= -O1

# testing..
#CONFIG += link_pkgconfig
#PKGCONFIG += ipopt
#PKGCONFIG += coinasl
#PKGCONFIG += coinmetis
#PKGCONFIG += ipoptamplinterface
#PKGCONFIG += coinmumps



#INCLUDEPATH += ". agents"


#OBJECTS_DIR = ../bin


CONFIG(release, debug|release) {
    TARGET = ffsm
}
CONFIG(debug, debug|release) {
    TARGET = ffsm_debug
}

#Release:DESTDIR = ../build/release
#Release:TARGET = ffsm
Release:OBJECTS_DIR = ../build/release
Release:MOC_DIR = ../build/release
Release:RCC_DIR = ../build/release
#Release:UI_DIR = ../build/release # then th header file can't find the other headers!

#Debug:DESTDIR = ../build/debug
#Debug:TARGET = ffsm_debug
Debug:OBJECTS_DIR = ../build/debug
Debug:MOC_DIR = ../build/debug
Debug:RCC_DIR = ../build/debug
#Debug:UI_DIR = ../build/debug


# Input
HEADERS += Adolc_debugtest.h \
           CommonLib.h \
           BaseClass.h \
           Gis.h \
           Init.h \
           InputNode.h \
           Ipopt_nlp_problem_debugtest.h \
           MainProgram.h \
           MainWindow.h \
           ModelData.h \
           ModelRegion.h \
           Opt.h \
           Output.h \
           Pixel.h \
           Sandbox.h \
           Scheduler.h \
           ThreadManager.h \
           MapBox.h  \
           Layers.h \
           unzip.h \
           unzip_p.h \
           zip.h \
           zip_p.h \
           zipentry_p.h \
           anyoption.h \
           ScenarioSelectionWidget.h \
           ModelCore.h \
           ModelCoreSpatial.h \
           Carbon.h \

FORMS   += MainWindow.ui
SOURCES += Adolc_debugtest.cpp \
           CommonLib.cpp \
           BaseClass.cpp \
           Gis.cpp \
           Init.cpp \
           Ipopt_nlp_problem_debugtest.cpp\
           InputNode.cpp \
           main.cpp \
           MainProgram.cpp \
           ModelData.cpp \
           ModelRegion.cpp \
           Opt.cpp \
           Output.cpp \
           Pixel.cpp \
           Scheduler.cpp \
           Sandbox.cpp \
           ThreadManager.cpp \
           MainWindow.cpp \
           MapBox.cpp \
           Layers.cpp \
           unzip.cpp \
           zip.cpp \
           anyoption.cpp \
           ScenarioSelectionWidget.cpp \
           ModelCore.cpp \
           ModelCoreSpatial.cpp \
           Carbon.cpp

RESOURCES += resources.qrc

