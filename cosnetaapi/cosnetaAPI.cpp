#define COSNETA_LIBRARY_BUILD 1

// System includes
#include <QtCore>
#include <QSharedMemory>

// Project includes
#include "trayComms.h"
#include "cosnetaAPI.h"
#include "sharedMem.h"

// Global variables (states mainly)
static bool               connectedToSharedMemory = false;
static QSharedMemory     *externalShare;
static cosneta_devices_t *externalData            = (cosneta_devices_t *)NULL;
static QThread            readerThread;
static trayComms         *reader;



// //////////////////////////////////////////
// A cross platform sleep class, and function

class Sleeper : public QThread
{
public:
    static void wait_in_ms(unsigned long msecs) {QThread::msleep(msecs);}
};
void msleep(unsigned long t)
{
    Sleeper::wait_in_ms(t);
}



// ///////////////
// Local functions
static bool connectToShare(void)
{
    if( connectedToSharedMemory ) return true;

    externalShare = new QSharedMemory( SHARED_MEMORY_KEY );

    if( ! externalShare->attach() )
    {
        delete externalShare;
        externalShare = NULL;

        return false;
    }
    else
    {
        // Later, we could check the size of the memory area we have attached to...
        externalData = (cosneta_devices_t *)externalShare->data();

        if( ! externalData ) // Shouldn't happen if attach() succeeded.
        {
            externalShare->detach();

            connectedToSharedMemory = false;

            return false;
        }
    }

    connectedToSharedMemory = true;

    // Now we have a share, we can create the shared memory reader task.
    // NB We haven't started the task (but not start it).
    reader = new trayComms(externalShare);

    return true;
}


// //////////////////////////////////////////////////////////////////////////////////
// The thread to poll for changes to allow events to be sent to a registered function




// //////////////////
// The API functions.

COS_BOOL cos_register_pen_event_callback(void (*callback)(pen_state_t *pen, int activePensMask), int maxRateHz, COS_BOOL onlyOnChanged)
{
    if( ! connectToShare() ) return COS_FALSE;

    // And store the callback function
    reader->setCallback(callback);
    reader->setMaxRate(maxRateHz);
    reader->setOnlyCallbackOnChanges(onlyOnChanged);

    // And start the reader task
    reader->doConnect(readerThread);
    reader->moveToThread(&readerThread);
    readerThread.start(QThread::NormalPriority);

    return COS_TRUE;
}

COS_BOOL cos_deregister_callback(void)
{
    // Stop the thread command
    if( ! reader->stopCommand() )
    {
        // It didn't stop (probably in user code), so force it to stop
        readerThread.terminate();
    }

    if( connectedToSharedMemory )
    {
        externalShare->detach();

        connectedToSharedMemory = false;
    }

    return COS_TRUE;
}

// For polling use

COS_BOOL cos_get_pen_data(int penNum, pen_state_t* state)
{
    if( ! connectToShare() ) return COS_FALSE;
    if( penNum<0 || penNum>=MAX_PENS ) return COS_FALSE;

    // And copy out the data for the specified pen (but use a lock to ensure consistent data)
    externalShare->lock();
    memcpy(state,&externalData->pen[penNum],sizeof(pen_state_t));
    externalShare->unlock();

    return COS_TRUE;
}

// Settings interface
COS_BOOL cos_retreive_pen_settings(int pen_num, pen_settings_t *settings)
{
    if( ! connectToShare() ) return COS_FALSE;
    if( pen_num<0 || pen_num>=MAX_PENS ) return COS_FALSE;

    // And copy out the data for the specified pen (but use a lock to ensure consistent data)
    externalShare->lock();
    memcpy(settings,&externalData->settings[pen_num],sizeof(pen_settings_t));
    externalShare->unlock();

    return COS_TRUE;
}

COS_BOOL cos_update_pen_settings(int pen_num, pen_settings_t *settings)
{
    if( ! connectToShare() ) return COS_FALSE;
    if( pen_num<0 || pen_num>=MAX_PENS ) return COS_FALSE;

    // And copy out the data for the specified pen (but use a lock to ensure consistent data)
    externalShare->lock();
    memcpy(&externalData->settings[pen_num],settings,sizeof(pen_settings_t));
    externalData->settings_changed_by_client[pen_num] = COS_TRUE;
    externalShare->unlock();

    return COS_TRUE;
}


COS_BOOL cos_get_net_pen_addr(int penNum, char *address, COS_BOOL *wants_view)
{
    address[0] = (char)0;

    if( ! connectToShare() )                                   return COS_FALSE;
    if( penNum<0 || penNum>=MAX_PENS )                         return COS_FALSE;
    if( ! COS_PEN_IS_FROM_NETWORK(externalData->pen[penNum]) ) return COS_FALSE;

    // And copy out the data for the specified pen (but use a lock to ensure consistent data)
    externalShare->lock();
    memcpy(address,&externalData->networkPenAddress[penNum],NET_PEN_ADDRESS_SZ);
    *wants_view = COS_PEN_CAN_VIEW(externalData->pen[penNum]);
    externalShare->unlock();

    return COS_TRUE;
}


int cos_num_connected_pens(void)
{
    if( ! connectToShare() ) return -1;

    int numPens = 0;

    for(int pen_num=0; pen_num<MAX_PENS; pen_num ++)
    {
        if( COS_PEN_IS_ACTIVE(externalData->pen[pen_num]) ) numPens ++;
    }

    return numPens;
}


COS_BOOL cos_API_is_running(void)
{
    return connectToShare() ? COS_TRUE : COS_FALSE;
}
