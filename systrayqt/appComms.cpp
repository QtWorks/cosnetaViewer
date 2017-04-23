
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTime>
#include <QDesktopServices>

#include <QDebug>

#include <appComms.h>
#include "main.h"
#include "../cosnetaapi/sharedMem.h"
#include "freerunner_low_level.h"

// #define NO_SHARED_MEMORY_TEST


QString commsDebugString = "Unset";

#include <savedSettings.h>
extern savedSettings     *persistentSettings;


appComms::appComms(dongleReader *dongleTask, networkReceiver *netRcv, sysTray_devices_state_t *table, QObject *parent) : QObject(parent)
{
    // Save a pointer to the internal data
    sharedTable = table;
#ifndef NO_SHARED_MEMORY_TEST
    // And create the shared memory area.
    externalShare = new QSharedMemory( SHARED_MEMORY_KEY );

    externalData = NULL;

    if( externalShare->attach() )
    {
        externalData = (cosneta_devices_t *)externalShare->data();

        QMessageBox::critical(NULL, "Cosneta Software Exiting...","Cosneta software already running. New instance not started.");

        throw("Abort.");
    }

    if( ! externalShare->create(sizeof(cosneta_devices_t)) )
    {
        delete externalShare;
        externalShare = NULL;

        QMessageBox::critical(NULL, "Cosneta Software Exiting...","Failed to create shared memory area.");

        throw("Abort.");
    }

    externalData = (cosneta_devices_t *)externalShare->data();

    if( externalData == NULL )
    {
        delete externalShare;
        externalShare = NULL;

        QMessageBox::critical(NULL, "Cosneta Software Exiting...","Failed to find shared memory area.");

        throw("Abort.");
    }

    qDebug() << "SHMem native key:" << externalShare->nativeKey();
#else
    externalData = (cosneta_devices_t *)calloc(1,sizeof(cosneta_devices_t));
#endif

    memcpy(externalData->pen,sharedTable->pen,MAX_PENS*sizeof(pen_state_t));

    // And miscellanea
    systemDriveState            = DRIVE_SYSTEM_WITH_ANY_PEN;
    presentationDriveState      = DRIVE_SYSTEM_WITH_ANY_PEN;
    mouseControlPenIndex        = 0;
    presentationControlPenIndex = 0;
    gesturesEnabled             = false;
    dongleReadtask              = dongleTask;
    networkTask                 = netRcv;
    leadPenPolicy               = USE_FIRST_CONNECTED_PEN;
}


appComms::~appComms()
{
#ifndef NO_SHARED_MEMORY_TEST
    if( externalShare != NULL )
    {
        delete externalShare;
        externalShare = NULL;
    }
#endif
}

