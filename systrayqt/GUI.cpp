#include "build_options.h"

#include <QtGui>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTimer>
#include <QString>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QColorDialog>

#include <QFileDialog>

#include "GUI.h"

// Allow read and write to variables (in gestures.h & pen_location.h)
#include "FreerunnerReader.h"
#include "networkReceiver.h"
#include "gestures.h"
#include "pen_location.h"
#include "main.h"


#ifdef INVENSENSE_EV_ARM
// InvSReader is explicitly here to prevent a circular include dependency
#include "invensenseReader.h"
extern invensenseReader  *InvSReader;
#endif


// Shared class interface
#include "appComms.h"
extern appComms  *Sender;
#include "savedSettings.h"
extern savedSettings *persistentSettings;

static QString maxUsers0Desc;
static QString maxUsers1Desc;
static QString maxUsers2Desc;
static QString maxUsers3Desc;
static QString maxUsers4Desc;
static QString maxUsers5Desc;
static QString maxUsers6Desc;
static QString maxUsers7Desc;
static QString maxUsers8Desc;



// Create the GUI elements. These are:
// 1) A sys tray icon which is always visible.
// 2) A properties window that is usaually hidden

CosnetaGui::CosnetaGui(dongleReader            *dongle,
                       networkReceiver         *network,
                       sysTray_devices_state_t *table)
{
    dongleReadtask = dongle;
    netReadTask    = network;
    sharedTable    = table;

    // load icon
#ifdef Q_OS_WIN
    QIcon icon = QIcon(":/images/cosneta-logo.png");      // Works on windows
#elif defined(Q_OS_MAC)
    QIcon icon = QIcon(":/images/cosneta_logo_vector_bw.svg"); // Want monochrome icon on Mac
#else // Q_OS_UNIX
    QIcon icon = QIcon(":/images/cosneta_logo_vector.svg"); // Tested on Kubuntu
#endif
    setWindowIcon(icon);

    maxUsers0Desc = tr("0 (view only)");
    maxUsers1Desc = tr("1");
    maxUsers2Desc = tr("2");
    maxUsers3Desc = tr("3");
    maxUsers4Desc = tr("4");
    maxUsers5Desc = tr("5");
    maxUsers6Desc = tr("6");
    maxUsers7Desc = tr("7");
    maxUsers8Desc = tr("8");

    currentMaxUsers = 0; // Should be changed to 1, below

    // Create properties window
    createPropertiesWindow();

    // We prefer to be a tray icon, but if we cannot generate one, we'll just need to be a normal app.

    // Removed from Linux build as can cause a segfault. Okay if in main(). TODO: investigate.
    if( ! QSystemTrayIcon::isSystemTrayAvailable() )
    {
        QMessageBox::critical(0, QObject::tr("Cosneta Driver"),
                              QObject::tr("No system tray icon available, so control window always present.") );

        trayIconMenu = NULL;
	
        trayIconPresent = false;
    }
    else
    {
        // build menus
        trayIconMenu = new QMenu(this);

        restoreAction = new QAction(tr("&Restore"), this);
        connect(restoreAction, SIGNAL(triggered()), this, SLOT(showRestoreWindow()));
        hideAction = new QAction(tr("&Hide"), this);
        connect(hideAction, SIGNAL(triggered()), this, SLOT(rejectChanges()));
        trayIconMenu->addAction(restoreAction);

        freeStyleStartAction = new QAction(tr("Launch &FreeStyle"), this);
        connect(freeStyleStartAction, SIGNAL(triggered()), this, SLOT(launchFreestyle()));
#ifdef DETECTS_FREESTYLE_CORRECTLY
        freeStyleStopAction = new QAction(tr("Force FreeStyle to quit"), this);
        connect(freeStyleStopAction, SIGNAL(triggered()), this, SLOT(haltFreestyle()));
#endif
        trayIconMenu->addAction(freeStyleStartAction);

        // Allow the user to set the maximumj number of users
        maxUsersMenu = trayIconMenu->addMenu(tr("&Max Contributors"));
        if( maxUsersMenu != NULL )
        {
            qDebug() << "Add max users menu options.";
            maxUsers0 = new QAction(maxUsers0Desc,this);
            maxUsers0->setCheckable(true);
            connect(maxUsers0, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo0()));
            maxUsersMenu->addAction(maxUsers0);

            maxUsers1 = new QAction(maxUsers1Desc,this);
            maxUsers1->setCheckable(true);
            connect(maxUsers1, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo1()));
            maxUsersMenu->addAction(maxUsers1);

            maxUsers2 = new QAction(maxUsers2Desc,this);
            maxUsers2->setCheckable(true);
            connect(maxUsers2, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo2()));
            maxUsersMenu->addAction(maxUsers2);

            maxUsers3 = new QAction(maxUsers3Desc,this);
            maxUsers3->setCheckable(true);
            connect(maxUsers3, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo3()));
            maxUsersMenu->addAction(maxUsers3);

            maxUsers4 = new QAction(maxUsers4Desc,this);
            maxUsers4->setCheckable(true);
            connect(maxUsers4, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo4()));
            maxUsersMenu->addAction(maxUsers4);

            maxUsers5 = new QAction(maxUsers5Desc,this);
            maxUsers5->setCheckable(true);
            connect(maxUsers5, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo4()));
            maxUsersMenu->addAction(maxUsers5);

            maxUsers6 = new QAction(maxUsers6Desc,this);
            maxUsers6->setCheckable(true);
            connect(maxUsers6, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo6()));
            maxUsersMenu->addAction(maxUsers6);

            maxUsers7 = new QAction(maxUsers7Desc,this);
            maxUsers7->setCheckable(true);
            connect(maxUsers7, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo7()));
            maxUsersMenu->addAction(maxUsers7);

            maxUsers8 = new QAction(maxUsers8Desc,this);
            maxUsers8->setCheckable(true);
            connect(maxUsers8, SIGNAL(triggered(bool)), this, SLOT(setMaxUsersTo8()));
            maxUsersMenu->addAction(maxUsers8);

            // Default option
            setMaxUsersTo1();
        }
        else
        {
            qDebug() << "Failed to create num users menu!!";
        }

        updateAction = new QAction(tr("Check for &Updates"), this);
        updateAction->setCheckable(true);
        updateAction->setChecked(persistentSettings->autoUpdateEnabled());
        connect(updateAction, SIGNAL(triggered(bool)), this, SLOT(updateCheck(bool)));
        trayIconMenu->addAction(updateAction);

        helpAction = new QAction(tr("&Help"), this);
        helpAction->setEnabled(true);
        connect(helpAction, SIGNAL(triggered(bool)), this, SLOT(help()));
        trayIconMenu->addAction(helpAction);
        trayIconMenu->addSeparator();

        quitAction = new QAction(tr("&Quit"), this);
        connect(quitAction, SIGNAL(triggered(bool)), this, SLOT(quitApp(bool)));
        trayIconMenu->addAction(quitAction);

        // set up and show the system tray icon
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(icon);
        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setToolTip(tr("Freestyle Controller"));
        trayIcon->show();

        trayIconPresent = true;
    }
    
    // Create the updater window
    updateWindow = new updater;
    
    // If required, start a new update check in the background...
    downloadMetaDataMgr    = NULL;
    metaDownloadInProgress = false;
    
    if( persistentSettings->autoUpdateEnabled() )
    {
        startUpdateCheck();
    }

    // And keep track of screen size. TODO: register for resize events
    desktop = new QDesktopWidget;

    network->updateScreenSize(desktop->width(),desktop->height());

    // If we couldn't get a tray icon, run as a minimised window
    if( ! trayIconPresent ) showMinimized();

    // Run with freeStyle if selected in the settings
    if( persistentSettings->getAutostartFreestyleEnabled() )
    {
        launchFreestyle();
    }
}

// Max connections menu actions
void CosnetaGui::setMaxUsersTo0() { updateCheckedNumCnx(0); }
void CosnetaGui::setMaxUsersTo1() { updateCheckedNumCnx(1); }
void CosnetaGui::setMaxUsersTo2() { updateCheckedNumCnx(2); }
void CosnetaGui::setMaxUsersTo3() { updateCheckedNumCnx(3); }
void CosnetaGui::setMaxUsersTo4() { updateCheckedNumCnx(4); }
void CosnetaGui::setMaxUsersTo5() { updateCheckedNumCnx(5); }
void CosnetaGui::setMaxUsersTo6() { updateCheckedNumCnx(6); }
void CosnetaGui::setMaxUsersTo7() { updateCheckedNumCnx(7); }
void CosnetaGui::setMaxUsersTo8() { updateCheckedNumCnx(8); }

void CosnetaGui::updateCheckedNumCnx(int optionToSet)
{
    qDebug() << "CosnetaGui: newMaxUsers =" << optionToSet << "old =" << currentMaxUsers;

    if( currentMaxUsers == optionToSet ) return;

    if( ! netReadTask->setMaxNumIPConnections(optionToSet) ) return;

    maxUsers0->setChecked(optionToSet==0);
    maxUsers1->setChecked(optionToSet==1);
    maxUsers2->setChecked(optionToSet==2);
    maxUsers3->setChecked(optionToSet==3);
    maxUsers4->setChecked(optionToSet==4);
    maxUsers5->setChecked(optionToSet==5);
    maxUsers6->setChecked(optionToSet==6);
    maxUsers7->setChecked(optionToSet==7);
    maxUsers8->setChecked(optionToSet==8);

    settings->updateMaxUsers(optionToSet);

    currentMaxUsers = optionToSet;
}

void CosnetaGui::startUpdateCheck(void)
{
    qDebug() << "startUpdateCheck() called. In progress" << metaDownloadInProgress
             << "dateOfLastUpdateCheck:" << persistentSettings->lastUpdateDate() << "now:" <<  QDate::currentDate();

    if( persistentSettings->lastUpdateDate() != QDate::currentDate() && ! metaDownloadInProgress )
    {
        if( downloadMetaDataMgr == NULL )
        {
            downloadMetaDataMgr = new QNetworkAccessManager;
        }
            
        QNetworkRequest request;
        request.setUrl(QUrl(AVAILABLE_UPDATES_URL));

        metaDataRequestReply = downloadMetaDataMgr->get(request);

        downloadedUpdateMetaData.clear();

        connect(downloadMetaDataMgr, SIGNAL(finished(QNetworkReply *)),this,SLOT(downloadFinished(QNetworkReply *)));
        connect(metaDataRequestReply,SIGNAL(readyRead()),              this,SLOT(downloadDataAvailable()));

        metaDownloadInProgress = true;
    }
}

void CosnetaGui::downloadFinished(QNetworkReply *reply)
{
    metaDownloadInProgress = false;

    switch( reply->error() )
    {
    case QNetworkReply::NoError:

        {
            // Remove obfuscation.
            XORPermutor = 0x6C554565;

            QByteArray metaDataTmp;

            for(int i=0; i<downloadedUpdateMetaData.length(); i++)
            {
                metaDataTmp.append( (quint8)((int)downloadedUpdateMetaData.at(i) ^ (int)nextXORByte()) );
            }

            qDebug("First 4 bytes (0x): %02X %02X %02X %02X (hex) len = %d",
                   255 & metaDataTmp[0], 255 & metaDataTmp[1], 255 & metaDataTmp[2], 255 & metaDataTmp[3],
                   metaDataTmp.size() );

            downloadedUpdateMetaData = qUncompress((uchar *)metaDataTmp.constData(),metaDataTmp.size());

            // Let the updater handle this.
            updateWindow->startUpdateDialogueIfValid(downloadedUpdateMetaData);


        // Record last check point - include when complete development
//        dateOfLastUpdateCheck = QDate::currentDate();
//        storeCurrentSettings();

        }
        break;

    default:

        balloonMessage("Failed to check for software updates.");
        qDebug() << "Update check failed to download meta-data:" << reply->errorString();
    }
}

