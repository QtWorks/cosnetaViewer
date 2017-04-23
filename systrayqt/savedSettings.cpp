#include "savedSettings.h"

#include <QCoreApplication>

// Settings file keys
#define SETTINGS_AUTOSTART_FREESTYLE          "autostart/freeStyle"
#define SETTINGS_AUTOUPDATE                   "update/auto_update_enabled"
#define SETTINGS_LASTUPDATECHECK              "update/last_update_check_date"
#define SETTINGS_PRESENTATION                 "presentation/autostart_filename"
#define SETTINGS_PRESENTATION_SOFTWARE        "presentation/software_control_type"
#define SETTINGS_LEAD_PEN_ENABLE              "lead_pen/enable"
#define SETTINGS_LEAD_PEN_POLICY              "lead_pen/policy"
#define SETTINGS_LEAD_PEN_MANUAL_NUM          "lead_pen/manual_number"
#define SETTINGS_MOUSE_CONTROL_ENABLE         "mouse_control/enable"
#define SETTINGS_MOUSE_CONTROL_POLICY         "mouse_control/policy"
#define SETTINGS_PRESENTATION_CONTROL_ENABLE  "presentation_control/enable"
#define SETTINGS_PRESENTATION_CONTROL_POLICY  "presentation_control/policy"
#define SETTINGS_SESSION_NAME                 "session/name"

savedSettings::savedSettings(QObject *parent) : QObject(parent)
{
    QCoreApplication::setOrganizationName("Cosneta Ltd");
    QCoreApplication::setOrganizationDomain("cosneta.com");
    QCoreApplication::setApplicationName("sysTrayQt");

    sfile = new QSettings();

    readAllCurrentSettings();
}

savedSettings::~savedSettings()
{
    saveAllCurrentSettings();

    sfile->sync();

    delete sfile;
}

void savedSettings::writeSettingsOut(void)
{
    saveAllCurrentSettings();

    sfile->sync();
}


void savedSettings::readAllCurrentSettings(void)
{
    autostartFreestyle           = sfile->value(SETTINGS_AUTOSTART_FREESTYLE,true).toBool();
    autoUpdate                   = sfile->value(SETTINGS_AUTOUPDATE,false).toBool();
    dateOfLastUpdateCheck        = sfile->value(SETTINGS_LASTUPDATECHECK,
                                         QDate::currentDate().addDays(-5)).toDate();
    shortcutPresentationFilenameStr
                                 = sfile->value(SETTINGS_PRESENTATION).toString();
    presentationApp              = (application_type_t)(sfile->value(SETTINGS_PRESENTATION_SOFTWARE,APP_PDF).toInt());
    leadPenEnable                = sfile->value(SETTINGS_LEAD_PEN_ENABLE,true).toBool();
    leadSelectPolicy             = (lead_select_policy_t)(sfile->value(SETTINGS_LEAD_PEN_POLICY,USE_FIRST_CONNECTED_PEN).toInt());
    leadPenManualNumber          = sfile->value(SETTINGS_LEAD_PEN_MANUAL_NUM,0).toInt();
    mouseControlEnable           = sfile->value(SETTINGS_MOUSE_CONTROL_ENABLE,true).toBool();
    mouseControlPolicy           = (system_drive_t)(sfile->value(SETTINGS_LEAD_PEN_POLICY,DRIVE_SYSTEM_WITH_ANY_PEN).toInt());
    presentationControlEnable    = sfile->value(SETTINGS_PRESENTATION_CONTROL_ENABLE,true).toBool();
    presentationControlPolicy    = (system_drive_t)(sfile->value(SETTINGS_PRESENTATION_CONTROL_POLICY,DRIVE_SYSTEM_WITH_ANY_PEN).toInt());
    sessionNameStr               = sfile->value(SETTINGS_SESSION_NAME,"Session name not set.").toString();
}