void appComms::updateActivePenNumbers(void)
{
    int p;

    // Copy current state TODO: remove penMode[] and just use flags.
//    QString dbgStr = "Pen states: ";
    for(p=0; p<MAX_PENS; p++)
    {
        if( sharedTable->pen[p].status_flags != COS_PEN_INACTIVE )
        {
            switch( sharedTable->penMode[p] )
            {
            case NORMAL_OVERLAY:

//                dbgStr += "NORM ";

                COS_SET_PEN_MODE(sharedTable->pen[p],COS_MODE_NORMAL);
                break;

            case DRIVE_MOUSE:

//                dbgStr += "MOUS ";

                COS_SET_PEN_MODE(sharedTable->pen[p],COS_MODE_DRIVE_MOUSE);
                break;

            case PRESENTATION_CONTROLLER:

//                dbgStr += "PRES ";

                COS_SET_PEN_MODE(sharedTable->pen[p],COS_MODE_DRIVE_PRESENTATION);
                break;
            }
        }
//        else  dbgStr += "---- ";
    }
//    qDebug() << dbgStr;

    // Maintain lead pen: logic is, the oldest pen on the system
    switch( leadPenPolicy )
    {
    case NO_LEAD_PEN:
    case MANUAL_PEN_SELECT:
        // Do nothing here.
        break;

    case USE_FIRST_CONNECTED_PEN:
    case USE_FIRST_CONNECTED_LOCAL_PEN:

        if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[sharedTable->leadPen])  )
        {
            int newLeadPenNum = sharedTable->leadPen;

            // skip on to the next active pen
            for(p=0; p<MAX_PENS; p++)
            {
                newLeadPenNum ++;
                if( newLeadPenNum >= MAX_PENS ) newLeadPenNum = 0;

                if( COS_PEN_IS_ACTIVE(sharedTable->pen[sharedTable->leadPen]) )
                {
                    if(  leadPenPolicy == USE_FIRST_CONNECTED_PEN           ||
                        (leadPenPolicy == USE_FIRST_CONNECTED_LOCAL_PEN &&
                         ! COS_PEN_IS_FROM_NETWORK(sharedTable->pen[newLeadPenNum]) )    )
                    {
                        sharedTable->leadPen = newLeadPenNum;

                        balloonMessage(QString("New lead pen: %1").arg(newLeadPenNum));
                        qDebug() << "New lead pen:" << newLeadPenNum;
                        break;
                    }
                }
            }
        }
        break;
    }

    // Maintain mouse drive pen
    switch( systemDriveState )
    {
    case NO_SYSTEM_DRIVE:

        mouseControlPenIndex = -1;
        break;

    case DRIVE_SYSTEM_WITH_LEAD_PEN:

        if( sharedTable->leadPen  >=  0                                       &&
            COS_PEN_IS_ACTIVE(sharedTable->pen[sharedTable->leadPen])         &&
            COS_GET_PEN_MODE(sharedTable->pen[sharedTable->leadPen]) == COS_MODE_DRIVE_MOUSE )
        {
            mouseControlPenIndex = sharedTable->leadPen;
        }
        else
        {
            mouseControlPenIndex = -1;
        }
        break;

    case DRIVE_SYSTEM_WITH_ANY_PEN:

        if( mouseControlPenIndex == -1                                           ||
            ! COS_PEN_IS_ACTIVE(sharedTable->pen[mouseControlPenIndex])          ||
            COS_GET_PEN_MODE(sharedTable->pen[mouseControlPenIndex]) != COS_MODE_DRIVE_MOUSE )
        {
            int chk = mouseControlPenIndex;

//            if( mouseControlPenIndex == -1 )
//                qDebug() << "Mouse controller pen status changed. Pen num was -1";
//            else
//                qDebug() << "Mouse controller pen status changed. Pen was" << mouseControlPenIndex
//                         << "active ="
//                         << COS_PEN_IS_ACTIVE(sharedTable->pen[mouseControlPenIndex])
//                         << "Pen mode ="
//                         << COS_PEN_MODE_STR(COS_GET_PEN_MODE(sharedTable->pen[mouseControlPenIndex]));

            mouseControlPenIndex = -1;

            // skip on to the next active pen
            for(int p=0; p<MAX_PENS; p++)
            {
                chk ++;
                if( chk >= MAX_PENS ) chk = 0;

                if( COS_PEN_IS_ACTIVE(sharedTable->pen[chk])         &&
                    COS_GET_PEN_MODE(sharedTable->pen[chk]) == COS_MODE_DRIVE_MOUSE    )
                {
                    mouseControlPenIndex = chk;

                    balloonMessage(QString("New mouse pen: %1").arg(sharedTable->settings[p].users_name));
                    qDebug() << "New mouse drive pen:" << chk;
                    break;
                }
            }
        }
        break;

    case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:

        if( mouseControlPenIndex == -1                                           ||
            ! COS_PEN_IS_ACTIVE(sharedTable->pen[mouseControlPenIndex])          ||
            COS_GET_PEN_MODE(sharedTable->pen[mouseControlPenIndex]) != COS_MODE_DRIVE_MOUSE )
        {
            int chk = mouseControlPenIndex;

            mouseControlPenIndex = -1;

            // skip on to the next active pen
            for(int p=0; p<MAX_PENS; p++)
            {
                chk ++;
                if( chk >= MAX_PENS ) chk = 0;

                if( COS_PEN_IS_ACTIVE(sharedTable->pen[chk])        &&
                   ! COS_PEN_IS_FROM_NETWORK(sharedTable->pen[chk]) &&
                     COS_GET_PEN_MODE(sharedTable->pen[chk]) == COS_MODE_DRIVE_MOUSE   )
                {
                    mouseControlPenIndex = chk;

                    balloonMessage(QString("New mouse pen: %1").arg(sharedTable->settings[p].users_name));
                    qDebug() << "New mouse drive pen:" << chk;
                    break;
                }
            }
        }
        break;
    }

    // Maintain presentation drive pen
    switch( presentationDriveState )
    {
    case NO_SYSTEM_DRIVE:

        presentationControlPenIndex = -1;
        break;

    case DRIVE_SYSTEM_WITH_LEAD_PEN:

        if( sharedTable->leadPen  >=  0                                        &&
            COS_PEN_IS_ACTIVE(sharedTable->pen[sharedTable->leadPen])          &&
            COS_GET_PEN_MODE(sharedTable->pen[sharedTable->leadPen]) == COS_MODE_DRIVE_PRESENTATION  )
        {
            presentationControlPenIndex = sharedTable->leadPen;
        }
        else
        {
            presentationControlPenIndex = -1;
        }
        break;

    case DRIVE_SYSTEM_WITH_ANY_PEN:

        if( presentationControlPenIndex != sharedTable->leadPen                 &&
            leadPenPolicy               != NO_LEAD_PEN                          &&
            COS_GET_PEN_MODE(sharedTable->pen[sharedTable->leadPen]) == COS_MODE_DRIVE_PRESENTATION )
        {
            // Only change if have a lead pen and it demands mouse control
            presentationControlPenIndex = sharedTable->leadPen;
        }
        else if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[presentationControlPenIndex])          ||
                 COS_GET_PEN_MODE(sharedTable->pen[presentationControlPenIndex]) != COS_MODE_DRIVE_PRESENTATION )
        {
            int chk = presentationControlPenIndex;

            presentationControlPenIndex = 0;

            // skip on to the next active pen
            for(int p=0; p<MAX_PENS; p++)
            {
                chk ++;
                if( chk >= MAX_PENS ) chk = 0;

                if( COS_PEN_IS_ACTIVE(sharedTable->pen[chk])           &&
                    COS_GET_PEN_MODE(sharedTable->pen[chk]) == COS_MODE_DRIVE_PRESENTATION   )
                {
                    presentationControlPenIndex = chk;

                    balloonMessage(QString("New presentation pen: %1").arg(sharedTable->settings[p].users_name));
                    qDebug() << "New presentation drive pen:" << chk;
                    break;
                }
            }
        }
        break;

    case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:

        if( presentationControlPenIndex != sharedTable->leadPen                 &&
            leadPenPolicy               != NO_LEAD_PEN                          &&
            COS_GET_PEN_MODE(sharedTable->pen[sharedTable->leadPen]) == COS_MODE_DRIVE_PRESENTATION )
        {
            // Only change if have a lead pen and it demands mouse control
            presentationControlPenIndex = sharedTable->leadPen;
        }
        else if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[presentationControlPenIndex])          ||
                 COS_GET_PEN_MODE(sharedTable->pen[presentationControlPenIndex]) != COS_MODE_DRIVE_PRESENTATION )
        {
            int chk = presentationControlPenIndex;

            presentationControlPenIndex = -1;

            // skip on to the next active pen
            for(int p=0; p<MAX_PENS; p++)
            {
                chk ++;
                if( chk >= MAX_PENS ) chk = 0;

                if( COS_PEN_IS_ACTIVE(sharedTable->pen[chk])        &&
                   ! COS_PEN_IS_FROM_NETWORK(sharedTable->pen[chk]) &&
                   COS_GET_PEN_MODE(sharedTable->pen[chk]) == COS_MODE_DRIVE_PRESENTATION     )
                {
                    presentationControlPenIndex = chk;

                    balloonMessage(QString("New presentation pen: %1").arg(presentationControlPenIndex));
                    qDebug() << "New presentation drive pen:" << presentationControlPenIndex;
                    break;
                }
            }
        }
        break;
    }
}


