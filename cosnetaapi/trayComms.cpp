#include <QThread>
#include <QTime>

#include "trayComms.h"
#include "cosnetaAPI_internal.h"


// Create/destroy
trayComms::trayComms(QSharedMemory *share, QObject *parent) : QObject(parent)
{
    externalShare           = share;
    onlyCallOnChanges       = true;
    prevActivePenMask = 0;
    minUpdateTimeMs         = 10;   // ie 100Hz
    registered_callback     = NULL;
    memchr(&prevState,0,sizeof(prevState));
    prevState.message_number = -1;

    externalData             = (cosneta_devices_t *)share->data();
}
trayComms::~trayComms()
{
}

// Kick from threading to call: readThread
void trayComms::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(readThread()) );
}

bool trayComms::stopCommand(void)
{

    quitRequest = true;     // Don't detach as it's ours, and we may be restarted.

    // Wait around for up to 0.5 seconds for the user code to exit
    if( inUserCode.tryLock(500) )
    {
        // Not in user code

        msleep(minUpdateTimeMs*2); // It aught to be dead by now

        return ! quitRequest;       // If stopped okay, quit request will be false
    }
    else
    {
        // User code is blocking.

        return false;
    }
}


bool trayComms::setMaxRate(int maxRateHz)
{
    if( maxRateHz > 1000 ) return false;

    minUpdateTimeMs = (1000/maxRateHz);

    return true;
}

void trayComms::setOnlyCallbackOnChanges(bool onlyCallOnChanges)
{
    onlyCallOnChanges = onlyCallOnChanges;
}

void trayComms::setCallback(void (*callback)(pen_state_t *pen, int activePensMask))
{
    registered_callback = callback;
}



// As we are currently using a shared memory block to communicate between
// tasks, we copy data here at 100Hz. We manage locks (provided by Qt)
// keep a "mostRecent" index between the two blocks in the received data.

void trayComms::readThread()
{
    QTime time;
    int   msec, msec_next, delay;
    int   activePensMask = 0;

    time.start();   // Just keep this running
    msec_next = (time.msec() + 100) % 1000;

    quitRequest = false;

    while( ! quitRequest )
    {
        // And copy the data (the locks may make this slow)
        externalShare->lock();
        // Extract and look dir changed data
        if(externalData->message_number != prevState.message_number )
        {
            bool changed     = false;
            int  changedMask;

            for(int p=0; p<MAX_PENS; p++)
                if( COS_PEN_IS_ACTIVE(externalData->pen[p]) )
                    activePensMask |= (1<<p);

            changedMask = activePensMask ^ prevActivePenMask;

            // Check for differences
            if( prevActivePenMask != activePensMask )
            {
                changed = true;
                memcpy(&prevState, externalData, sizeof(cosneta_devices_t));
            }
            else
            {
                // Okay, we'll have to check the active pens for changes
                // (check them all as this is where we do the previous state copy)
                // The logic is that if the pen is active & the data has changed,
                // or if the active/inactive has changed then we have changed data.
                for(int p=0; p<MAX_PENS; p++)
                {
                    int checkMask = 1<<p;

                    if( changedMask & checkMask )
                    {
                        changed = true;
                        memcpy(&(prevState.pen[p]), &(externalData->pen[p]), sizeof(pen_state_t));
                    }
                    else if( activePensMask & checkMask )
                    {
                        if( memcmp(&(externalData->pen[p]), &(prevState.pen[p]), sizeof(pen_state_t)) )
                        {
                            changed = true;
                            memcpy(&(prevState.pen[p]), &(externalData->pen[p]), sizeof(pen_state_t));
                        }
                    }
                }
                prevState.message_number = externalData->message_number;
            }

            // This allows the sysTray to know that data is actually being read
            externalData->message_number_read = externalData->message_number;
        }
        externalShare->unlock();

        inUserCode.lock();  // We dont want to kill this in user code

        // Call the registered function (NB The thread in the sysTray will not update this while we have this lock)
        (*registered_callback)(externalData->pen, activePensMask);

        inUserCode.unlock();


        // How long to sleep to wait for 1/100 sec
        msec = time.msec();

        delay = msec_next - msec;
        if( delay>0 ) msleep(delay);

        msec_next = (msec + 100) % 1000;    // Note that this can be greater than 1000, but not <100
    }

    quitRequest = false;                    // Ready for next run (and a signal that we've exited)
}
