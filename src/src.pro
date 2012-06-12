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
     INCLUDEPATH += win32/include/coin
     INCLUDEPATH += win32/include/coin/ThirdParty
     LIBS += -lcoinmetis -lcoinmumps -lblas -llapack
     LIBS += -lpthread -lgfortran -lcoinmetis -lblas

}

win32 {

    INCLUDEPATH += win32/include/coin
    INCLUDEPATH += win32/include/coin/ThirdParty
    INCLUDEPATH += $$[QT_INSTALL_DATA]/src/3rdparty/zlib
    LIBS += -L win32/lib -lipopt
    LIBS += -L win32/lib -lcoinmetis
    LIBS += -L win32/lib -lcoinmumps
    LIBS += -L win32/lib -lcoinblas
    LIBS += -L win32/lib -lcoinlapack
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
HEADERS += Agent_base.h \
           ipopt_nlp_problem.h \
           Agent_space.h \
           BaseRegmas.h \
           Gis.h \
           Init.h \
           InputNode.h \
           MainProgram.h \
           MainWindow.h \
           Manager_base.h \
           Manager_space.h \
           ModelObject.h \
           Opt.h \
           Output.h \
           Pixel.h \
           RegData.h \
           Sandbox.h \
           Scheduler.h \
           SuperAgentManager.h \
           ThreadManager.h \
           agents/Agent_farmer.h \
           agents/Manager_farmers.h \
           MapBox.h  \
           Layers.h \
           unzip.h \
           unzip_p.h \
           zip.h \
           zip_p.h \
           zipentry_p.h \
           anyoption.h \
           ScenarioSelectionWidget.h

FORMS += MainWindow.ui
SOURCES += Agent_base.cpp \
           ipopt_nlp_problem.cpp \
           Agent_space.cpp \
           BaseRegmas.cpp \
           Gis.cpp \
           Init.cpp \
           InputNode.cpp \
           main.cpp \
           MainProgram.cpp \
           Manager_base.cpp \
           Manager_space.cpp \
           ModelObject.cpp \
           Opt.cpp \
           Output.cpp \
           Pixel.cpp \
           RegData.cpp \
           Scheduler.cpp \
           Sandbox.cpp \
           SuperAgentManager.cpp \
           ThreadManager.cpp \
           agents/Agent_farmer.cpp \
           agents/Manager_farmers.cpp \
           MainWindow.cpp \
           MapBox.cpp \
           Layers.cpp \
           unzip.cpp \
           zip.cpp \
           anyoption.cpp \
           ScenarioSelectionWidget.cpp

RESOURCES += resources.qrc