void appComms::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(sendThread()) );
}


void appComms::stopCommand(void)
{
    quitRequest = true;     // Don't detach as it's ours, and we may be restarted.
}


void appComms::applyActionsToLocalSystem(int last_gesture_seq_num[],
                                         int last_app_ctrl_seq_num[])
{
    if( systemDriveState     != NO_SYSTEM_DRIVE                               &&
        mouseControlPenIndex >= 0 && mouseControlPenIndex < MAX_PENS          &&
        COS_PEN_IS_ACTIVE(sharedTable->pen[mouseControlPenIndex])             &&
        COS_GET_PEN_MODE(sharedTable->pen[mouseControlPenIndex]) == COS_MODE_DRIVE_MOUSE )
    {
        QPoint p = QPoint(sharedTable->pen[mouseControlPenIndex].screenX,
                   sharedTable->pen[mouseControlPenIndex].screenY);

        // Send location to the mouse (fails on Linux)
        QCursor::setPos(p.x(), p.y());

        // And buttons
        setMouseButtons(&p,sharedTable->pen[mouseControlPenIndex].buttons);

        // For now, just send gesture keypresses in mouse mode
        if( gesturesEnabled &&
            last_gesture_seq_num[mouseControlPenIndex] != sharedTable->gesture_sequence_num[mouseControlPenIndex] )
        {
            // Lead pen wsnts to send gesture keypresses
            if( sharedTable->gesture_key[mouseControlPenIndex] != KEY_NUM_NO_KEY )
            {
                // Have a gesture, so send it to the operating system
                sendKeypressToOS(sharedTable->gesture_key[mouseControlPenIndex],
                                 sharedTable->gesture_key_mods[mouseControlPenIndex] & KEY_MOD_SHIFT,
                                 sharedTable->gesture_key_mods[mouseControlPenIndex] & KEY_MOD_CTRL, false  );
            }

            last_gesture_seq_num[mouseControlPenIndex] = sharedTable->gesture_sequence_num[mouseControlPenIndex];
        }

        // Check for raw keypresses generated somewhere
        if( APP_CTRL_IS_RAW_KEYPRESS(sharedTable->pen[mouseControlPenIndex].app_ctrl_action)   &&
            last_app_ctrl_seq_num[mouseControlPenIndex] != sharedTable->pen[mouseControlPenIndex].app_ctrl_sequence_num )
        {
            bool      shiftPressed   = Qt::ShiftModifier   == (Qt::ShiftModifier   & sharedTable->pen[mouseControlPenIndex].app_ctrl_action);
            bool      controlPressed = Qt::ControlModifier == (Qt::ControlModifier & sharedTable->pen[mouseControlPenIndex].app_ctrl_action);
            int       keyNum = APP_CTRL_KEY_FROM_RAW_KEYPRESS(sharedTable->pen[mouseControlPenIndex].app_ctrl_action);

            sendKeypressToOS(keyNum, shiftPressed, controlPressed, false );

            last_app_ctrl_seq_num[mouseControlPenIndex] = sharedTable->pen[mouseControlPenIndex].app_ctrl_sequence_num;

            qDebug() << "Keypress:" << APP_CTRL_KEY_FROM_RAW_KEYPRESS(sharedTable->pen[mouseControlPenIndex].app_ctrl_action)
                     << "from (" << QString("0x%1").arg(sharedTable->pen[mouseControlPenIndex].app_ctrl_action,0,16) << ")"
                     << "Index"  << sharedTable->pen[mouseControlPenIndex].app_ctrl_sequence_num
                     << "Cell"   << (char)(APP_CTRL_KEY_FROM_RAW_KEYPRESS(sharedTable->pen[mouseControlPenIndex].app_ctrl_action)&255);
        }
    }

    if( presentationDriveState      != NO_SYSTEM_DRIVE                               &&
        presentationControlPenIndex >= 0 && presentationControlPenIndex < MAX_PENS   &&
        COS_PEN_IS_ACTIVE(sharedTable->pen[presentationControlPenIndex])             &&
        COS_GET_PEN_MODE(sharedTable->pen[presentationControlPenIndex]) == COS_MODE_DRIVE_PRESENTATION     )
    {
        // presentationControlPenIndex
        // Lead pen wsnts to send application controls
        if( sharedTable->pen[presentationControlPenIndex].app_ctrl_action       != APP_CTRL_NO_ACTION        &&
            sharedTable->pen[presentationControlPenIndex].app_ctrl_sequence_num != last_app_ctrl_seq_num[presentationControlPenIndex]  )
        {
            if( sharedTable->pen[presentationControlPenIndex].app_ctrl_action == APP_CTRL_START_SHORTCUT )
            {
                if( QFile::exists(persistentSettings->shortcutPresentationFilename()) )
                {
#ifdef Q_OS_WIN
                    // Key combo to minimise all other applications. Then the presentation might be in front.
                    sendWindowsKeyCombo('M', false, false, false);
#endif
                    balloonMessage(QString("Launch %1").arg(persistentSettings->shortcutPresentationFilename()));
                    QDesktopServices::openUrl(QUrl(QString("file:%2").arg(persistentSettings->shortcutPresentationFilename()),QUrl::TolerantMode));
                }
                else
                {
                    balloonMessage(QString("Presentation %1 not found.").arg(persistentSettings->shortcutPresentationFilename()));;
                }
            }
            else if( ! APP_CTRL_IS_PEN_MODE(sharedTable->pen[presentationControlPenIndex].app_ctrl_action) )
            {
                // Have a presntation control action, so send it to the operating system
                sendApplicationCommandOS(persistentSettings->shortcutPresentationType(),
                                         sharedTable->pen[presentationControlPenIndex].app_ctrl_action );
            }

            last_app_ctrl_seq_num[presentationControlPenIndex] = sharedTable->pen[presentationControlPenIndex].app_ctrl_sequence_num;
        }
    }
#if 0
    QPoint p;

    switch( sharedTable->penMode[penIndex] )
    {
    case DRIVE_MOUSE:

        // Limit action on pen mode
        switch( systemDriveState )
        {
        case NO_SYSTEM_DRIVE:
            return;

        case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:
            if( COS_PEN_IS_FROM_NETWORK(sharedTable->pen[penIndex]) ) return;
            break;

        case DRIVE_SYSTEM_WITH_LEAD_PEN:
            if( penIndex != sharedTable->leadPen ) return;
            break;

        case DRIVE_SYSTEM_WITH_ANY_PEN:
            break;
        }

        p = QPoint(sharedTable->pen[penIndex].screenX,
                   sharedTable->pen[penIndex].screenY);

        // Lead pen wsnts to drive mouse

        // Send location to the mouse (fails on Linux)
        QCursor::setPos(p.x(), p.y());

        // And buttons
        setMouseButtons(&p,sharedTable->pen[penIndex].buttons);

        // And set the flag
        COS_SET_PEN_MODE(sharedTable->pen[penIndex],COS_MODE_DRIVE_MOUSE);

        // For now, just send gesture keypresses in mouse mode
        if( gesturesEnabled &&
            last_gesture_seq_num[penIndex] != sharedTable->gesture_sequence_num[penIndex] )
        {
            // Lead pen wsnts to send gesture keypresses
            if( sharedTable->gesture_key[penIndex] != KEY_NUM_NO_KEY )
            {
                // Have a gesture, so send it to the operating system
                sendKeypressToOS(sharedTable->gesture_key[penIndex],
                                 sharedTable->gesture_key_mods[penIndex] & KEY_MOD_SHIFT,
                                 sharedTable->gesture_key_mods[penIndex] & KEY_MOD_CTRL, false  );
            }

            last_gesture_seq_num[penIndex] = sharedTable->gesture_sequence_num[penIndex];
        }

        // Check for raw keypresses generated somewhere
        if( APP_CTRL_IS_RAW_KEYPRESS(sharedTable->pen[penIndex].app_ctrl_action)   &&
            last_app_ctrl_seq_num[penIndex] != sharedTable->pen[penIndex].app_ctrl_sequence_num )
        {
            bool      shiftPressed   = Qt::ShiftModifier   == (Qt::ShiftModifier   & sharedTable->pen[penIndex].app_ctrl_action);
            bool      controlPressed = Qt::ControlModifier == (Qt::ControlModifier & sharedTable->pen[penIndex].app_ctrl_action);
            int       keyNum = APP_CTRL_KEY_FROM_RAW_KEYPRESS(sharedTable->pen[penIndex].app_ctrl_action);

            sendKeypressToOS(keyNum, shiftPressed, controlPressed, false );

            last_app_ctrl_seq_num[penIndex] = sharedTable->pen[penIndex].app_ctrl_sequence_num;

            qDebug() << "Keypress:" << APP_CTRL_KEY_FROM_RAW_KEYPRESS(sharedTable->pen[penIndex].app_ctrl_action)
                     << "from (" << QString("0x%1").arg(sharedTable->pen[penIndex].app_ctrl_action,0,16) << ")"
                     << "Index"  << sharedTable->pen[penIndex].app_ctrl_sequence_num
                     << "Cell"   << (char)(APP_CTRL_KEY_FROM_RAW_KEYPRESS(sharedTable->pen[penIndex].app_ctrl_action)&255);
        }

        break;

    case PRESENTATION_CONTROLLER:

        // Limit action on pen mode
        switch( presentationDriveState )
        {
        case NO_SYSTEM_DRIVE:
            return;

        case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:
            if( COS_PEN_IS_FROM_NETWORK(sharedTable->pen[penIndex]) ) return;
            break;

        case DRIVE_SYSTEM_WITH_LEAD_PEN:
            if( penIndex != sharedTable->leadPen ) return;
            break;

        case DRIVE_SYSTEM_WITH_ANY_PEN:
            break;
        }

        // Lead pen wsnts to send application controls
        if( sharedTable->pen[penIndex].app_ctrl_action       != APP_CTRL_NO_ACTION        &&
            sharedTable->pen[penIndex].app_ctrl_sequence_num != last_app_ctrl_seq_num[penIndex]  )
        {
            if( sharedTable->pen[penIndex].app_ctrl_action == APP_CTRL_START_SHORTCUT )
            {
                if( QFile::exists(shortcutPresentationFilename) )
                {
                    balloonMessage(QString("Launch %1").arg(shortcutPresentationFilename));
                    QDesktopServices::openUrl(QString("file:%2").arg(shortcutPresentationFilename));
                }
                else
                {
                    balloonMessage(QString("Presentation %1 not found.").arg(shortcutPresentationFilename));;
                }
            }
            else if( ! APP_CTRL_IS_PEN_MODE(sharedTable->pen[penIndex].app_ctrl_action) )
            {
                // Have a presntation control action, so send it to the operating system
                sendApplicationCommandOS(APP_CTRL_APP_TYPE_POWERPOINT,
                                         sharedTable->pen[penIndex].app_ctrl_action );
            }

            last_app_ctrl_seq_num[penIndex] = sharedTable->pen[penIndex].app_ctrl_sequence_num;
        }
        COS_SET_PEN_MODE(sharedTable->pen[penIndex],COS_MODE_DRIVE_PRESENTATION);
        break;

    case NORMAL_OVERLAY:

        COS_SET_PEN_MODE(sharedTable->pen[penIndex],COS_MODE_NORMAL);
        break;
    }
#endif
}