void CosnetaGui::downloadDataAvailable(void)
{
    downloadedUpdateMetaData.append(metaDataRequestReply->readAll());

    qDebug() << "Downloaded" << metaDataRequestReply->size() << ":" << downloadedUpdateMetaData.toHex();
}



void CosnetaGui::createPropertiesWindow(void)
{
    // The [*] is a placeholder for cross-platform data changed tagging
    // TODO: populate setWindowModified() as required/
    this->setWindowTitle("Freestyle Controller");
    resize(800, 630);
    
    // Create the tab views
    propertiesTabs = new QTabWidget();
    settings       = new SettingsTab(dongleReadtask,netReadTask,sharedTable);

    connect(settings,SIGNAL(settingsUpdatedMaxUsers(int)),
            this,    SLOT(updateCheckedNumCnx(int)) );
//    gestures          = new GesturesTab();
//    penSettingsEditor = new editPenDataTab(sharedTable,dongleReadtask);
#ifdef DEBUG_TAB
    debug          = new DebugTab();
    board          = new BoardDevTab(dongleReadtask);
#endif

    // Add the views to the tab item
    propertiesTabs->addTab(settings,              QObject::tr("Properties"));
//    propertiesTabs->addTab(penSettingsEditor,     QObject::tr("Pen Settings"));
//    propertiesTabs->addTab(gestures,              QObject::tr("Gestures"));
#ifdef DEBUG_TAB
    propertiesTabs->addTab(debug,                 QObject::tr("Debug"));
    propertiesTabs->addTab(board,                 QObject::tr("Board Development"));
#endif

    QHBoxLayout *buttonBox = new QHBoxLayout();
    buttonBox->addItem(new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Minimum)); // Push buttons to the right
    startSessionBtn  = new QPushButton(tr("&Start Session"),this);
    updateSessionBtn = new QPushButton(tr("&Apply"),this);
    rejectUpdatesBtn = new QPushButton(tr("&Reject"),this);

    connect(startSessionBtn,  SIGNAL(clicked()), this, SLOT(startSession()));
    connect(updateSessionBtn, SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(rejectUpdatesBtn, SIGNAL(clicked()), this, SLOT(rejectChanges()));

    connect(propertiesTabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));

    buttonBox->addWidget(startSessionBtn);
    buttonBox->addWidget(updateSessionBtn);
    buttonBox->addWidget(rejectUpdatesBtn);

    QVBoxLayout *tabAndMain = new QVBoxLayout;
    tabAndMain->addWidget(propertiesTabs);
    tabAndMain->addLayout(buttonBox);
    
    setLayout(tabAndMain);
}

void CosnetaGui::rebuildTrayMenu(void)
{
    // Check we are running with a tray icon
    if( trayIconMenu == NULL ) return;

    //trayIcon->setContextMenu(trayIconMenuWhenVisible);
    trayIconMenu->clear();
    if( this->isVisible() ) trayIconMenu->addAction(hideAction);
    else                    trayIconMenu->addAction(restoreAction);
#ifdef DETECTS_FREESTYLE_CORRECTLY
    if( freeStyleIsRunning() ) trayIconMenu->addAction(freeStyleStopAction);
    else                       trayIconMenu->addAction(freeStyleStartAction);
#else
    trayIconMenu->addAction(freeStyleStartAction);
#endif
    trayIconMenu->addMenu(maxUsersMenu);
    trayIconMenu->addAction(updateAction);
    trayIconMenu->addAction(helpAction);
    trayIconMenu->addAction(quitAction);

    trayIcon->setContextMenu(NULL);
    trayIcon->setContextMenu(trayIconMenu);
}

void CosnetaGui::setFlipPresent(bool present)
{
    settings->setFlipPresent(present);
}

void CosnetaGui::showInitialScreen(void)
{
    settings->showInitialScreen();

    startSessionBtn->show();
    updateSessionBtn->hide();
    rejectUpdatesBtn->hide();

    show();
}

// Slots
void CosnetaGui::showRestoreWindow()
{
    // Move the other settings stuff in here (TODO)
    settings->retrieveCurrentSettings();

    startSessionBtn->hide();
    updateSessionBtn->show();
    rejectUpdatesBtn->show();

    // Populate with current data
    settings->mouseControlsBox->setChecked(Sender->getSystemDriveState()!=NO_SYSTEM_DRIVE);
    settings->gestureControlsBox->setChecked(Sender->gesturesAreEnabled());

    QString statusString;
    // NOTE BREAK in OOD here!
    netReadTask->connectionStatus(settings->listenerIP,settings->listenerPort, statusString);
    settings->networkReceiverLabel->setText( QString("IP: %1   Port: %2   %3")
                                                    .arg(settings->listenerIP).arg(settings->listenerPort).arg(statusString) );

    // TODO: Select start tab

    // Copy in current pen settings data
    int numPensPresent = 0;
    for(int pen=0; pen<MAX_PENS; pen++)
    {
        if( COS_PEN_IS_ACTIVE(sharedTable->pen[pen]) )
        {
            settings->summaryView[pen]->setEnabled(true);
            settings->summaryView[pen]->setVisible(true);

            sharedTable->settings[pen].users_name[MAX_USER_NAME_SZ-1] = (char)0;
            settings->penName[pen]->setText(QString(sharedTable->settings[pen].users_name));
            QString style = QString("QLabel { background: rgb(%1,%2,%3)}")
                                    .arg(sharedTable->settings[pen].colour[0])
                                    .arg(sharedTable->settings[pen].colour[1])
                                    .arg(sharedTable->settings[pen].colour[2]);
            qDebug() << "Set colour for pen" << pen << "to" << style;
            settings->colourSwatch[pen]->setStyleSheet(style);
            switch(sharedTable->gestCalc[pen].mouseModeSelected())
            {
            case NORMAL_OVERLAY:          settings->penState[pen]->setText(tr("Annotate"));
                                          break;

            case DRIVE_MOUSE:             settings->penState[pen]->setText(tr("Mouse"));
                                          break;

            case PRESENTATION_CONTROLLER: settings->penState[pen]->setText(tr("Presentor"));
                                          break;

            default:                      settings->penState[pen]->setText(tr("??"));
                                          break;
            }

            numPensPresent ++;
        }
        else
        {
            settings->summaryView[pen]->setEnabled(false);
            settings->summaryView[pen]->setVisible(false);
        }
    }
    if( numPensPresent<1 )
    {
        settings->makeDummyNotPresentPenEntry(0);
    }

    settings->populatePointerDriveList();

    // Show it off
    settings->showRestoreScreen();
    showNormal();

    // Repopulate the penSettingsEditor after show() to allow it to reposition
    // the comboBoxes around the diagram.
    if( propertiesTabs->currentWidget() == penSettingsEditor )
    {
        penSettingsEditor->updateForDisplay();
    }

    // Must be after the show() or will put a "Show" rather than a "Hide" entry in
    rebuildTrayMenu();
}

void CosnetaGui::startSession()
{
    applyChanges();
}

void CosnetaGui::applyChanges()
{
    if( propertiesTabs->currentWidget() == penSettingsEditor )
    {
        penSettingsEditor->applyAnyChangedSettings();
    }
    else if( propertiesTabs->currentWidget() == settings )
    {
        settings->applySettingsChanges();
    }

    if( settings->mouseControlsBox->isChecked() )
    {
        int driveIndex = settings->mouseDrivePenID->currentIndex();

        switch( driveIndex )
        {
        case 0:  Sender->setMouseDriveState(DRIVE_SYSTEM_WITH_LEAD_PEN);
                 Sender->setLeadPenIndex( settings->mouseDrivePenID->currentIndex() - 1 );
                 break;

        case 1:  Sender->setMouseDriveState(DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN);
                 break;

        case 2:  Sender->setMouseDriveState(DRIVE_SYSTEM_WITH_ANY_PEN);
                 break;

        default: Sender->setMouseDriveState(NO_SYSTEM_DRIVE);
        }
    }
    else
    {
        Sender->setMouseDriveState(NO_SYSTEM_DRIVE);
    }

    // Copy the new data to the actual working data
    freerunnerGesturesDriveOn = settings->editingGesturesEnabled;
    freerunnerPointerDriveOn  = settings->editingPointerEnabled;
    mouseSensitivity          = settings->editingPointerSensitivityPercent;
#ifdef INVENSENSE_EV_ARM
    bool e = settings->invensenseControlsBox->isChecked();
    int  p = settings->invensensePort->value();

    if( settings->invensenseEnabled && ! e )
    {
        // Stop the device
        InvSReader->stopCommand();

        balloonMessage("Invensense port: "+ QString::number(p)+" stopped.");
    }
    else if( ! settings->invensenseEnabled && e )
    {
        // Start the device
        InvSReader->startCommand(p);

        balloonMessage("Invensense port: "+ QString::number(p)+" started.");
    }
    else if( e && p != settings->currentInvensensePort )
    {
        // Reconfigure
        InvSReader->stopCommand();
        InvSReader->waitForStop();
        InvSReader->startCommand(p);

        balloonMessage("Invensense restarted to port: "+ QString::number(p));
    }

    settings->currentInvensensePort = p;
    settings->invensenseEnabled     = e;

#endif
    // And hide
    if( trayIconPresent ) hide();
    else                  showMinimized();

    rebuildTrayMenu();
}

void CosnetaGui::rejectChanges()
{
    if( trayIconPresent ) hide();
    else                  showMinimized();

    rebuildTrayMenu();
}

void CosnetaGui::closeEvent(QCloseEvent *e)
{
    if( trayIconPresent ) hide();
    else                  showMinimized();

    rebuildTrayMenu();

    e->ignore();
}

void CosnetaGui::launchFreestyle()
{
    if( freeStyleIsRunning() )
    {
        balloonMessage(tr("Cannot launch freeStyle as it is already running."));
    }
    else
    {
        launchFreeStyle();

        rebuildTrayMenu();
    }
}

void CosnetaGui::haltFreestyle()
{
    if( ! freeStyleIsRunning() )
    {
        balloonMessage(tr("Cannot halt freeStyle as it is not running."));
    }
    else
    {
        stopFreeStyle();

        rebuildTrayMenu();
    }
}

void CosnetaGui::updateCheck(bool checked)
{
    // If enabling, and required, kick off a check right now
    if( ! persistentSettings->autoUpdateEnabled() )
    {
        persistentSettings->setAutoUpdateEnabled(false);
        
        startUpdateCheck();
    }
    else // Disable auto-update
    {
        persistentSettings->setAutoUpdateEnabled(false);
    }
    updateAction->setChecked(persistentSettings->autoUpdateEnabled());
    
    // Firmware updates: later (currently don't have hardware)
//    updateWindow->show();
}

void CosnetaGui::help()
{
    QString helpFileName = QString("help") + QDir::separator() + QString("systray_index.html");
    QDesktopServices::openUrl(QString("file:%1").arg(helpFileName));
}

