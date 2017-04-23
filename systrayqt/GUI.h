
#ifndef _COSNETA_GUI
#define _COSNETA_GUI

#include "build_options.h"

#include <QtGui>
#include <QAbstractButton>
#include <QSystemTrayIcon>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QFile>
#include <QGraphicsView>
#include <QDesktopWidget>
#include <QSettings>
#include <QDate>
#include <QNetworkAccessManager>

#include "dongleReader.h"
#include "networkReceiver.h"
#include "FreerunnerReader.h"

#include "updater.h"

// #define DETECTS_FREESTYLE_CORRECTLY

class QAction;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QTabWidget;
class QPlainTextEdit;


class SettingsTab : public QWidget
{
    Q_OBJECT

public slots:
    void driveMouseStateChanged();
    void drivePresentationStateChanged();
    void useALeadPenChanged();
    void leadPolicyChanged(int index);
    void useGesturesStateChanged();
    void autoStartChanged();
    void pointerSensitivityChanged(int value);
    void colourSelect(int);
    void populatePointerDriveList(void);
    void populatePresenterDriveList(void);
    void mouseControllerChanged(int index);
    void presentationControllerChanged(int index);
    void selectDocumentFilename(void);
    void applySettingsChanges(void);
    void maxUsersChanged(int newMaxUsers);

public:
    SettingsTab(class dongleReader *dongle, networkReceiver *network, sysTray_devices_state_t *tableIn,
                QWidget *parent = 0);

    void        updateMaxUsers(int newMaxUsers);
    void        makeDummyNotPresentPenEntry(int entryNum);
    void        setFlipPresent(bool present);
    void        retrieveCurrentSettings(void);
    void        showInitialScreen(void);
    void        showRestoreScreen(void);

    class dongleReader      *dongleReadTask;
    networkReceiver         *networkInput;
    sysTray_devices_state_t *sharedTable;

    QCheckBox     *useLeadPen;
    QComboBox     *leadSelectPolicy;

    QLineEdit     *sessionName;
    QLineEdit     *sessionPassword;

    QComboBox     *maxUsersCombo;

    // List of pens attached to dongle
    QButtonGroup  *leadPenGroup;
    QCheckBox     *isLeadPen[MAX_PENS];
    QColor         currentColour[MAX_PENS];

    QFrame        *summaryView[MAX_PENS];   // May throw this reference away... tbd
    QLabel        *colourSwatch[MAX_PENS];
    QLabel        *penName[MAX_PENS];
    QLabel        *penState[MAX_PENS];

    // Generic settings
    bool          presenterModeEnabled;
    QComboBox    *presenterApplication;
    bool          editingGesturesEnabled;
    bool          editingPointerEnabled;
    int           editingPointerSensitivityPercent;
#ifdef INVENSENSE_EV_ARM
    bool        invensenseEnabled;
    int         currentInvensensePort;
#endif
    QString      listenerIP;
    int          listenerPort;
    QLabel      *networkReceiverLabel;
    int          mouseDriveIndex;
    int          presentationDriveIndex;
    bool         initialView;

    // Public to allow initialisation
    QGroupBox   *mouseControlsBox;
    QGroupBox   *presenterControlsBox;
    QGroupBox   *gestureControlsBox;
    QLineEdit   *shortcutDocumentFilenameText;
    QPushButton *shortcutDocumentFilenameButton;
#ifdef INVENSENSE_EV_ARM
    QGroupBox   *invensenseControlsBox;
    QSpinBox    *invensensePort;
#endif
    QGroupBox   *networkConnectionBox;
    QCheckBox   *autoStartCheck;
    QComboBox   *mouseDrivePenID;
    QComboBox   *presentorDrivePenID;
    QGroupBox   *autostartBox;
    QCheckBox   *autoStartFreeStyle;

signals:
    void         settingsUpdatedMaxUsers(int newMaxUsers);

private:
    QSignalMapper  *colourButtonMap;
    QLabel         *flipPresentTag;
};




class GesturesTab : public QWidget
{
    Q_OBJECT

public:
    GesturesTab(QWidget *parent = 0);
};


// These defines represent the hardware button sequence.