// As we are currently using a shared memory block to communicate between
// tasks, we copy data here at 100Hz. We manage locks (provided by Qt)
// keep a "mostRecent" index between the two blocks in the received data.

void appComms::sendThread()
{
    QTime time;
    int   msec, msec_next, delay;
    int   message_number      = 0;
    int   message_number_read = 0;
    int   last_gesture_seq_num[MAX_PENS];
    int   last_command_seq_num[MAX_PENS];

    time.start();   // Just keep this running
    msec_next = (time.msec() + RATE_TIMESTEP_MS) % 1000;

    for(int pen=0; pen<MAX_PENS; pen++)
    {
        last_gesture_seq_num[pen] = sharedTable->gesture_sequence_num[pen];
        last_command_seq_num[pen] = sharedTable->pen[pen].app_ctrl_sequence_num;
    }

    quitRequest = false;

    while( ! quitRequest )
    {
        // Keep a lead pen, a mouse drive pen and a presentation drive pen
        updateActivePenNumbers();

        // Do we drive the mouse or a presentation. If so from which pen & let apps know it's been used.
        applyActionsToLocalSystem(last_gesture_seq_num, last_command_seq_num);
#if 0
        switch(systemDriveState)
        {
        case DRIVE_SYSTEM_WITH_LEAD_PEN:

            // If specified pen vanishes/stops being willing, stop driving mouse
            if( COS_PEN_IS_ACTIVE(sharedTable->pen[sharedTable->leadPen]) )
            {
                // Lead pen is present
                applyActionsToLocalSystem(sharedTable->leadPen, last_gesture_seq_num, last_command_seq_num);
            }
            else
            {
                // Lost lead pen
                systemDriveState = NO_SYSTEM_DRIVE;
            }
            break;


        case DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN:

            // Don't fallthrough to the pen drive code if it is a network connection
            if( COS_PEN_IS_FROM_NETWORK(sharedTable->pen[mouseControlPenIndex]) ) break;
            // Not a network connection, so fallthrough

        case DRIVE_SYSTEM_WITH_ANY_PEN:
            // If specified pen vanishes/stops being willing, check for another mouse
            if( COS_PEN_IS_ACTIVE(sharedTable->pen[mouseControlPenIndex]) &&
                sharedTable->penMode[mouseControlPenIndex] != NORMAL_OVERLAY )
            {
                // systemDrive pen is present
                applyActionsToLocalSystem(mouseControlPenIndex, last_gesture_seq_num, last_command_seq_num);
            }
            else
            {
                // Don't have an active pen: check for a new one
                // TODO: pick up new pen when it wants to drive the system.

                for(int s=0; s<MAX_PENS; s++)
                {
                    mouseControlPenIndex ++;
                    if( mouseControlPenIndex>MAX_PENS ) mouseControlPenIndex = 0;

                    if( COS_PEN_IS_ACTIVE(sharedTable->pen[mouseControlPenIndex]) &&
                        sharedTable->penMode[mouseControlPenIndex] != NORMAL_OVERLAY )
                    {
                        qDebug() << "New system drive pen:" << mouseControlPenIndex;
                        break;
                    }
                }
            }

            break;

        case NO_SYSTEM_DRIVE:
            break;
        }
#endif
        // And copy the data (the locks may make this slower)
        copyDataToSharedMemory(message_number,message_number_read);

        // Is there anyone listening on the other end?
        // Say yes if a message was read with the last 0.5 seconds.
        sharedTable->applicationIsConnected = ((message_number_read-message_number) < (500/RATE_TIMESTEP_MS));

        // How long to sleep to wait for 1/100 sec (10 mSec)
        msec = time.msec();

        delay = msec_next - msec;
        if( delay>0 ) msleep(delay);

        msec_next = (msec + RATE_TIMESTEP_MS) % 1000;    // Note that this can be greater than 1000, but not <10
    } // while( ! quitRequest )
}