void CosnetaGui::quitApp(bool checked)
{
    // TODO: popup yes/no confirm exit dialogue

    trayIcon->hide();

    updateWindow->hide();
    this->hide();

    QCoreApplication::instance()->quit();
}

void CosnetaGui::tabChanged()
{
    // Have we made anything that needs to be updated visible?
#ifdef DEBUG_TAB
    if( propertiesTabs->currentWidget() == debug )
    {
        debug->debugTabVisible = true;
    }
    else
    {
        debug->debugTabVisible = false;
    }

    if( propertiesTabs->currentWidget() == board )
    {
        board->boardTabVisible = true;
    }
    else
    {
        board->boardTabVisible = false;
    }
#endif
    if( propertiesTabs->currentWidget() == settings )
    {
        // Use this to fill in the current state
        showRestoreWindow();
    }
    else if( propertiesTabs->currentWidget() == penSettingsEditor )
    {
        penSettingsEditor->updateForDisplay();
    }
}


// Display a balloon message for delay seconds using the title
void CosnetaGui::showBalloonMessage(QString messageText, int delay)
{
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);
    trayIcon->showMessage(QObject::tr("Cosneta"), messageText, icon, delay * 1000);
}


void CosnetaGui::penChanged(int penNum)
{
    qDebug() << "penChanged("  << penNum << ") visible: " << isVisible()
             << " now_active=" << COS_PEN_IS_ACTIVE(sharedTable->pen[penNum]);

    if( isVisible() && penNum>=0 && penNum<MAX_PENS )
    {
        sharedTable->settings[penNum].users_name[MAX_USER_NAME_SZ-1] = (char)0;
        settings->penName[penNum]->setText(QString(sharedTable->settings[penNum].users_name));
//        settings->penSensitivitySlider[penNum]->setValue(sharedTable->settings[penNum].sensitivity);
        QString style = QString("QLabel { background: rgb(%1,%2,%3)}")
                                .arg(sharedTable->settings[penNum].colour[0])
                                .arg(sharedTable->settings[penNum].colour[1])
                                .arg(sharedTable->settings[penNum].colour[2]);
        qDebug() << "Set colour for pen" << penNum << "to" << style;
        settings->colourSwatch[penNum]->setStyleSheet(style);
        switch(sharedTable->gestCalc[penNum].mouseModeSelected())
        {
        case NORMAL_OVERLAY:          settings->penState[penNum]->setText(tr("Annotate"));
                                      break;

        case DRIVE_MOUSE:             settings->penState[penNum]->setText(tr("Mouse"));
                                      break;

        case PRESENTATION_CONTROLLER: settings->penState[penNum]->setText(tr("Presentor"));
                                      break;

        default:                      settings->penState[penNum]->setText(tr("??"));
                                      break;
        }

        // TODO: update displayed button mapping for a pen

        bool nowPresent = COS_PEN_IS_ACTIVE(sharedTable->pen[penNum]);

        settings->summaryView[penNum]->setVisible(nowPresent);
        settings->summaryView[penNum]->setEnabled(nowPresent);

        if( ! nowPresent )
        {
            // Check for no entries present, and place the no pens entry at zero

            int numPresent = 0;

            for(int pen=0; pen<MAX_PENS; pen++)
            {
                if( COS_PEN_IS_ACTIVE(sharedTable->pen[penNum]) ) numPresent++;
            }

            // If changed, and count==0, must have decremented to here, as editing
            // should be impossible, so must have decremented by removal, so put in dummy
            if( numPresent<1 )
            {
                settings->makeDummyNotPresentPenEntry(0);
            }
        }

        settings->update();
    }
    else
    {
        if( ! isVisible() )
        {
            qDebug() << QString("Don't update as GUI not visible.");
        }
        else
        {
            balloonMessage(QString("Device %1 out of range.").arg(penNum));
        }
    }
}




SettingsTab::SettingsTab(dongleReader *dongle,
                         networkReceiver *network, sysTray_devices_state_t *tableIn,
                         QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *settingsLayout = new QVBoxLayout;

    dongleReadTask = dongle;
    networkInput   = network;
    sharedTable    = tableIn;

    // Editor for the session name and password
    QGridLayout *namePasswordBox   = new QGridLayout;

    QLabel      *nameLabel = new QLabel(tr("Session Name"));
    namePasswordBox->addWidget(nameLabel,0,0);
    sessionName = new QLineEdit;
    namePasswordBox->addWidget(sessionName,0,1);

    QLabel      *passwordLabel = new QLabel(tr("Session Password"));
    namePasswordBox->addWidget(passwordLabel,1,0);
    sessionPassword = new QLineEdit;
    namePasswordBox->addWidget(sessionPassword,1,1);

    settingsLayout->addLayout(namePasswordBox);

    // Set the maximum number of users that can interact with the session
    QGridLayout *maxInteractorsBox   = new QGridLayout;
    QLabel      *maxUsersLabel = new QLabel(tr("Maximum Number of Controllers"));
    maxUsersCombo = new QComboBox;
    maxUsersCombo->addItem(maxUsers0Desc,0);
    maxUsersCombo->addItem(maxUsers1Desc,1);
    maxUsersCombo->addItem(maxUsers2Desc,2);
    maxUsersCombo->addItem(maxUsers3Desc,3);
    maxUsersCombo->addItem(maxUsers4Desc,4);
    maxUsersCombo->addItem(maxUsers5Desc,5);
    maxUsersCombo->addItem(maxUsers6Desc,6);
    maxUsersCombo->addItem(maxUsers7Desc,7);
    maxUsersCombo->addItem(maxUsers8Desc,8);
    connect(maxUsersCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(maxUsersChanged(int)));
    maxInteractorsBox->addWidget(maxUsersLabel,0,0);
    maxInteractorsBox->addWidget(maxUsersCombo,0,1);

    settingsLayout->addLayout(maxInteractorsBox);

    initialView = false;

    // Here we start the intended final view: a list of the connected pens.
    // In this list, we can change the pen name, enable or disable gestures for
    // that pen and adjust the input sensitivity.
    // All list items are created, but only shown when

    QGroupBox   *penSettingsBox        = new QGroupBox(tr("Connected devices"));
    QVBoxLayout *penSettingsLayout     = new QVBoxLayout();
    QHBoxLayout *penSettingsLeadLayout = new QHBoxLayout();

    useLeadPen = new QCheckBox(tr("Use a lead device"));
    useLeadPen->setChecked(true);
    connect(useLeadPen, SIGNAL(clicked()), this, SLOT(useALeadPenChanged()));
    penSettingsLeadLayout->addWidget(useLeadPen);
    penSettingsLeadLayout->addStretch(0);

    QLabel *leadSelectPolicyLabel = new QLabel(tr("Lead device select policy"));
    penSettingsLeadLayout->addWidget(leadSelectPolicyLabel);

    leadSelectPolicy = new QComboBox;
    leadSelectPolicy->addItem(tr("Use first connected device"),USE_FIRST_CONNECTED_PEN);
    //leadSelectPolicy->addItem(tr("Use first connected local freeRunner"),USE_FIRST_CONNECTED_LOCAL_PEN);
    leadSelectPolicy->addItem(tr("Select manually, below"),MANUAL_PEN_SELECT);
    connect(leadSelectPolicy, SIGNAL(currentIndexChanged(int)), this, SLOT(leadPolicyChanged(int)));
    penSettingsLeadLayout->addWidget(leadSelectPolicy);

    penSettingsLayout->addLayout(penSettingsLeadLayout);

    leadPenGroup = new QButtonGroup;

    // Create an editor for each possible local pen
    for(int pen=0; pen<MAX_PENS; pen++)
    {
        // A row showing a summary of the pen in question
        summaryView[pen] = new QFrame;
        summaryView[pen]->setVisible(false);
        summaryView[pen]->setDisabled(true);

        QHBoxLayout *viewLayout = new QHBoxLayout;

        penName[pen]      = new QLabel("<No device present>"); // Never displayed (we hope)
        penState[pen]     = new QLabel("-");                // Never displayed (we hope)
        colourSwatch[pen] = new QLabel("    ");
        isLeadPen[pen]    = new QCheckBox();
        isLeadPen[pen]->setEnabled(false);
        if( Sender->getLeadPenIndex() == pen ) isLeadPen[pen]->setChecked(true);
        else                                   isLeadPen[pen]->setChecked(false);
        leadPenGroup->addButton(isLeadPen[pen]);

        viewLayout->addWidget(isLeadPen[pen]);
        viewLayout->addSpacing(20);
        viewLayout->addWidget(colourSwatch[pen]);
        viewLayout->addSpacing(10);
        viewLayout->addWidget(penName[pen]);
        viewLayout->addStretch(0);
        viewLayout->addWidget(penState[pen]);
        summaryView[pen]->setLayout(viewLayout);

        penSettingsLayout->addWidget(summaryView[pen]);
    }
    penSettingsBox->setLayout(penSettingsLayout);

    // Special case for first pen: always show it, but inactive
    makeDummyNotPresentPenEntry(0);

    // Here we use 2 groupboxes. The first for mouse control and the second for gestures.

    // The mouse control box allows us to select the pen to control the mouse.
    mouseControlsBox   = new QGroupBox(tr("Allow pointer/presentation control from devices"));
    mouseControlsBox->setCheckable(true);
    editingPointerEnabled = true;                           // TODO: Get current value and populate
    mouseControlsBox->setChecked(editingPointerEnabled);
    connect(mouseControlsBox, SIGNAL(clicked()), this, SLOT(driveMouseStateChanged()) );

    // The gesture box allows TBD??
    presenterControlsBox = new QGroupBox(tr("Enable presentation controls"));
    presenterControlsBox->setCheckable(true);
    presenterModeEnabled = true;                          // TODO: Get current value and populate
    presenterControlsBox->setChecked(presenterModeEnabled);
    connect(presenterControlsBox, SIGNAL(clicked()), this, SLOT(drivePresentationStateChanged()) );
    presenterApplication = new QComboBox;
    presenterApplication->addItem(tr("Adobe Acrobat PDF"),APP_PDF);
    presenterApplication->addItem(tr("PowerPoint"),APP_POWERPOINT);

    // Controls contained within the gestureControlsBox
    QVBoxLayout *presenterLayout               = new QVBoxLayout;
    QHBoxLayout *defaultPresentationTypeLayout = new QHBoxLayout;
    QHBoxLayout *shortcutOpenLayout            = new QHBoxLayout;

    QLabel *presenterControlsText = new QLabel(tr("Quick Open Presentation Application:"));
    defaultPresentationTypeLayout->addWidget(presenterControlsText);
    defaultPresentationTypeLayout->addWidget(presenterApplication);
    QLabel *presenterDocumentText = new QLabel(tr("Quick Open Presentation"));
    shortcutDocumentFilenameText = new QLineEdit(QDir::homePath());
    shortcutDocumentFilenameButton = new QPushButton(tr("Select Document File"));
    connect(shortcutDocumentFilenameButton, SIGNAL(clicked()), this, SLOT(selectDocumentFilename()) );
    shortcutOpenLayout->addWidget(presenterDocumentText);
    shortcutOpenLayout->addWidget(shortcutDocumentFilenameText);
    shortcutOpenLayout->addWidget(shortcutDocumentFilenameButton);

    presentorDrivePenID = new QComboBox;
    populatePresenterDriveList();
    connect(presentorDrivePenID, SIGNAL(currentIndexChanged(int)), this, SLOT(presentationControllerChanged(int)));

    QLabel      *presentorControlLabel   = new QLabel(tr("Device to use:"));
    QHBoxLayout *presentorControlsLayout = new QHBoxLayout;
    presentorControlsLayout->addWidget(presentorControlLabel);
    presentorControlsLayout->addWidget(presentorDrivePenID);

    presenterLayout->addLayout(presentorControlsLayout);

    presenterLayout->addLayout(defaultPresentationTypeLayout);
    presenterLayout->addLayout(shortcutOpenLayout);
    presenterControlsBox->setLayout(presenterLayout);

    // The gesture box allows TBD??
    gestureControlsBox = new QGroupBox(tr("Gesture controls"));
    gestureControlsBox->setCheckable(true);
    editingGesturesEnabled = true;                          // TODO: Get current value and populate
    gestureControlsBox->setChecked(editingGesturesEnabled);
    connect(gestureControlsBox, SIGNAL(clicked()), this, SLOT(useGesturesStateChanged()) );

#ifdef INVENSENSE_EV_ARM
    invensenseControlsBox = new QGroupBox(tr("InvenSense evaluation board controls"));
    invensenseControlsBox->setCheckable(true);
    invensenseEnabled     = false;
#ifdef Q_OS_WIN
    currentInvensensePort = 13;
#else
    currentInvensensePort = 0;
#endif
    invensenseControlsBox->setChecked(invensenseEnabled);
#endif

    // The enable network connections box just shows the connection information.
    networkConnectionBox   = new QGroupBox(tr("Allow network connections"));
    networkConnectionBox->setCheckable(true);
    QHBoxLayout *networkLayout = new QHBoxLayout;

    QString statusString;
    networkInput->connectionStatus(listenerIP, listenerPort, statusString);
    networkReceiverLabel = new QLabel(QString("IP: %1   Port: %2  %3")
                                      .arg(listenerIP).arg(listenerPort).arg(statusString));
    networkLayout->addWidget(networkReceiverLabel);
    networkConnectionBox->setLayout(networkLayout);

    // And add a free checkbox to allow this program to run on login
    autoStartCheck = new QCheckBox(tr("Start on log in."));
    connect(autoStartCheck,     SIGNAL(clicked()), this, SLOT(autoStartChanged()) );

    flipPresentTag = new QLabel();

    // Controls contained within the mouseControlsBox
    QLabel      *mouseControlLabel   = new QLabel(tr("Device to use:"));
    QHBoxLayout *mouseControlsLayout = new QHBoxLayout;
    mouseControlsLayout->addWidget(mouseControlLabel);

    mouseDrivePenID = new QComboBox;
    populatePointerDriveList();
    connect(mouseDrivePenID, SIGNAL(currentIndexChanged(int)), this, SLOT(mouseControllerChanged(int)));
    mouseControlsLayout->addWidget(mouseDrivePenID);

    mouseControlsBox->setLayout(mouseControlsLayout);

    // Controls contained within the gestureControlsBox
    QHBoxLayout *gesturesLayout = new QHBoxLayout;
    QLabel *gestureControlsText = new QLabel(tr("Allow a set of pre-defined gestures to generate input events. "
                                                "These events can be edited in the Gestures tab."));
    gesturesLayout->addWidget(gestureControlsText);
    gestureControlsBox->setLayout(gesturesLayout);

#ifdef INVENSENSE_EV_ARM
    // Allow an input from the InvenSense ARM eval board
    QHBoxLayout *invensenseLayout = new QHBoxLayout;
    invensensePort                = new QSpinBox;
#ifdef Q_OS_WIN
    QLabel *invensenseControlsText = new QLabel(tr("Specify the COM port number:"));
    invensensePort->setRange(0,99);
#else
    QLabel *invensenseControlsText = new QLabel(tr("Will attempt to read from /dev/rfcomm<n>. n="));
    invensensePort->setRange(0,9);
#endif
    invensensePort->setValue(currentInvensensePort);
    invensensePort->setSingleStep(1);

    invensenseLayout->addWidget(invensenseControlsText);
    invensenseLayout->addWidget(invensensePort);
    invensenseControlsBox->setLayout(invensenseLayout);
#endif
    // Auto started applications
    autostartBox = new QGroupBox(tr("Automatically started applications"));
    autostartBox->setCheckable(false);

    autoStartFreeStyle = new QCheckBox(tr("FreeStyle overlay and screen echo application."));
    autoStartFreeStyle->setChecked(true);

    QHBoxLayout *autoStartLayout = new QHBoxLayout;
    autoStartLayout->addWidget(autoStartFreeStyle);

    autostartBox->setLayout(autoStartLayout);

    // Put some space at the bottom (for now)
    QSpacerItem *spacer = new QSpacerItem(1,1, QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Include mouseControls sensitivity slider (in an HBox to give us a label before it)

    settingsLayout->addWidget(penSettingsBox);
    settingsLayout->addWidget(mouseControlsBox);
    settingsLayout->addWidget(presenterControlsBox);
//    settingsLayout->addWidget(gestureControlsBox);
#ifdef INVENSENSE_EV_ARM
    settingsLayout->addWidget(invensenseControlsBox);
#endif
    settingsLayout->addWidget(networkConnectionBox);
    settingsLayout->addWidget(autoStartCheck);
    settingsLayout->addWidget(autostartBox);
    settingsLayout->addWidget(flipPresentTag);
    settingsLayout->addItem(spacer);

    setLayout(settingsLayout);

    setFlipPresent(false);

    retrieveCurrentSettings();
}

// Tray icon menu is updating us, here
void SettingsTab::updateMaxUsers(int newMaxUsers)
{
    maxUsersCombo->setCurrentIndex(newMaxUsers);
}

// Send a notification to the tray icon menu
void SettingsTab::maxUsersChanged(int newMaxUsers)
{
    qDebug() << "Settings tab: newMaxUsers =" << newMaxUsers;

    emit settingsUpdatedMaxUsers(newMaxUsers);
}

void SettingsTab::selectDocumentFilename(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Shortcut Button Presentation File"),
                                shortcutDocumentFilenameText->text(),
                                tr("PDF files (*.pdf);;PowerPoint files (*.ppt *.pps *.pptm *.pptx *.pptsx *.sldx *.sldm"));

    if( fileName.length()>0 )
    {
        shortcutDocumentFilenameText->setText(fileName);
    }
}