#define BUTTON_LEFT_MOUSE_IDX      0
#define BUTTON_MIDDLE_MOUSE_IDX    4
#define BUTTON_RIGHT_MOUSE_IDX     1
#define BUTTON_SCROLL_DOWN_IDX     3
#define BUTTON_SCROLL_UP_IDX       2
#define BUTTON_MODE_SWITCH_IDX     5
#define BUTTON_OPT_A_IDX           6
#define BUTTON_OPT_B_IDX           7
#define BUTTON_SURFACE_SETECT_IDX 15

class editPenDataTab : public QWidget
{
    Q_OBJECT

public:
    editPenDataTab(sysTray_devices_state_t *table, dongleReader *dongleTask, QWidget *parent = 0);

    void       rescaleEditTab(void);
    void       populateEditWithData(void);
    void       updateForDisplay(void);
    void       applyAnyChangedSettings(void);

public slots:

    void       currentPenNumChanged(int index);
    void       colourSelect(void);
    void       optionsIndexChanged(int userBoxChanged, int newIndex);
    void       buttonMouse1OptionsIndexChanged(int index);
    void       buttonMouse2OptionsIndexChanged(int index);
    void       buttonMouse3OptionsIndexChanged(int index);
    void       buttonUpOptionsIndexChanged(int index);
    void       buttonDownOptionsIndexChanged(int index);
    void       buttonModeOptionsIndexChanged(int index);
    void       buttonOptAOptionsIndexChanged(int index);
    void       buttonOptBOptionsIndexChanged(int index);

private:

    QComboBox *createButtonOptionsCombo(QWidget *parent);

    dongleReader            *dongleReadtask;
    sysTray_devices_state_t *sharedTable;

    QComboBox        *penList;
    QLineEdit        *penName;
    QSlider          *sensitivitySlider;
    QPushButton      *colourSelector;
    QColor            penColour;
    quint32           optionsFlags;

    bool              ignoreComboChanges;
    int               currentPenIndex;

    // Buttons combo boxes and containers
    QWidget          *buttonContainer;

    QComboBox        *buttonMouse1Options;
    QComboBox        *buttonMouse2Options;
    QComboBox        *buttonMouse3Options;
    QComboBox        *buttonUpOptions;
    QComboBox        *buttonDownOptions;
    QComboBox        *buttonModeOptions;
    QComboBox        *buttonOptAOptions;
    QComboBox        *buttonOptBOptions;
//    QComboBox        *buttonSurfaceDetectOptions;   // Maybe?

    QComboBox        *buttons[16];

    // Allow for resizing of drawing
    QLabel           *penImageLabel;
    QPixmap          *penImageFull;
};



#ifdef DEBUG_TAB

class DebugTab : public QWidget
{
    Q_OBJECT
    
public:
	DebugTab(QWidget *parent = 0);

    bool              debugTabVisible;

public slots:
    void              updateDebug(void);
    void              saveDataButtonClicked();
    void              showDebugButtonClicked();
    void              saveGesturesButtonClicked();
    void              generateDataButtonClicked();
    void              useMouseDataButtonClicked();
#ifdef INVENSENSE_EV_ARM
    void              invensenseLogButtonClicked();
#endif

private:
    QPlainTextEdit   *debugText;
    QPushButton      *saveFileButton;
    QPushButton      *showDebugButton;
    QPushButton      *saveGesturesButton;
    QPushButton      *generateDataButton;
    QPushButton      *useMouseDataButton;
#ifdef INVENSENSE_EV_ARM
    QPushButton      *invensenseLogButton;
#endif
    QTimer		     *timer;
    bool              diskSaveActive;
    bool              showingDebug;
    bool              gestureSaveActive;
#ifdef INVENSENSE_EV_ARM
    bool              invensenseLoggingOn;
#endif
};

// Where the debug should go...
#define DEBUG_SHOWN_TO_SERIAL      0
#define DEBUG_SHOWN_TO_USB        10

// What the debug should be...
#define DEBUG_SHOWN_PEN_1          1
#define DEBUG_SHOWN_PEN_2          2
#define DEBUG_SHOWN_PEN_3          3
#define DEBUG_SHOWN_PEN_4          4
#define DEBUG_SHOWN_PEN_5          5
#define DEBUG_SHOWN_PEN_6          6
#define DEBUG_SHOWN_PEN_7          7
#define DEBUG_SHOWN_PEN_8          8
#define DEBUG_SHOWN_FLIP           9
#define DEBUG_SHOW_MEASURE_TIMING 11