// Manage copying changes from sysTray->applications & from applications->sysTray
void appComms::copyDataToSharedMemory(int &message_number, int &message_number_read)
{
    message_number ++;
#ifndef NO_SHARED_MEMORY_TEST
    externalShare->lock();
#endif
    memcpy(externalData->pen,sharedTable->pen,MAX_PENS*sizeof(pen_state_t));

    for(int pen=0; pen<MAX_PENS; pen++)
    {
        // Essentially, a logical OR, and clear the source, in each direction
        if( externalData->settings_changed_by_client[pen] == COS_TRUE )
        {
            qDebug() << "Pen settings for pen" << pen << "changed by freeStyle";

            memcpy(&(sharedTable->settings[pen]), &(externalData->settings[pen]),
                                   sizeof(pen_settings_t));
            sharedTable->settings_changed_by_client[pen]  = true;
            externalData->settings_changed_by_client[pen] = COS_FALSE;

            // Update settings for this pen
            pen_settings_t penSettings;

            memset(&penSettings, 0, sizeof(penSettings));
            strncpy(penSettings.users_name, sharedTable->settings[pen].users_name,
                    MAX_USER_NAME_SZ-2); // leave a null at the end
            penSettings.sensitivity = sharedTable->settings[pen].sensitivity;
            penSettings.colour[0]   = sharedTable->settings[pen].colour[0];
            penSettings.colour[1]   = sharedTable->settings[pen].colour[1];
            penSettings.colour[2]   = sharedTable->settings[pen].colour[2];
            for(int button=0; button<NUM_MAPPABLE_BUTTONS; button++ )
            {
                penSettings.button_order[button] = button; // TODO: button mapping
            }

            if( dongleReadtask->updatePenSettings(pen, &penSettings) )
            {
                qDebug() << "Settings chnaged for pen" << pen;
            }
        }
        // Essentially, a logical OR, and clear the source, in each direction
        if( sharedTable->settings_changed_by_systray[pen] )
        {
            qDebug() << "Pen settings for pen" << pen << "changed by sysTray";

            memcpy(&(externalData->settings[pen]), &(sharedTable->settings[pen]),
                                    sizeof(pen_settings_t));
            externalData->settings_changed_by_systray[pen] = COS_TRUE;
            sharedTable->settings_changed_by_systray[pen]  = false;
            strcpy(externalData->networkPenAddress[pen],sharedTable->networkPenAddress[pen]);
        }
    }

    externalData->message_number = message_number;
#ifndef NO_SHARED_MEMORY_TEST
    externalShare->unlock();
#endif
    message_number_read = externalData->message_number_read;
}