void SettingsTab::applySettingsChanges(void)
{
    int selLeadPen = 0;

    persistentSettings->setSessionName(sessionName->text());
    persistentSettings->setSessionPassword(sessionPassword->text());
    networkInput->setNewPassword(sessionPassword->text());
    persistentSettings->setAutostartFreestyleEnabled( autoStartFreeStyle->isChecked() );

    // Lead pens, mouse drive and presentation drive pens.
    if( useLeadPen->isChecked() )
    {
        lead_select_policy_t value = (lead_select_policy_t)(leadSelectPolicy->itemData( leadSelectPolicy->currentIndex() ).toInt() );
        Sender->setLeadPenPolicy(value);
        persistentSettings->setLeadSelectPolicy(value);
        persistentSettings->setLeadPenEnabled(true);
        if( value == MANUAL_PEN_SELECT )
        {
            for(int p=0; p<MAX_PENS; p++)
            {
                if( isLeadPen[p]->isChecked() )
                {
                    selLeadPen = p;
                    break;
                }
            }
            Sender->setLeadPenIndex(selLeadPen);
        }
    }
    else
    {
        Sender->setLeadPenPolicy(NO_LEAD_PEN);
        persistentSettings->setLeadPenEnabled(false);
        persistentSettings->setLeadSelectPolicy(NO_LEAD_PEN);
    }

    // Presentation and mouse drive states
    if( mouseControlsBox->isChecked() )
    {
        system_drive_t value = (system_drive_t)(mouseDrivePenID->itemData( mouseDrivePenID->currentIndex() ).toInt() );
        Sender->setMouseDriveState(value);
        persistentSettings->setMouseControlPolicy(value);
        persistentSettings->setMouseControlEnabled(true);
    }
    else
    {
        Sender->setMouseDriveState(NO_SYSTEM_DRIVE);
        persistentSettings->setMouseControlEnabled(false);
        persistentSettings->setMouseControlPolicy(NO_SYSTEM_DRIVE);
    }

    if( presenterControlsBox->isChecked() )
    {
        system_drive_t value = (system_drive_t)(presentorDrivePenID->itemData( presentorDrivePenID->currentIndex() ).toInt() );
        Sender->setPresentationDriveState(value);
        persistentSettings->setPresentationControlEnabled(true);
        persistentSettings->setPresentationControlPolicy(value);
    }
    else
    {
        Sender->setPresentationDriveState(NO_SYSTEM_DRIVE);
        persistentSettings->setPresentationControlEnabled(false);
        persistentSettings->setPresentationControlPolicy(NO_SYSTEM_DRIVE);
    }

    // Autostart
    if( autoStartCheck->isChecked() != willAutoStart() )
    {
        setAutoStart( autoStartCheck->isChecked() );
    }

    if( QFile::exists(shortcutDocumentFilenameText->text()) )
    {
        // Shortcut presentation file
        persistentSettings->setShortcutPresentationFilename( shortcutDocumentFilenameText->text() );
    }
    else qDebug() << "Selected document" << shortcutDocumentFilenameText->text() << "does not exist. Ignore.";

    persistentSettings->setShortcutPresentationType(
                (application_type_t)presenterApplication->itemData(
                               presenterApplication->currentIndex() ).toInt() );
}

void SettingsTab::showInitialScreen(void)
{
    initialView = true;

    // Hide bits of the display and show others...
    for(int pen=0; pen<MAX_PENS; pen++)
    {
        summaryView[pen]->setVisible(false);
    }
}

void SettingsTab::showRestoreScreen(void)
{
    initialView = false;

    // Hide bits of the display and show others...
    int numShown = 0;
    for(int pen=0; pen<MAX_PENS; pen++)
    {
        if( COS_PEN_IS_ACTIVE(sharedTable->pen[pen]) )
        {
            summaryView[pen]->setVisible( true );
            numShown ++;
        }
        else
        {
            summaryView[pen]->setVisible( false );
        }
    }

    if( numShown < 1 ) summaryView[0]->setVisible( true );
}

