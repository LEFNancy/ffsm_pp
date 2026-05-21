/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "MapBox.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionLoadConfiguration;
    QAction *actionSaveLog;
    QAction *actionSaveLogAs;
    QAction *actionRun;
    QAction *actionPause;
    QAction *actionStop;
    QAction *actionAboutRegMAS;
    QAction *actionExit;
    QAction *actionHideDebugMsgs;
    QAction *actionRegMASDocumentation;
    QAction *actionFitMap;
    QAction *actionViewResults;
    QWidget *centralwidget;
    QHBoxLayout *hboxLayout;
    QSplitter *splitter;
    QWidget *layoutWidget;
    QVBoxLayout *vboxLayout;
    QComboBox *layerSelector;
    QSpacerItem *spacerItem;
    MapBox *mapBox;
    QTabWidget *tabWidget;
    QWidget *log_area;
    QVBoxLayout *verticalLayout;
    QTextEdit *logArea;
    QPushButton *viewResultsButton;
    QWidget *model_viewer;
    QHBoxLayout *hboxLayout1;
    QTreeWidget *statusView;
    QWidget *plot_info;
    QGridLayout *gridLayout;
    QTextEdit *pxInfoArea;
    QMenuBar *menubar;
    QMenu *menuView;
    QMenu *menuHelp;
    QMenu *menuAction;
    QMenu *menuFile;
    QStatusBar *statusbar;
    QToolBar *modelToolBar;
    QToolBar *fileToolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->setWindowModality(Qt::ApplicationModal);
        MainWindow->resize(667, 467);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QStringLiteral(":/imgs/icon.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        MainWindow->setIconSize(QSize(24, 24));
        actionLoadConfiguration = new QAction(MainWindow);
        actionLoadConfiguration->setObjectName(QStringLiteral("actionLoadConfiguration"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/imgs/open.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionLoadConfiguration->setIcon(icon1);
        actionSaveLog = new QAction(MainWindow);
        actionSaveLog->setObjectName(QStringLiteral("actionSaveLog"));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/imgs/save.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSaveLog->setIcon(icon2);
        actionSaveLogAs = new QAction(MainWindow);
        actionSaveLogAs->setObjectName(QStringLiteral("actionSaveLogAs"));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/imgs/saveas.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSaveLogAs->setIcon(icon3);
        actionRun = new QAction(MainWindow);
        actionRun->setObjectName(QStringLiteral("actionRun"));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/imgs/play.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRun->setIcon(icon4);
        actionPause = new QAction(MainWindow);
        actionPause->setObjectName(QStringLiteral("actionPause"));
        QIcon icon5;
        icon5.addFile(QStringLiteral(":/imgs/pause.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPause->setIcon(icon5);
        actionStop = new QAction(MainWindow);
        actionStop->setObjectName(QStringLiteral("actionStop"));
        QIcon icon6;
        icon6.addFile(QStringLiteral(":/imgs/stop.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStop->setIcon(icon6);
        actionAboutRegMAS = new QAction(MainWindow);
        actionAboutRegMAS->setObjectName(QStringLiteral("actionAboutRegMAS"));
        QIcon icon7;
        icon7.addFile(QStringLiteral(":/imgs/info.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAboutRegMAS->setIcon(icon7);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        QIcon icon8;
        icon8.addFile(QStringLiteral(":/imgs/exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionExit->setIcon(icon8);
        actionHideDebugMsgs = new QAction(MainWindow);
        actionHideDebugMsgs->setObjectName(QStringLiteral("actionHideDebugMsgs"));
        actionHideDebugMsgs->setCheckable(true);
        QIcon icon9;
        icon9.addFile(QStringLiteral(":/imgs/clear.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionHideDebugMsgs->setIcon(icon9);
        actionRegMASDocumentation = new QAction(MainWindow);
        actionRegMASDocumentation->setObjectName(QStringLiteral("actionRegMASDocumentation"));
        QIcon icon10;
        icon10.addFile(QStringLiteral(":/imgs/help.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRegMASDocumentation->setIcon(icon10);
        actionFitMap = new QAction(MainWindow);
        actionFitMap->setObjectName(QStringLiteral("actionFitMap"));
        QIcon icon11;
        icon11.addFile(QStringLiteral(":/imgs/view-refresh.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFitMap->setIcon(icon11);
        actionViewResults = new QAction(MainWindow);
        actionViewResults->setObjectName(QStringLiteral("actionViewResults"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        hboxLayout = new QHBoxLayout(centralwidget);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName(QStringLiteral("splitter"));
        splitter->setOrientation(Qt::Horizontal);
        layoutWidget = new QWidget(splitter);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        vboxLayout = new QVBoxLayout(layoutWidget);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        layerSelector = new QComboBox(layoutWidget);
        layerSelector->setObjectName(QStringLiteral("layerSelector"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(layerSelector->sizePolicy().hasHeightForWidth());
        layerSelector->setSizePolicy(sizePolicy1);

        vboxLayout->addWidget(layerSelector);

        spacerItem = new QSpacerItem(200, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);

        mapBox = new MapBox(layoutWidget);
        mapBox->setObjectName(QStringLiteral("mapBox"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(2);
        sizePolicy2.setVerticalStretch(2);
        sizePolicy2.setHeightForWidth(mapBox->sizePolicy().hasHeightForWidth());
        mapBox->setSizePolicy(sizePolicy2);
        mapBox->setMinimumSize(QSize(300, 300));

        vboxLayout->addWidget(mapBox);

        splitter->addWidget(layoutWidget);
        tabWidget = new QTabWidget(splitter);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        log_area = new QWidget();
        log_area->setObjectName(QStringLiteral("log_area"));
        verticalLayout = new QVBoxLayout(log_area);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        logArea = new QTextEdit(log_area);
        logArea->setObjectName(QStringLiteral("logArea"));

        verticalLayout->addWidget(logArea);

        viewResultsButton = new QPushButton(log_area);
        viewResultsButton->setObjectName(QStringLiteral("viewResultsButton"));
        viewResultsButton->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));

        verticalLayout->addWidget(viewResultsButton);

        tabWidget->addTab(log_area, QString());
        model_viewer = new QWidget();
        model_viewer->setObjectName(QStringLiteral("model_viewer"));
        hboxLayout1 = new QHBoxLayout(model_viewer);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        statusView = new QTreeWidget(model_viewer);
        statusView->setObjectName(QStringLiteral("statusView"));

        hboxLayout1->addWidget(statusView);

        tabWidget->addTab(model_viewer, QString());
        plot_info = new QWidget();
        plot_info->setObjectName(QStringLiteral("plot_info"));
        gridLayout = new QGridLayout(plot_info);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        pxInfoArea = new QTextEdit(plot_info);
        pxInfoArea->setObjectName(QStringLiteral("pxInfoArea"));
        pxInfoArea->setOverwriteMode(false);
        pxInfoArea->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        gridLayout->addWidget(pxInfoArea, 0, 0, 1, 1);

        tabWidget->addTab(plot_info, QString());
        splitter->addWidget(tabWidget);

        hboxLayout->addWidget(splitter);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 667, 25));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QStringLiteral("menuView"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        menuAction = new QMenu(menubar);
        menuAction->setObjectName(QStringLiteral("menuAction"));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);
        modelToolBar = new QToolBar(MainWindow);
        modelToolBar->setObjectName(QStringLiteral("modelToolBar"));
        modelToolBar->setOrientation(Qt::Horizontal);
        MainWindow->addToolBar(Qt::TopToolBarArea, modelToolBar);
        fileToolBar = new QToolBar(MainWindow);
        fileToolBar->setObjectName(QStringLiteral("fileToolBar"));
        fileToolBar->setOrientation(Qt::Horizontal);
        MainWindow->addToolBar(Qt::TopToolBarArea, fileToolBar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuAction->menuAction());
        menubar->addAction(menuView->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuView->addAction(actionHideDebugMsgs);
        menuView->addAction(actionFitMap);
        menuHelp->addAction(actionRegMASDocumentation);
        menuHelp->addAction(actionAboutRegMAS);
        menuAction->addAction(actionRun);
        menuAction->addAction(actionPause);
        menuAction->addAction(actionStop);
        menuFile->addAction(actionLoadConfiguration);
        menuFile->addAction(actionSaveLog);
        menuFile->addAction(actionSaveLogAs);
        modelToolBar->addAction(actionRun);
        modelToolBar->addAction(actionPause);
        modelToolBar->addAction(actionStop);
        fileToolBar->addAction(actionLoadConfiguration);
        fileToolBar->addAction(actionSaveLog);
        fileToolBar->addAction(actionExit);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "FFSM - Forest Sector Simulator", nullptr));
        actionLoadConfiguration->setText(QApplication::translate("MainWindow", "&Load Configuration", nullptr));
        actionSaveLog->setText(QApplication::translate("MainWindow", "&Save log", nullptr));
        actionSaveLogAs->setText(QApplication::translate("MainWindow", "Save log &as..", nullptr));
        actionRun->setText(QApplication::translate("MainWindow", "&Run", nullptr));
        actionPause->setText(QApplication::translate("MainWindow", "&Pause / Resume", nullptr));
        actionStop->setText(QApplication::translate("MainWindow", "&Stop", nullptr));
        actionAboutRegMAS->setText(QApplication::translate("MainWindow", "&About RegMAS", nullptr));
        actionExit->setText(QApplication::translate("MainWindow", "&Exit", nullptr));
        actionHideDebugMsgs->setText(QApplication::translate("MainWindow", "Hide &debug messages", nullptr));
        actionRegMASDocumentation->setText(QApplication::translate("MainWindow", "RegMAS &documentation", nullptr));
        actionFitMap->setText(QApplication::translate("MainWindow", "&Fit map in Window", nullptr));
        actionViewResults->setText(QApplication::translate("MainWindow", "goToResults", nullptr));
#ifndef QT_NO_WHATSTHIS
        logArea->setWhatsThis(QApplication::translate("MainWindow", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Run-time logs collecting area (can be saved)</p></body></html>", nullptr));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        viewResultsButton->setToolTip(QApplication::translate("MainWindow", "<html><head/><body><p>You will need a recent version of LibreOffice (or OpenOffice) installed on your system to view the results.</p><p>If you don't have it you can download it from <a href=\"http://www.libreoffice.org\"><span style=\" text-decoration: underline; color:#0000ff;\">http://www.libreoffice.org.</span></a></p><p/></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        viewResultsButton->setText(QApplication::translate("MainWindow", "Go to results", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(log_area), QApplication::translate("MainWindow", "Log area", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = statusView->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("MainWindow", "1", nullptr));
#ifndef QT_NO_WHATSTHIS
        statusView->setWhatsThis(QApplication::translate("MainWindow", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Run-time viewer of important model status variables</p></body></html>", nullptr));
#endif // QT_NO_WHATSTHIS
        tabWidget->setTabText(tabWidget->indexOf(model_viewer), QApplication::translate("MainWindow", "Model viewer", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(plot_info), QApplication::translate("MainWindow", "Plot info", nullptr));
        menuView->setTitle(QApplication::translate("MainWindow", "&View", nullptr));
        menuHelp->setTitle(QApplication::translate("MainWindow", "&Help", nullptr));
        menuAction->setTitle(QApplication::translate("MainWindow", "&Action", nullptr));
        menuFile->setTitle(QApplication::translate("MainWindow", "&File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