// Getters and setters...

system_drive_t appComms::getSystemDriveState(void)
{
    return systemDriveState;
}

void  appComms::setMouseDriveState(system_drive_t state)
{
    if( state == DRIVE_SYSTEM_WITH_LEAD_PEN || state == DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN ||
        state == DRIVE_SYSTEM_WITH_ANY_PEN  || state == NO_SYSTEM_DRIVE )
    {
        systemDriveState = state;

        qDebug() << "setMouseDriveState()" << state;
    }
}

system_drive_t appComms::getPresentationDriveState(void)
{
    return presentationDriveState;
}

void  appComms::setPresentationDriveState(system_drive_t state)
{
    if( state == DRIVE_SYSTEM_WITH_LEAD_PEN || state == DRIVE_SYSTEM_WITH_ANY_LOCAL_PEN ||
        state == DRIVE_SYSTEM_WITH_ANY_PEN  || state == NO_SYSTEM_DRIVE )
    {
        presentationDriveState = state;

        qDebug() << "setPresentationDriveState()" << state;
    }
}

int  appComms::getLeadPenIndex(void)
{
    return mouseControlPenIndex;
}

void  appComms::setLeadPenIndex(int index)
{
    if( leadPenPolicy != MANUAL_PEN_SELECT )
    {
        qDebug() << "Internal error: attempted to set lead pen, but not in manual lead select mode";
        return;
    }
    if( index>=0 && index<MAX_PENS )
    {
        mouseControlPenIndex = index;

        qDebug() << "setLeadPenIndex()" << index;
    }
}

void  appComms::setLeadPenPolicy(lead_select_policy_t newPolicy)
{
    switch( newPolicy )
    {
    case NO_LEAD_PEN:                   // GUI: useLeadPen: Enable/disbale
    case USE_FIRST_CONNECTED_PEN:       // GUI: leadSelectPolicy: 0
    case USE_FIRST_CONNECTED_LOCAL_PEN: // GUI: leadSelectPolicy: 1
    case MANUAL_PEN_SELECT:             // GUI: leadSelectPolicy: 2

        leadPenPolicy = newPolicy;
        qDebug() << "setLeadPenPolicy()" << newPolicy;
        break;

    default:
        qDebug() << "Bad new lead pen select policy:" << newPolicy;
    }
}

void  appComms::enableGestures(void)
{
    qDebug() << "Enable gestures.";

    gesturesEnabled = true;
}

void  appComms::disableGestures(void)
{
    qDebug() << "Disable gestures.";

    gesturesEnabled = false;
}

bool  appComms::gesturesAreEnabled(void)
{
    return gesturesEnabled;
}