void SettingsTab::retrieveCurrentSettings(void)
{
    int i;

    // Autostart
    autoStartCheck->setChecked( willAutoStart() );
    autoStartFreeStyle->setChecked( persistentSettings->getAutostartFreestyleEnabled() );
    sessionName->setText(persistentSettings->getSessionName());
    sessionPassword->setText(persistentSettings->getSessionPassword());

    // Lead/mouse/presentation options
    useLeadPen->setChecked( persistentSettings->getLeadPenEnabled() );
    if( ! persistentSettings->getLeadPenEnabled() )
    {
        Sender->setLeadPenPolicy(NO_LEAD_PEN);;
    }
    else
    {
        Sender->setLeadPenPolicy( persistentSettings->getLeadSelectPolicy() );

        for(i=0; i<leadSelectPolicy->count(); i++)
        {
            if( leadSelectPolicy->itemData(i).toInt() == persistentSettings->getLeadSelectPolicy() )
            {
                leadSelectPolicy->setCurrentIndex(i);
                break;
            }
        }
    }

    mouseControlsBox->setChecked( persistentSettings->getMouseControlEnabled() );
    if( ! persistentSettings->getMouseControlEnabled() )
    {
        Sender->setMouseDriveState(NO_SYSTEM_DRIVE);
    }
    else
    {
        Sender->setMouseDriveState( persistentSettings->getMouseControlPolicy() );

        for(i=0; i<mouseDrivePenID->count(); i++)
        {
            if( mouseDrivePenID->itemData(i).toInt() == persistentSettings->getMouseControlPolicy() )
            {
                mouseDrivePenID->setCurrentIndex(i);
                break;
            }
        }
    }

    presenterControlsBox->setChecked( persistentSettings->getPresentationControlEnabled() );
    if( ! persistentSettings->getPresentationControlEnabled() )
    {
        Sender->setPresentationDriveState(NO_SYSTEM_DRIVE);
    }
    else
    {
        Sender->setPresentationDriveState( persistentSettings->getPresentationControlPolicy() );

        for(i=0; i<presentorDrivePenID->count(); i++)
        {
            if( presentorDrivePenID->itemData(i).toInt() == persistentSettings->getPresentationControlPolicy() )
            {
                presentorDrivePenID->setCurrentIndex(i);
                break;
            }
        }
    }

    shortcutDocumentFilenameText->setText( persistentSettings->shortcutPresentationFilename() );

    for(i=0; i < presenterApplication->count(); i++)
    {
        if( presenterApplication->itemData(i).toInt() == persistentSettings->shortcutPresentationType() )
        {
            presenterApplication->setCurrentIndex(i);
            break;
        }
    }

    if( i>= presenterApplication->count()) qDebug() << "Failed to find presentor software type matching value stored in settings.";
}


void SettingsTab::makeDummyNotPresentPenEntry(int entryNum)
{
    penName[entryNum]->setText( tr("No device connected.") );
    penState[entryNum]->setText("-");
    colourSwatch[entryNum]->setStyleSheet("QLabel { background: rgb(128,128,128) }");

    summaryView[entryNum]->setVisible(true);
    summaryView[entryNum]->setEnabled(false);
}

void SettingsTab::setFlipPresent(bool present)
{
    return;

    if( present ) flipPresentTag->setText("Flip dongle present.");
    else          flipPresentTag->setText("NO Flip dongle present.");

    flipPresentTag->repaint();
}

void SettingsTab::colourSelect(int pen)
{
    QColor newColour = QColorDialog::getColor(currentColour[pen],this,tr("Device Colour"));
    if( newColour.isValid() )
    {
        currentColour[pen] = newColour;
        colourSwatch[pen]->setStyleSheet(QString("QLabel { background: rgb(%1,%2,%3) }")
                                                   .arg(newColour.red())
                                                   .arg(newColour.green())
                                                   .arg(newColour.blue()) );
    }
}

void SettingsTab::driveMouseStateChanged()
{
    editingPointerEnabled = mouseControlsBox->isChecked();

    if( ! editingPointerEnabled )
    {
        mouseDrivePenID->setCurrentIndex(3);
    }
    else
    {
        if( mouseDrivePenID->currentIndex() == 3 )
        {
            // Default to driver with any pen
            mouseDrivePenID->setCurrentIndex(2);
        }
    }
}

// Regenerate the combo list for pen to drive the mouse
void SettingsTab::populatePointerDriveList(void)
{
    mouseDrivePenID->clear();

    mouseDrivePenID->addItem(tr("Only use lead device."),DRIVE_SYSTEM_WITH_LEAD_PEN);
    mouseDrivePenID->addItem(tr("Use any local device that selects to do so."),DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN);
    mouseDrivePenID->addItem(tr("Use any device (local or network) that selects to do so."),DRIVE_SYSTEM_WITH_ANY_PEN);
    mouseDrivePenID->addItem(tr("Do not allow mouse control."),NO_SYSTEM_DRIVE);

    for(int p=0; p<MAX_PENS; p++)
    {
        if( summaryView[p]->isEnabled() )
        {
            mouseDrivePenID->addItem(QString("%1 %2").arg(p).arg(penName[p]->text()));
        }
    }

    // And select the current value
    switch( Sender->getSystemDriveState() )
    {
    case DRIVE_SYSTEM_WITH_ANY_PEN:
        mouseDrivePenID->setCurrentIndex(2);
        mouseControlsBox->setChecked(true);
        break;

    case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:
        mouseDrivePenID->setCurrentIndex(1);
        mouseControlsBox->setChecked(true);
        break;

    case DRIVE_SYSTEM_WITH_LEAD_PEN:
        mouseDrivePenID->setCurrentIndex(0);
        mouseControlsBox->setChecked(true);
        break;

    case NO_SYSTEM_DRIVE:
        mouseDrivePenID->setCurrentIndex(3);
        mouseControlsBox->setChecked(false);
        break;
    }
}

void SettingsTab::drivePresentationStateChanged()
{
    presenterModeEnabled = mouseControlsBox->isChecked();

    if( ! presenterModeEnabled )
    {
        mouseDrivePenID->setCurrentIndex(3);
    }
    else
    {
        if( mouseDrivePenID->currentIndex() == 3 )
        {
            // Default to driver with any pen
            mouseDrivePenID->setCurrentIndex(2);
        }
    }
}

// Regenerate the combo list for pen to drive the mouse
void SettingsTab::populatePresenterDriveList(void)
{
    presentorDrivePenID->clear();

    presentorDrivePenID->addItem(tr("Only use lead device."),DRIVE_SYSTEM_WITH_LEAD_PEN);
    presentorDrivePenID->addItem(tr("Use any local device that selects to do so."),DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN);
    presentorDrivePenID->addItem(tr("Use any device (local or network) that selects to do so."),DRIVE_SYSTEM_WITH_ANY_PEN);
    presentorDrivePenID->addItem(tr("Do not allow presentation control."),NO_SYSTEM_DRIVE);

    for(int p=0; p<MAX_PENS; p++)
    {
        if( summaryView[p]->isEnabled() )
        {
            presentorDrivePenID->addItem(QString("%1 %2").arg(p).arg(penName[p]->text()));
        }
    }

    // And select the current value
    switch( Sender->getPresentationDriveState() )
    {
    case DRIVE_SYSTEM_WITH_ANY_PEN:
        presentorDrivePenID->setCurrentIndex(2);
        presenterControlsBox->setChecked(true);
        break;

    case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:
        presentorDrivePenID->setCurrentIndex(1);
        presenterControlsBox->setChecked(true);
        break;

    case DRIVE_SYSTEM_WITH_LEAD_PEN:
        presentorDrivePenID->setCurrentIndex(0);
        presenterControlsBox->setChecked(true);
        break;

    case NO_SYSTEM_DRIVE:
        presentorDrivePenID->setCurrentIndex(3);
        presenterControlsBox->setChecked(false);
        break;
    }
}

// A new value for the pen to control the mouse has been selected
void SettingsTab::leadPolicyChanged(int index)
{
    bool  enableManualSelect;

    if( index == 2 )    enableManualSelect = true;
    else                enableManualSelect = false;

    for(int p=0; p<MAX_PENS; p++)
    {
        isLeadPen[p]->setEnabled(enableManualSelect);
    }

    qDebug() << "leadPolicyChanged. Index =" << index;
}

// A new value for the pen to control the mouse has been selected
void SettingsTab::mouseControllerChanged(int index)
{
    mouseDriveIndex = index;
}

// A new value for the pen to control the mouse has been selected
void SettingsTab::presentationControllerChanged(int index)
{
    presentationDriveIndex = index;
}


void SettingsTab::useALeadPenChanged()
{
    bool  enableManualSelect = false;

    if( useLeadPen->isChecked() )
    {
        leadSelectPolicy->setEnabled(true);

        if( leadSelectPolicy->currentIndex() != 2 ) enableManualSelect = true;
    }
    else
    {
        leadSelectPolicy->setEnabled(false);
    }

    // Enable or disable all the lead pen controls based on this

    for(int p=0; p<MAX_PENS; p++)
    {
        isLeadPen[p]->setEnabled(enableManualSelect);
    }
}

void SettingsTab::useGesturesStateChanged()
{
    editingGesturesEnabled = mouseControlsBox->isChecked();
}

void SettingsTab::autoStartChanged()
{
    // Do something
    balloonMessage("autoStartChanged.");
}


void SettingsTab::pointerSensitivityChanged(int value)
{
    editingPointerSensitivityPercent = value;
}





editPenDataTab::editPenDataTab(sysTray_devices_state_t *table, dongleReader *dongleTask, QWidget *parent) : QWidget(parent)
{
    // Firstly, create the components for the editor
    QLabel    *selectLabel = new QLabel(tr("Device to Update"));
    penList = new QComboBox;
    penList->addItem("No devices connected.");
    connect(penList, SIGNAL(currentIndexChanged(int)), this, SLOT(currentPenNumChanged(int)) );

    // Pen name fields
    QLabel    *nameLabel  = new QLabel(tr("Device Name"));
    penName = new QLineEdit();

    QGridLayout *generalPenLayout = new QGridLayout;
    generalPenLayout->setHorizontalSpacing(10);

    // Controls for sensitifity
    QLabel  *sensitivityLabel  = new QLabel(tr("Sensitivity"));
    sensitivitySlider = new QSlider(Qt::Horizontal);

    sensitivitySlider->setRange(0,100);
    sensitivitySlider->setValue(50);

    // Pen preferred colour button (clicking this will pop-up a colour selector)
    QLabel      *colourLabel    = new QLabel(tr("Draw Colour"));
    colourSelector = new QPushButton();
    connect(colourSelector,SIGNAL(clicked()),this, SLOT(colourSelect()));

    // General pen edit: dropdown list of pens available
    //                   pen name
    //                   pen sensitivity
    //                   pen default colour
    generalPenLayout->addWidget(selectLabel,       0, 0);
    generalPenLayout->addWidget(penList,           0, 1);
    generalPenLayout->addWidget(nameLabel,         1, 0);
    generalPenLayout->addWidget(penName,           1, 1);
    generalPenLayout->addWidget(sensitivityLabel,  2, 0);
    generalPenLayout->addWidget(sensitivitySlider, 2, 1);
    generalPenLayout->addWidget(colourLabel,       3, 0);
    generalPenLayout->addWidget(colourSelector,    3, 1);

    buttonContainer = new QWidget;
    buttonContainer->resize(800,600);

    // Do this first so it ends up under the other widgets
    penImageLabel = new QLabel(buttonContainer);
    penImageFull  = new QPixmap(":/images/freerunner_drawing.png");
    if( penImageFull->isNull() )
    {
        balloonMessage("Settings Editor Error: Failed to read PNG diagram of FreeRunner.");
        qDebug() << "Settings Editor Error: Failed to read PNG diagram of FreeRunner.";
    }
    // Scale to current size
    penImageLabel->setPixmap(penImageFull->scaledToHeight(100));

    // Button editor: vertical selectors | image | vertical selectors
    buttonMouse1Options        = createButtonOptionsCombo(buttonContainer);
    buttonMouse2Options        = createButtonOptionsCombo(buttonContainer);
    buttonMouse3Options        = createButtonOptionsCombo(buttonContainer);
    buttonUpOptions            = createButtonOptionsCombo(buttonContainer);
    buttonDownOptions          = createButtonOptionsCombo(buttonContainer);
    buttonModeOptions          = createButtonOptionsCombo(buttonContainer);
    buttonOptAOptions          = createButtonOptionsCombo(buttonContainer);
    buttonOptBOptions          = createButtonOptionsCombo(buttonContainer);
//    buttonSurfaceDetectOptions = createButtonOptionsCombo(leftButtonContainer);   // Maybe?

    // Register a callback for each (I'd love a callback that gave a reference to the object, but ho hum).
    connect(buttonMouse1Options, SIGNAL(currentIndexChanged(int)), this, SLOT(buttonMouse1OptionsIndexChanged(int)) );
    connect(buttonMouse2Options, SIGNAL(currentIndexChanged(int)), this, SLOT(buttonMouse2OptionsIndexChanged(int)) );
    connect(buttonMouse3Options, SIGNAL(currentIndexChanged(int)), this, SLOT(buttonMouse3OptionsIndexChanged(int)) );
    connect(buttonUpOptions,     SIGNAL(currentIndexChanged(int)), this, SLOT(buttonUpOptionsIndexChanged(int)) );
    connect(buttonDownOptions,   SIGNAL(currentIndexChanged(int)), this, SLOT(buttonDownOptionsIndexChanged(int)) );
    connect(buttonModeOptions,   SIGNAL(currentIndexChanged(int)), this, SLOT(buttonModeOptionsIndexChanged(int)) );
    connect(buttonOptAOptions,   SIGNAL(currentIndexChanged(int)), this, SLOT(buttonOptAOptionsIndexChanged(int)) );
    connect(buttonOptBOptions,   SIGNAL(currentIndexChanged(int)), this, SLOT(buttonOptBOptionsIndexChanged(int)) );

    // Also store an array of pointers to these combo boxes so we can do list access
    for(int idx=0; idx<(sizeof(buttons)/sizeof(QComboBox *)); idx++) buttons[idx] = NULL;

    buttons[0] = buttonMouse1Options;
    buttons[1] = buttonMouse2Options;
    buttons[2] = buttonMouse3Options;
    buttons[3] = buttonUpOptions;
    buttons[4] = buttonDownOptions;
    buttons[5] = buttonModeOptions;
    buttons[6] = buttonOptAOptions;
    buttons[7] = buttonOptBOptions;

    // Position dropdowns to line up with dotted lines
    buttonMouse1Options->move(100,0);    buttonMouse2Options->move(100,20);
    buttonDownOptions->move(100,40);     buttonModeOptions->move(100,60);

    buttonMouse3Options->move(100,0);    buttonUpOptions->move(100,20);
    buttonOptAOptions->move(100,40);     buttonOptBOptions->move(100,60);
    // These will all be changed by a repositioning when they are shown (our code)

    // Overall layout: dropdown list of pens available
    //                 deneral pen editor
    //                 button editor
    QVBoxLayout *editPenLayout = new QVBoxLayout;

    editPenLayout->addLayout(generalPenLayout);
    editPenLayout->addWidget(buttonContainer);

    setLayout(editPenLayout);

    dongleReadtask     = dongleTask;
    sharedTable        = table; // global table of pen data
    currentPenIndex    = -1;    // No pen
    ignoreComboChanges = false;
}



