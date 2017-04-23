
#ifndef _APP_COMMS
#define _APP_COMMS

#include <QObject>
#include <QThread>
#include <QSharedMemory>
#include "main.h"
#include "dongleReader.h"

// Update at 100Hz for now
#define RATE_TIMESTEP_MS 10

class appComms : public QObject
{
    Q_OBJECT

public:
    appComms(dongleReader *dongleTask, networkReceiver *netRcv, sysTray_devices_state_t *table, QObject *parent = 0);
    ~appComms();

    void           doConnect(QThread &cThread);
    void           stopCommand(void);

    system_drive_t getSystemDriveState(void);
    void           setMouseDriveState(system_drive_t state);
    system_drive_t getPresentationDriveState(void);
    void           setPresentationDriveState(system_drive_t state);
    int            getLeadPenIndex(void);
    void           setLeadPenIndex(int index);
    void           enableGestures(void);
    void           disableGestures(void);
    bool           gesturesAreEnabled(void);
    void           setLeadPenPolicy(lead_select_policy_t newPolicy);


public slots:
    void                     sendThread();

private:
    void                     copyDataToSharedMemory(int &message_number, int &message_number_read);
    void                     applyActionsToLocalSystem(int last_gesture_seq_num[], int last_app_ctrl_seq_num[]);
    void                     updateActivePenNumbers(void);

    bool                     quitRequest;
    bool                     gesturesEnabled;

    sysTray_devices_state_t *sharedTable;
    QSharedMemory           *externalShare;
    cosneta_devices_t       *externalData;

    dongleReader            *dongleReadtask;
    networkReceiver         *networkTask;

    system_drive_t           systemDriveState;
    system_drive_t           presentationDriveState;
    uint32_t                 mouseControlPenIndex;
    uint32_t                 presentationControlPenIndex;
    lead_select_policy_t     leadPenPolicy;
};


#endif // _APP_COMMS
