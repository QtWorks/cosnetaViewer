#ifndef SAVEDSETTINGS_H
#define SAVEDSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QDate>

#include "main.h"

class savedSettings : public QObject
{
    Q_OBJECT
public:
    explicit savedSettings(QObject *parent = 0);
    ~savedSettings();

    void                 writeSettingsOut(void);

    // Setters and getters for each setting
    bool                 getAutostartFreestyleEnabled(void);
    void                 setAutostartFreestyleEnabled(bool enable);
    bool                 autoUpdateEnabled(void);
    void                 setAutoUpdateEnabled(bool enable);
    QDate                lastUpdateDate(void);
    void                 setNowAsLastUpdateDate(void);
    QString              shortcutPresentationFilename(void);
    void                 setShortcutPresentationFilename(QString filepath);
    application_type_t   shortcutPresentationType(void);
    void                 setShortcutPresentationType(application_type_t type);
    bool                 getLeadPenEnabled(void);
    void                 setLeadPenEnabled(bool enable);
    lead_select_policy_t getLeadSelectPolicy(void);
    void                 setLeadSelectPolicy(lead_select_policy_t policy);
    bool                 getMouseControlEnabled(void);
    void                 setMouseControlEnabled(bool enable);
    system_drive_t       getMouseControlPolicy(void);
    void                 setMouseControlPolicy(system_drive_t policy);
    bool                 getPresentationControlEnabled(void);
    void                 setPresentationControlEnabled(bool enable);
    system_drive_t       getPresentationControlPolicy(void);
    void                 setPresentationControlPolicy(system_drive_t policy);
    QString              getSessionName(void);
    void                 setSessionName(QString newName);
    QString              getSessionPassword(void);
    void                 setSessionPassword(QString newPassword);

signals:
    
public slots:
    
private:
    void       readAllCurrentSettings(void);
    void       saveAllCurrentSettings(void);

    QSettings *sfile;

    bool                 autostartFreestyle;
    bool                 autoUpdate;
    QDate                dateOfLastUpdateCheck;
    QString              shortcutPresentationFilenameStr;
    application_type_t   presentationApp;
    bool                 leadPenEnable;
    lead_select_policy_t leadSelectPolicy;
    int                  leadPenManualNumber;
    bool                 mouseControlEnable;
    system_drive_t       mouseControlPolicy;
    bool                 presentationControlEnable;
    system_drive_t       presentationControlPolicy;
    QString              sessionNameStr;
    QString              sessionPasswordStr;
};

#endif // SAVEDSETTINGS_H