QComboBox *editPenDataTab::createButtonOptionsCombo(QWidget *parent)
{
    QComboBox *newOptions = new QComboBox(parent);

    newOptions->addItem(tr("Button 1, Left mouse"),   QVariant(BUTTON_LEFT_MOUSE_IDX) );
    newOptions->addItem(tr("Button 2, Middle mouse"), QVariant(BUTTON_MIDDLE_MOUSE_IDX) );
    newOptions->addItem(tr("Button 3, Right mouse"),  QVariant(BUTTON_RIGHT_MOUSE_IDX) );

    newOptions->addItem(tr("Up Menu, Next slide"), QVariant(BUTTON_SCROLL_UP_IDX) );
    newOptions->addItem(tr("Down Menu, Last slide"),  QVariant(BUTTON_SCROLL_DOWN_IDX) );

    newOptions->addItem(tr("Mode switch"), QVariant(BUTTON_MODE_SWITCH_IDX) );

    newOptions->addItem(tr("Opt A"), QVariant(BUTTON_OPT_A_IDX) );
    newOptions->addItem(tr("Opt B"), QVariant(BUTTON_OPT_B_IDX) );

    return newOptions;
}

void editPenDataTab::currentPenNumChanged(int index)
{
    if( index == currentPenIndex || index<0 || index>BUTTON_OPT_B_IDX ) return;

    currentPenIndex = index;

    populateEditWithData();
}


// Position percentage * 10 (ie 0..1000)
#define LEFT_TICK_PCT_1  122
#define LEFT_TICK_PCT_2  244
#define LEFT_TICK_PCT_3  366
#define LEFT_TICK_PCT_4  488

#define RIGHT_TICK_PCT_1  122
#define RIGHT_TICK_PCT_2  244
#define RIGHT_TICK_PCT_3  390
#define RIGHT_TICK_PCT_4  537


// This is horribly hacked (hence the *110/100 & -60 values and
// the commented out test...

void editPenDataTab::rescaleEditTab(void)
{
    // Position the boxes for this window size. Firstly ensure the containers
    // are the right size or else they will be centered vertically.

    //buttonContainer->resize(this->height(),this->width()); // this is done for us...
    // Scaling is a hack
    int parentWidth  = this->width()  * 100/100;
    int parentHeight = this->height() *  85/100; // 110/100 is a hack
    int scaledImageWidth;

    if( penImageFull->height() )
    {
        scaledImageWidth = (penImageFull->width() * parentHeight) / penImageFull->height();
    }
    else
    {
        QMessageBox::warning(NULL, "Internal sysTray error",
                                   "penImageFull->height() = 0 !!");
        scaledImageWidth = 100;
    }

    penImageLabel->setGeometry((this->width() - scaledImageWidth)/2,0,
                               scaledImageWidth,parentHeight);
    penImageLabel->setPixmap(penImageFull->scaledToHeight(parentHeight));

    // Scale the percentages by this->height() so it scales appropriately
    int comboHtOffset = buttonMouse1Options->height()/2;

    int comboLeftPos  = (parentWidth - scaledImageWidth)/2 // Left edge of image
                        - buttonMouse1Options->width();  // *2 is a hack
    if( comboLeftPos<0 )
    {
        comboLeftPos = 0;
    }

    int comboRightPos  = (parentWidth + scaledImageWidth)/2; // right of image
    if( (comboRightPos+buttonMouse3Options->width()) > parentWidth )
    {
        comboRightPos = parentWidth - buttonMouse3Options->width();
        if( comboRightPos<0 ) comboRightPos = 0;
    }

    // Hacks
    parentHeight  =  this->height() *85/100; //  *85/100;
    comboHtOffset -= 0; // -= 25;

    buttonMouse1Options->move(comboLeftPos, parentHeight*LEFT_TICK_PCT_1/1000 - comboHtOffset);
    buttonMouse2Options->move(comboLeftPos, parentHeight*LEFT_TICK_PCT_2/1000 - comboHtOffset);
    buttonDownOptions->move(comboLeftPos, parentHeight*LEFT_TICK_PCT_3/1000 - comboHtOffset);
    buttonModeOptions->move(comboLeftPos, parentHeight*LEFT_TICK_PCT_4/1000 - comboHtOffset);

    buttonMouse3Options->move(comboRightPos, parentHeight*RIGHT_TICK_PCT_1/1000 - comboHtOffset);
    buttonUpOptions->move(comboRightPos, parentHeight*RIGHT_TICK_PCT_2/1000 - comboHtOffset);
    buttonOptAOptions->move(comboRightPos, parentHeight*RIGHT_TICK_PCT_3/1000 - comboHtOffset);
    buttonOptBOptions->move(comboRightPos, parentHeight*RIGHT_TICK_PCT_4/1000 - comboHtOffset);
}

void editPenDataTab::populateEditWithData(void)
{
    ignoreComboChanges = true;

    if( currentPenIndex < 0 || ! COS_PEN_IS_ACTIVE(sharedTable->pen[currentPenIndex]) )
    {
        // Fill in dummy values & make entries not editable
        penName->setText(tr("No FreeRunner connected."));
        penName->setEnabled(false);

        sensitivitySlider->setValue(50);
        sensitivitySlider->setEnabled(false);

        colourSelector->setStyleSheet("QPushButton { background: rgb(128,128,128) }");
        colourSelector->setEnabled(false);

        for(int buttonNum=0; buttonNum<=BUTTON_OPT_B_IDX; buttonNum ++)
        {
            buttons[buttonNum]->setCurrentIndex(buttonNum);
            buttons[buttonNum]->setEnabled(false);
        }
    }
    else
    {
        penList->setCurrentIndex(currentPenIndex);

        // Show settings for selected pen in the editor
        sharedTable->settings[currentPenIndex].users_name[MAX_USER_NAME_SZ-1] = (char)0;
        penName->setText(QString(sharedTable->settings[currentPenIndex].users_name));
        penName->setEnabled(true);

        sensitivitySlider->setValue(sharedTable->settings[currentPenIndex].sensitivity);
        sensitivitySlider->setEnabled(true);

        QString style = QString("QPushButton { background: rgb(%1,%2,%3)}")
                                .arg(sharedTable->settings[currentPenIndex].colour[0])
                                .arg(sharedTable->settings[currentPenIndex].colour[1])
                                .arg(sharedTable->settings[currentPenIndex].colour[2]);
        qDebug() << "Set colour for pen" << currentPenIndex << "to" << style;
        colourSelector->setStyleSheet(style);
        colourSelector->setEnabled(true);
        penColour = QColor(sharedTable->settings[currentPenIndex].colour[0],
                           sharedTable->settings[currentPenIndex].colour[1],
                           sharedTable->settings[currentPenIndex].colour[2]);

        for(int buttonNum=0; buttonNum<=BUTTON_OPT_B_IDX; buttonNum++)
        {
            buttons[buttonNum]->setCurrentIndex(sharedTable->settings[currentPenIndex].button_order[buttonNum]);
        }

//        optionsFlags = sharedTable->settings[currentPenIndex].options_flags;
    }

    ignoreComboChanges = false;
}

void editPenDataTab::updateForDisplay(void)
{
    // Adjust for window size (doesn't work)
    rescaleEditTab();

    // Update edit entry:
    if( currentPenIndex != -1 || currentPenIndex > MAX_PENS )
    {
        if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[currentPenIndex]) )
        {
            currentPenIndex = -1;
        }
    }

    // Rebuild pen selector combo and choose first if one isn't selected
    penList->clear();

    for(int search=0; search<MAX_PENS; search++ )
    {
        if( COS_PEN_IS_ACTIVE(sharedTable->pen[search]) )
        {
            sharedTable->settings[search].users_name[MAX_USER_NAME_SZ-1] = (char)0;
            penList->addItem(QString(sharedTable->settings[search].users_name));
            if( currentPenIndex < 0 ) currentPenIndex = search;
        }
        else
        {
            penList->addItem(tr("<No device>"));
        }
    }

    // Populate it for the current data
    populateEditWithData();
}


