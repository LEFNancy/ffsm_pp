######################################################################
# Project file for the Pythia Forest Model
# http://www.pythia-model.org
#
# You need the Qt GUI framework to use this file.
######################################################################

QT += xml
TARGET = pythia
DESTDIR = ..

unix {
     LIBS += -lipopt
     LIBS += -ladolc
     #LIBS += -lColPack
     INCLUDEPATH += /usr/include/coin
     INCLUDEPATH += /usr/include/coin/ThirdParty
     INCLUDEPATH += /usr/include/adolc
     LIBS += -lcoinmetis -lcoinmumps -lblas -llapack
     LIBS += -lpthread -lgfortran -lcoinmetis -lblas

}

win32 {

    INCLUDEPATH += win32/include/coin
    INCLUDEPATH += win32/include/coin/ThirdParty
    INCLUDEPATH += win32/include
    INCLUDEPATH += win32/include/adolc
    INCLUDEPATH += $$[QT_INSTALL_DATA]/src/3rdparty/zlib
    LIBS += -L win32/lib -lipopt
    LIBS += -L win32/lib -lcoinmetis
    LIBS += -L win32/lib -lcoinmumps
    LIBS += -L win32/lib -lcoinblas
    LIBS += -L win32/lib -lcoinlapack
    LIBS += -L win32/lib -ladolc
    LIBS += -lpthread -lgfortran -lcoinmetis -lcoinblas
}


TEMPLATE = app
DEPENDPATH += ". agents"
CONFIG += warn_on \
          qt \
          thread \
          debug
CONFIG -= release

# testing..
#CONFIG += link_pkgconfig
#PKGCONFIG += ipopt
#PKGCONFIG += coinasl
#PKGCONFIG += coinmetis
#PKGCONFIG += ipoptamplinterface
#PKGCONFIG += coinmumps



INCLUDEPATH += ". agents"


OBJECTS_DIR = ../bin

# Input
HEADERS += Adolc_debugtest.h \
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
           ModelCore.h
FORMS   += MainWindow.ui
SOURCES += Adolc_debugtest.cpp \
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
           ModelCore.cpp

RESOURCES += resources.qrc