void savedSettings::saveAllCurrentSettings(void)
{
    sfile->setValue(SETTINGS_AUTOSTART_FREESTYLE,autostartFreestyle);
    sfile->setValue(SETTINGS_AUTOUPDATE,autoUpdate);
    sfile->setValue(SETTINGS_LASTUPDATECHECK,dateOfLastUpdateCheck);
    sfile->setValue(SETTINGS_PRESENTATION,shortcutPresentationFilenameStr);
    sfile->setValue(SETTINGS_PRESENTATION_SOFTWARE,presentationApp);
    sfile->setValue(SETTINGS_LEAD_PEN_ENABLE,leadPenEnable);
    sfile->setValue(SETTINGS_LEAD_PEN_POLICY,leadSelectPolicy);
    sfile->setValue(SETTINGS_LEAD_PEN_MANUAL_NUM,leadPenManualNumber);
    sfile->setValue(SETTINGS_MOUSE_CONTROL_ENABLE,mouseControlEnable);
    sfile->setValue(SETTINGS_MOUSE_CONTROL_POLICY,mouseControlPolicy);
    sfile->setValue(SETTINGS_PRESENTATION_CONTROL_ENABLE,presentationControlEnable);
    sfile->setValue(SETTINGS_PRESENTATION_CONTROL_POLICY,presentationControlPolicy);
    sfile->setValue(SETTINGS_SESSION_NAME,sessionNameStr);
}


// Setters and getters

bool savedSettings::getAutostartFreestyleEnabled(void)
{
    return autostartFreestyle;
}

void savedSettings::setAutostartFreestyleEnabled(bool enable)
{
    autostartFreestyle = enable;
}

bool savedSettings::autoUpdateEnabled(void)
{
    return autoUpdate;
}

void savedSettings::setAutoUpdateEnabled(bool enable)
{
    autoUpdate = enable;
}

QDate savedSettings::lastUpdateDate(void)
{
    return dateOfLastUpdateCheck;
}

void savedSettings::setNowAsLastUpdateDate(void)
{
    dateOfLastUpdateCheck = QDate::currentDate();
}

QString savedSettings::shortcutPresentationFilename(void)
{
    return shortcutPresentationFilenameStr;
}

void savedSettings::setShortcutPresentationFilename(QString filepath)
{
    shortcutPresentationFilenameStr = filepath;
}

application_type_t savedSettings::shortcutPresentationType(void)
{
    return presentationApp;
}

void savedSettings::setShortcutPresentationType(application_type_t type)
{
    presentationApp = type;
}

bool savedSettings::getLeadPenEnabled(void)
{
    return leadPenEnable;
}

void savedSettings::setLeadPenEnabled(bool enable)
{
    leadPenEnable = enable;
}

lead_select_policy_t savedSettings::getLeadSelectPolicy(void)
{
    return leadSelectPolicy;
}

void savedSettings::setLeadSelectPolicy(lead_select_policy_t policy)
{
    leadSelectPolicy = policy;
}

bool savedSettings::getMouseControlEnabled(void)
{
    return mouseControlEnable;
}

void savedSettings::setMouseControlEnabled(bool enable)
{
    mouseControlEnable = enable;
}

system_drive_t savedSettings::getMouseControlPolicy(void)
{
    return mouseControlPolicy;
}

void savedSettings::setMouseControlPolicy(system_drive_t policy)
{
    mouseControlPolicy = policy;
}

bool savedSettings::getPresentationControlEnabled(void)
{
    return presentationControlEnable;
}

void savedSettings::setPresentationControlEnabled(bool enable)
{
    presentationControlEnable = enable;
}

system_drive_t savedSettings::getPresentationControlPolicy(void)
{
    return presentationControlPolicy;
}

void savedSettings::setPresentationControlPolicy(system_drive_t policy)
{
    presentationControlPolicy = policy;
}

QString savedSettings::getSessionName(void)
{
    return sessionNameStr;
}

void savedSettings::setSessionName(QString newName)
{
    sessionNameStr = newName;
}

QString savedSettings::getSessionPassword(void)
{
    return sessionPasswordStr;
}

void savedSettings::setSessionPassword(QString newPassword)
{
    sessionPasswordStr = newPassword;
}