void editPenDataTab::optionsIndexChanged(int userBoxChanged, int newIndex)
{
    if( ignoreComboChanges ) return;

    ignoreComboChanges = true;

    // Empiracally, newIndex is the previous value for userBoxChanged. Querying the
    // buttons[userBoxChanged]->currentIndex() returns the new value.

    int updatedValue = buttons[userBoxChanged]->currentIndex();

    int boxUnsingNewIndex;

    // Find any other combo box mapping using this value

    for(boxUnsingNewIndex=0; boxUnsingNewIndex<=BUTTON_OPT_B_IDX; boxUnsingNewIndex ++)
    {
//        qDebug() << boxUnsingNewIndex << "comboBox[" << boxUnsingNewIndex << "] selecting"
//                 << buttons[boxUnsingNewIndex]->currentIndex();

        if( userBoxChanged  !=  boxUnsingNewIndex    &&
            buttons[boxUnsingNewIndex]->currentIndex() == updatedValue )
        {
            break;
        }
    }

    qDebug() << "Change box [" << userBoxChanged << "] to new index"  << updatedValue
             << "Other box using this index is"  << boxUnsingNewIndex
             << " New value (parameter)"         << newIndex;

    if( boxUnsingNewIndex>BUTTON_OPT_B_IDX )
    {
         qDebug() << "Failed to find matching comboBox for change to box #" << userBoxChanged;

         return;
    }

    int  loop;
    bool taken[BUTTON_OPT_B_IDX+1];

    for( loop=0; loop<=BUTTON_OPT_B_IDX; loop++ ) taken[loop] = false;

    for( loop=0; loop<=BUTTON_OPT_B_IDX; loop++ )
    {
        int  dropIndex = buttons[loop]->currentIndex();

        if( dropIndex >= 0 && dropIndex <= BUTTON_OPT_B_IDX)
        {
            taken[buttons[loop]->currentIndex()] = true;

//            qDebug() << "- " << loop << "comboBox[" << dropIndex << "] = false";
        }
        else qDebug() << "Unexpected combo box" << loop << "value" << dropIndex;
    }

    for( loop=0; loop<=BUTTON_OPT_B_IDX; loop++ )
    {
        if( ! taken[loop] )
        {
            qDebug() << "Set box" << boxUnsingNewIndex << "to" << loop;

            ignoreComboChanges = true;

            buttons[boxUnsingNewIndex]->setCurrentIndex(loop);

            ignoreComboChanges = false;

            break;
        }
    }
}


// Yuck ! Maybe use a map later...
void editPenDataTab::buttonMouse1OptionsIndexChanged(int index) { optionsIndexChanged(0,index); }
void editPenDataTab::buttonMouse2OptionsIndexChanged(int index) { optionsIndexChanged(1,index); }
void editPenDataTab::buttonMouse3OptionsIndexChanged(int index) { optionsIndexChanged(2,index); }
void editPenDataTab::buttonUpOptionsIndexChanged(int index)     { optionsIndexChanged(3,index); }
void editPenDataTab::buttonDownOptionsIndexChanged(int index)   { optionsIndexChanged(4,index); }
void editPenDataTab::buttonModeOptionsIndexChanged(int index)   { optionsIndexChanged(5,index); }
void editPenDataTab::buttonOptAOptionsIndexChanged(int index)   { optionsIndexChanged(6,index); }
void editPenDataTab::buttonOptBOptionsIndexChanged(int index)   { optionsIndexChanged(7,index); }


void editPenDataTab::colourSelect(void)
{
    QColor newColour = QColorDialog::getColor(penColour,this,tr("Device Colour"));
    if( newColour.isValid() )
    {
        penColour = newColour;
        colourSelector->setStyleSheet(QString("QPushButton { background: rgb(%1,%2,%3) }")
                                                   .arg(newColour.red())
                                                   .arg(newColour.green())
                                                   .arg(newColour.blue()) );
    }
    else qDebug() << "colourSelect(): Invalid colour returned.";
}

void editPenDataTab::applyAnyChangedSettings(void)
{
    if( currentPenIndex<0 || currentPenIndex>= MAX_PENS )
    {
        qDebug() << "Failed to save settings for bad pen number:" << currentPenIndex;
        return;
    }

    // Check for changes
    bool changed = false;

    if( QString(sharedTable->settings[currentPenIndex].users_name) != penName->text() )
    {
        changed = true;
    }
    else if( sharedTable->settings[currentPenIndex].sensitivity != sensitivitySlider->value() )
    {
        qDebug() << "Pen sensitivity changed.";
        changed = true;
    }
    else if( penColour.red()   != sharedTable->settings[currentPenIndex].colour[0] ||
             penColour.green() != sharedTable->settings[currentPenIndex].colour[1] ||
             penColour.blue()  != sharedTable->settings[currentPenIndex].colour[2] )
    {
        changed = true;
    }
//    else if(  sharedTable->settings[currentPenIndex].options_flags != optionsFlags )
//    {
//        changed = true;
//    }
    else
    {
        for(int btn=0; btn<=BUTTON_OPT_B_IDX; btn ++)
        {
            if( buttons[btn]->currentIndex() !=
                    sharedTable->settings[currentPenIndex].button_order[btn] )
            {
                changed = true;
                break;
            }
        }
    }

    if( changed )
    {
        // Cosntruct a settings blob and apply it.
        // Update settings for this pen
        pen_settings_t penSettings;
        int            btn;

        memset(&penSettings, 0, sizeof(penSettings));
        strncpy(penSettings.users_name, penName->text().toLocal8Bit(),
                MAX_USER_NAME_SZ-1); // leave a null at the end
        penSettings.sensitivity = sensitivitySlider->value();
        penSettings.colour[0]   = penColour.red();
        penSettings.colour[1]   = penColour.green();
        penSettings.colour[2]   = penColour.blue();
        for(btn=0; btn<=BUTTON_OPT_B_IDX; btn++ )
        {
            penSettings.button_order[btn] = buttons[btn]->currentIndex();
        }
        for(;btn<NUM_MAPPABLE_BUTTONS;btn++)
        {
            penSettings.button_order[btn] = btn;
        }
//        penSettings.options_flags = optionsFlags;

        qDebug() << "Update pen settings, colour = " << penColour;

        if( dongleReadtask == NULL )
        {
            qDebug() << "Internal error: dongleReadtask == NULL";
        }
        else if( ! dongleReadtask->updatePenSettings(currentPenIndex, &penSettings) )
        {
            // This can happen if it's a network pen. Pop-up a balloon message
            // inside dongleReader if it's a comms to local hardware problem
            qDebug() << "Failed to apply new settings for pen" << currentPenIndex
                     << "name" << penSettings.users_name;
        }
        else
        {
            // Update shared table to reflect the new values
            strncpy(sharedTable->settings[currentPenIndex].users_name,
                    penSettings.users_name,                 MAX_USER_NAME_SZ-1);
            sharedTable->settings[currentPenIndex].users_name[MAX_USER_NAME_SZ-1] = (char)0;
            sharedTable->settings[currentPenIndex].sensitivity = penSettings.sensitivity;
            sharedTable->settings[currentPenIndex].colour[0]   = penSettings.colour[0];
            sharedTable->settings[currentPenIndex].colour[1]   = penSettings.colour[1];
            sharedTable->settings[currentPenIndex].colour[2]   = penSettings.colour[2];
            for(int btn=0; btn<NUM_MAPPABLE_BUTTONS; btn++ )
            {
                sharedTable->settings[currentPenIndex].button_order[btn] = penSettings.button_order[btn];
            }
//            sharedTable->settings[currentPenIndex].options_flags = options_flags;
        }
    }
}





GesturesTab::GesturesTab(QWidget *parent) : QWidget(parent)
{
}




#ifdef DEBUG_TAB

extern bool startInvensenseLogging(void);
extern void stopInvensenseLogging(void);

#define SAVE_TO_DISK_TEXT      tr("Save samples to disk")
#define STOP_SAVE_TO_DISK_TEXT tr("Stop saving to disk ")

#define SHOW_LOW_LEVEL_DEBUG   tr("Start low level debug")
#define HIDE_LOW_LEVEL_DEBUG   tr("Stop low level debug ")

#define SAVE_GESTURES_TO_DISK_TEXT      tr("Save gestures to disk")
#define STOP_SAVE_GESTURES_TO_DISK_TEXT tr("Stop saving gestures ")

#define GENERATE_SAMPLE_POINTER_DATA_TEXT        tr("Generate sample data   ")
#define STOP_GENERATING_SAMPLE_POINTER_DATA_TEXT tr("Stop making sample data")

#define USE_MOUSE_DATA_TEXT        tr("Use mouse data   ")
#define STOP_USING_MOUSE_DATA_TEXT tr("Stop using mouse data")

#ifdef INVENSENSE_EV_ARM
#define LOG_INVS_TO_DISK_TEXT      tr("Start invensense logging")
#define STOP_LOG_INVS_TO_DISK_TEXT tr("Stop invensense logging")
#endif

DebugTab::DebugTab(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *topButtons = new QHBoxLayout;

    saveFileButton = new QPushButton(SAVE_TO_DISK_TEXT);
    connect(saveFileButton, SIGNAL(clicked()), this, SLOT(saveDataButtonClicked()) );

    diskSaveActive = false;

    showDebugButton = new QPushButton(SHOW_LOW_LEVEL_DEBUG);
    connect(showDebugButton, SIGNAL(clicked()), this, SLOT(showDebugButtonClicked()) );

    showingDebug = false;

    saveGesturesButton = new QPushButton(SAVE_GESTURES_TO_DISK_TEXT);
    connect(saveGesturesButton, SIGNAL(clicked()), this, SLOT(saveGesturesButtonClicked()) );

    gestureSaveActive = false;

    generateDataButton = new QPushButton(GENERATE_SAMPLE_POINTER_DATA_TEXT);
    connect(generateDataButton, SIGNAL(clicked()), this, SLOT(generateDataButtonClicked()) );

    generateSampleData = false;

    useMouseDataButton = new QPushButton(USE_MOUSE_DATA_TEXT);
    connect(useMouseDataButton, SIGNAL(clicked()), this, SLOT(useMouseDataButtonClicked()) );

    useMouseData = false;

#ifdef INVENSENSE_EV_ARM
    invensenseLogButton = new QPushButton(LOG_INVS_TO_DISK_TEXT);
    connect(invensenseLogButton, SIGNAL(clicked()), this, SLOT(invensenseLogButtonClicked()) );

    invensenseLoggingOn = false;
#endif
    QSpacerItem *spacer = new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Minimum);   // Space to right
    topButtons->addWidget(saveFileButton);
    topButtons->addWidget(showDebugButton);
    topButtons->addWidget(saveGesturesButton);
    topButtons->addWidget(generateDataButton);
    topButtons->addWidget(useMouseDataButton);
#ifdef INVENSENSE_EV_ARM
    topButtons->addWidget(invensenseLogButton);
#endif
    topButtons->addItem(spacer);

    debugText = new QPlainTextEdit;
    debugText->appendPlainText(tr("Debug output\n"));
    debugText->setReadOnly(true);
    
    QVBoxLayout *debugLayout = new QVBoxLayout;
    debugLayout->addItem(topButtons);
    debugLayout->addWidget(debugText);

    setLayout(debugLayout);
    
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateDebug()));
    timer->start(1000);						// 1 second refresh
}

void  DebugTab::saveDataButtonClicked()
{
    if( diskSaveActive )
    {
        qDebug() << "stopDataSaving() - to add to dongleReader.";

        saveFileButton->setText(SAVE_TO_DISK_TEXT);
        diskSaveActive = false;
    }
    else // Not currently saving to disk
    {
        qDebug() << "startDataSaving() - to add to dongleReader.";

        saveFileButton->setText(STOP_SAVE_TO_DISK_TEXT);
        diskSaveActive = true;
    }
}