class BoardDevTab : public QWidget
{
    Q_OBJECT

public:
    BoardDevTab(dongleReader *dongleRead, QWidget *parent = 0);

    bool              boardTabVisible;
    bool              savingData;

public slots:
    void     updateDebug(void);
    void     startStopSaveToFile(void);
    void     selectSaveFilename(void);
    void     changeDebugShownIndex(int index);

private:
    QPlainTextEdit   *debugText;
    bool              readDebugDataOverUSB;

    QLineEdit        *saveFilenameText;
    QPushButton      *saveDebugToFileButton;
    QPushButton      *selectFilenameButton;
    QTimer		     *timer;
    QComboBox        *debugConsoleShown;

    dongleReader     *dongle;

    int               rateDataIndex;
    QFile             hwDebugSaveFile;
    QTextStream       hwDebugOutStream;
};

#endif



class CosnetaGui : public QWidget
{
    Q_OBJECT

public slots:
    void            startSession(void);
    void            applyChanges();
    void            rejectChanges();
    void            showRestoreWindow(void);
    void            updateCheck(bool checked);
    void            launchFreestyle();
    void            haltFreestyle();
    void            help();
    void            quitApp(bool checked);
    void            tabChanged();
    void            penChanged(int penNum);
    void            downloadDataAvailable(void);
    void            downloadFinished(QNetworkReply *reply);
    void            updateCheckedNumCnx(int optionToSet);

    void            setMaxUsersTo0();
    void            setMaxUsersTo1();
    void            setMaxUsersTo2();
    void            setMaxUsersTo3();
    void            setMaxUsersTo4();
    void            setMaxUsersTo5();
    void            setMaxUsersTo6();
    void            setMaxUsersTo7();
    void            setMaxUsersTo8();


public:
    CosnetaGui(dongleReader *dongle, networkReceiver *network, sysTray_devices_state_t *table);

    void             closeEvent(QCloseEvent *e);
    void		     showBalloonMessage(QString messageText, int delay);
    void             setFlipPresent(bool present);
    void             showInitialScreen(void);

private:
    void		     createPropertiesWindow(void);
    void             rebuildTrayMenu(void);
    void             retreiveCurrentSettings(void);
    void             storeCurrentSettings(void);
    void             startUpdateCheck(void);

    QPushButton     *startSessionBtn;
    QPushButton     *updateSessionBtn;
    QPushButton     *rejectUpdatesBtn;

    QTabWidget	    *propertiesTabs;
    SettingsTab     *settings;
    GesturesTab     *gestures;
    editPenDataTab  *penSettingsEditor;
#ifdef DEBUG_TAB
    DebugTab        *debug;
    BoardDevTab     *board;
#endif

    QNetworkAccessManager *downloadMetaDataMgr;
    QNetworkReply         *metaDataRequestReply;
    bool                   metaDownloadInProgress;
    QByteArray             downloadedUpdateMetaData;
    updater               *updateWindow;

    dongleReader    *dongleReadtask;
    networkReceiver *netReadTask;

    bool             trayIconPresent;

    // Tray menu things
    QSystemTrayIcon	*trayIcon;
    QMenu           *trayIconMenu;
    QMenu           *maxUsersMenu;

    // Adjust maximum number of connections options
    QAction         *maxUsers0;
    QAction         *maxUsers1;
    QAction         *maxUsers2;
    QAction         *maxUsers3;
    QAction         *maxUsers4;
    QAction         *maxUsers5;
    QAction         *maxUsers6;
    QAction         *maxUsers7;
    QAction         *maxUsers8;

    int              currentMaxUsers;

    QAction 	    *restoreAction;
    QAction 	    *hideAction;
    QAction 	    *updateAction;
    QAction 	    *freeStyleStartAction;
#ifdef DETECTS_FREESTYLE_CORRECTLY
    QAction 	    *freeStyleStopAction;
#endif
    QAction 	    *helpAction;
    QAction 	    *quitAction;

    sysTray_devices_state_t *sharedTable;
    QDesktopWidget          *desktop;

};


#endif