void DebugTab::showDebugButtonClicked()
{
    if( showingDebug )
    {
        showDebugButton->setText(SHOW_LOW_LEVEL_DEBUG);

        showingDebug = false;
    }
    else
    {
        showDebugButton->setText(HIDE_LOW_LEVEL_DEBUG);

        showingDebug = true;
    }
}

void  DebugTab::saveGesturesButtonClicked()
{
    if( gestureSaveActive )
    {
        stopGestureSaving();

        saveGesturesButton->setText(SAVE_GESTURES_TO_DISK_TEXT);

        gestureSaveActive = false;
    }
    else // Not currently saving to disk
    {
        if( ! startGestureSaving() ) return;

        saveGesturesButton->setText(STOP_SAVE_GESTURES_TO_DISK_TEXT);

        gestureSaveActive = true;
    }
}

void  DebugTab::generateDataButtonClicked()
{
    if( generateSampleData )
    {
        generateDataButton->setText(GENERATE_SAMPLE_POINTER_DATA_TEXT);

        generateSampleData = false;
    }
    else // Start generating sample data
    {
        generateDataButton->setText(STOP_GENERATING_SAMPLE_POINTER_DATA_TEXT);

        generateSampleData = true;
    }
}

void  DebugTab::useMouseDataButtonClicked()
{
    if( useMouseData )
    {
        useMouseDataButton->setText(USE_MOUSE_DATA_TEXT);

        useMouseData = false;
    }
    else // Start generating sample data
    {
        useMouseDataButton->setText(STOP_USING_MOUSE_DATA_TEXT);

        useMouseData = true;
    }
}


#ifdef INVENSENSE_EV_ARM
void  DebugTab::invensenseLogButtonClicked()
{
    if( invensenseLoggingOn )
    {
        stopInvensenseLogging();

        invensenseLogButton->setText(LOG_INVS_TO_DISK_TEXT);

        invensenseLoggingOn = false;
    }
    else // Not currently saving to disk
    {
        if( ! startInvensenseLogging() )
        {
            balloonMessage("Failed to start Invensense Logger.");
            return;
        }

        invensenseLogButton->setText(STOP_LOG_INVS_TO_DISK_TEXT);

        invensenseLoggingOn = true;
    }
}
#endif

extern QString dongleDebugString;
#ifdef INVENSENSE_EV_ARM
extern QString genericDebugString;
#endif
extern QString gestureDebugString;
extern QString locationDebugString;
extern QString commsDebugString;
#ifdef INVENSENSE_EV_ARM
extern QString invsDebugString;
#endif
extern QString networkDebugString;

void DebugTab::updateDebug(void )
{
    if( ! debugText )              return;
    if( ! showingDebug )           return;
    if( ! debugTabVisible )        return;
    if( ! debugText->isVisible() ) return;

    debugText->clear();

#ifdef INVENSENSE_EV_ARM
    debugText->appendPlainText(genericDebugString);
#endif
    debugText->appendPlainText("Dongle");
    debugText->appendPlainText(dongleDebugString);

    debugText->appendPlainText("Gestures");
    debugText->appendPlainText(gestureDebugString);

    debugText->appendPlainText("3D Position");
    debugText->appendPlainText(locationDebugString);

    debugText->appendPlainText("App comms");
    debugText->appendPlainText(commsDebugString);

#ifdef INVENSENSE_EV_ARM
    debugText->appendPlainText("Invensense State");
    debugText->appendPlainText(invsDebugString);
#endif
    debugText->appendPlainText("Netdata State");
    debugText->appendPlainText(networkDebugString);
}



#define START_DEBUG_SAVE_TEXT   tr("Start saving debug")
#define STOP_DEBUG_SAVE_TEXT    tr("Stop saving debug ")


BoardDevTab::BoardDevTab(dongleReader *dongleRead, QWidget *parent) : QWidget(parent)
{
    rateDataIndex = 0;

    dongle = dongleRead;

    QHBoxLayout *topButtons = new QHBoxLayout;

    readDebugDataOverUSB    = false;

    QLabel *debugShownLabel = new QLabel("Debug shown:");
    debugConsoleShown = new QComboBox();
    debugConsoleShown->addItem("Debug to Serial", DEBUG_SHOWN_TO_SERIAL);
    debugConsoleShown->addItem("Debug to here",   DEBUG_SHOWN_TO_USB);
    debugConsoleShown->addItem("Display Flip",    DEBUG_SHOWN_FLIP);
    debugConsoleShown->addItem("Display Pen 1",   DEBUG_SHOWN_PEN_1);
    debugConsoleShown->addItem("Display Pen 2",   DEBUG_SHOWN_PEN_2);
    debugConsoleShown->addItem("Display Pen 3",   DEBUG_SHOWN_PEN_3);
    debugConsoleShown->addItem("Display Pen 4",   DEBUG_SHOWN_PEN_4);
    debugConsoleShown->addItem("Display Pen 5",   DEBUG_SHOWN_PEN_5);
    debugConsoleShown->addItem("Display Pen 6",   DEBUG_SHOWN_PEN_6);
    debugConsoleShown->addItem("Display Pen 7",   DEBUG_SHOWN_PEN_7);
    debugConsoleShown->addItem("Display Pen 8",   DEBUG_SHOWN_PEN_8);
    connect(debugConsoleShown, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDebugShownIndex(int)) );

    savingData = false;
    saveDebugToFileButton = new QPushButton(START_DEBUG_SAVE_TEXT);
    connect(saveDebugToFileButton, SIGNAL(clicked()), this, SLOT(startStopSaveToFile()) );

    saveFilenameText = new QLineEdit(QDir::homePath()+QString("/BoardDevDebug.txt"));

    selectFilenameButton = new QPushButton(tr("Select Save File"));
    connect(selectFilenameButton, SIGNAL(clicked()), this, SLOT(selectSaveFilename()) );

    topButtons->addWidget(debugShownLabel);
    topButtons->addWidget(debugConsoleShown);
    topButtons->addWidget(saveDebugToFileButton);
    topButtons->addWidget(saveFilenameText);
    topButtons->addWidget(selectFilenameButton);

    debugText = new QPlainTextEdit;
    debugText->appendPlainText(tr("Debug output from board\n"));
    debugText->setReadOnly(true);

    QVBoxLayout *debugLayout = new QVBoxLayout;
    debugLayout->addItem(topButtons);
    debugLayout->addWidget(debugText);

    setLayout(debugLayout);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateDebug()));
    timer->start(250);						// 0.25 second refresh
}

void BoardDevTab::changeDebugShownIndex(int index)
{
    int     *selection = (int *)debugConsoleShown->itemData( index ).data();
    int      pen;
    char     buffer[UPDATE_BLOCK_SZ];
    qint32  *first = (int *)buffer;

    *first = DEBUG_CONTROL;

    switch( *selection )
    {
    case DEBUG_SHOWN_TO_SERIAL:
        readDebugDataOverUSB = false;
        strcpy(buffer+sizeof(qint32),"Debug to serial.");
        break;

    case DEBUG_SHOWN_TO_USB:
        readDebugDataOverUSB = true;
        strcpy(buffer+sizeof(qint32),"Debug to USB.");
        break;

    case DEBUG_SHOWN_FLIP:
        strcpy(buffer+sizeof(qint32),"Show Flip.");
        break;

    case DEBUG_SHOWN_PEN_1:
    case DEBUG_SHOWN_PEN_2:
    case DEBUG_SHOWN_PEN_3:
    case DEBUG_SHOWN_PEN_4:
    case DEBUG_SHOWN_PEN_5:
    case DEBUG_SHOWN_PEN_6:
    case DEBUG_SHOWN_PEN_7:
    case DEBUG_SHOWN_PEN_8:

        pen = *selection - DEBUG_SHOWN_PEN_1;   // Pen number 0-7
        sprintf(buffer+sizeof(qint32),"Show pen %d.",pen);
        readDebugDataOverUSB = true;

        break;

    case DEBUG_SHOW_MEASURE_TIMING:
        strcpy(buffer+sizeof(qint32),"Show Timing.");
        break;

    default:
        return;
    }

    // And update data

    QFile   updateBlock;
    QString updateFileName = getCosnetadevicePath() + "COSNETA/DEV/UPDATERD.DAT";

    updateBlock.setFileName(updateFileName);

    if( ! updateBlock.open(QIODevice::Unbuffered|QIODevice::ReadWrite) )
    {
        balloonMessage("Failed to update debug on Flip - not found.");
        qDebug() << "Failed to update debug. Open failed.";

        return;
    }

    int sent = updateBlock.write(buffer,UPDATE_BLOCK_SZ);

    qDebug() << "Sent" << sent << "bytes to update block.";

    updateBlock.close();
#ifdef Q_OS_WIN
    // Flush to device
    QString fname = getCosnetadevicePath().replace(2,1,QString("\\"));
    HANDLE hDev = CreateFile(fname.toStdWString().data(),
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if( hDev == INVALID_HANDLE_VALUE )
    {
        qDebug() << QString("Failed to flush debug update: failed to open %1 error=%2")
                    .arg(fname).arg(GetLastError());

        return;
    }

    FlushFileBuffers(hDev);

    CloseHandle(hDev);
#endif
}


void BoardDevTab::startStopSaveToFile(void)
{
    if( savingData )
    {
        hwDebugSaveFile.close();

        saveDebugToFileButton->setText(START_DEBUG_SAVE_TEXT);
        savingData = false;
    }
    else
    {
        hwDebugSaveFile.setFileName(saveFilenameText->text());

        //if( hardwareDebugSaveFile.exists() )

        if( hwDebugSaveFile.open(QIODevice::WriteOnly | QIODevice::Text) )
        {
            saveDebugToFileButton->setText(STOP_DEBUG_SAVE_TEXT);
            hwDebugOutStream.setDevice(&hwDebugSaveFile);
            savingData = true;
        }

        if( ! hwDebugSaveFile.isOpen() )
        {
            balloonMessage(tr("Failed to open debug output file."));
        }
    }
}

void BoardDevTab::selectSaveFilename(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select Dump File"),
                                saveFilenameText->text(),
                                tr("Text files (*.txt)"));

    if( fileName.length()>0 )
    {
        saveFilenameText->setText(fileName);
    }
}



#define NUM_LINES 4096
#define NUM_CHARS 512
void BoardDevTab::updateDebug(void)
{
    static char    buf[NUM_CHARS] = "";
    static QString screen[NUM_LINES];
    static int     newLine = 0;

    bool  readOkay;

    if( ! boardTabVisible )        return;
    if( ! debugText->isVisible() ) return;
    if( ! readDebugDataOverUSB )   return;

    // Send wireless development ioctl to the board, and add the response to this box

    readOkay = dongle->readDebugRead((char *)buf, NUM_CHARS);
    if( strlen(buf) > 0 )
    {
        if( savingData )
        {
            hwDebugOutStream << buf;
        }

        // TODO: Add each new line as a separate entry
        screen[newLine] = QString(buf);

        // Got another line of data
        int     readLn = newLine+1;
        QString out = "";

        for(int showLn=0; showLn<NUM_LINES; showLn++)
        {
            if( readLn>=NUM_LINES ) readLn = 0;

            out += screen[readLn];
            //out += QString("\n"); - removed as it makes more sense to have it with the debug output line
            readLn ++;
        }
        debugText->clear();
        debugText->appendPlainText(out);

        if( readOkay )
        {
            newLine ++;
            if( newLine>=NUM_LINES ) newLine = 0;
        }
    }
}


#endif




