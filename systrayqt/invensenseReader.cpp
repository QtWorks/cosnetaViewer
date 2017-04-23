#include "invensenseReader.h"

#ifdef INVENSENSE_EV_ARM

#include <QThread>
#include "main.h"
#include "invensense_low_level.h"
#include "gestures.h"

#include <iostream>

QString invsDebugString = "No debug yet";

invensenseReader::invensenseReader(sysTray_devices_state_t *table, QObject *parent) : QObject(parent)
{
    sharedTable    = table;
    portOpen       = false;
    closeDeviceCmd = true;
}

invensenseReader::~invensenseReader()
{
}

void invensenseReader::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(readThread()) );
}

void invensenseReader::readThread()
{
    invensense_sample_t sample;
    int                 keypress;
    bool                sendShift;
    bool                sendControl;
    QPoint              mouse_loc;
    QDesktopWidget      desktop;
    int                 srch;

    closeDeviceCmd = false;

    while( true )
    {
#ifdef DEBUG_TAB
        invsDebugString = "InvS reader waiting for start command.";
#endif
        // Wait for enable
        while( closeDeviceCmd ) msleep( 200 );

        while( ! closeDeviceCmd )
        {
#ifdef DEBUG_TAB
            invsDebugString = "InvS reader waiting for open port.";
#endif
            // This will generally wait for the device to be plugged in.
            // May want to change the open to only open if not already done (in USB code).
            if( invensenseOpenDevice(portNum) )
            {
#ifdef DEBUG_TAB
                invsDebugString = "InvS reader: port open.";
#endif
                balloonMessage("Serial device detected by invensense driver.");

                portOpen = true;

                // Find a free slot
                sharedTable->addRemoveDeviceMutex->lock();

                for(srch=0; srch<MAX_PENS; srch++)
                {
                    if( sharedTable->pen[srch].active != COS_TRUE )
                    {
#ifdef DEBUG_TAB
                        invsDebugString = QString("InvS: Found slot %1").arg(srch);
#endif
                        std::cerr << "InvS: Found slot " << srch << std::endl;

                        sharedTable->pen[srch].active = COS_TRUE;
                        sharedTable->locCalc[srch]->updateScreenGeometry(&desktop);
                        break;
                    }
                }

                sharedTable->addRemoveDeviceMutex->unlock();

                while( srch<MAX_PENS &&          // That is, we found a slot ;)
                       ! closeDeviceCmd &&
                       invensenseReadSample( &sample ) )
                {
                    // maintain mouse position (NB updates shared data area.)
                    sharedTable->locCalc[srch]->update( &sample );
#ifdef DEBUG_TAB
                    invsDebugString = QString("InvS: (%1,%2)")
                                          .arg(sharedTable->pen[srch].screenX)
                                          .arg(sharedTable->pen[srch].screenY);
#endif
                    // We have a new sample: call code to manage gestures and 3D cursor position

                    sharedTable->gestCalc[srch].update( &sample );
#if 0 // Move output to apps or mouse to appComms
                    if( sharedTable->gestCalc[srch].canDriveMouse() )
                    {
                        // Extract the mouse position & then set it

                        sharedTable->locCalc[srch]->getCurrent2Dpos(&mouse_loc);

                        // Send location to the mouse (fails on Linux)
                        QCursor::setPos(mouse_loc.x(), mouse_loc.y());

                        // ?? replace next call with
                        // ?? QMouseEvent ev(QEvent::MouseButtonPress,mouse_loc,Qt::LeftButton,Qt::LeftButton,Qt::ShiftModifier);
                        // ?? qApp->postEvent(desktop,ev);
                        // ?? ...
                        // ?? qApp->sendPostedEvents();
                        setMouseButtons(&mouse_loc, sample.buttons);
                    }
                    else
                    {
                        // Retreive gesture command and send it (basically, a keypress)

                        if(  sharedTable->gestCalc[srch].gestureGetKeypress(&keypress, &sendShift, &sendControl) )
                        {
                            // Send it to the operating system

                            sendKeypressToOS(keypress, sendShift, sendControl);
                        }
                    }
#endif
                }

                if( ! closeDeviceCmd )
                {
#ifdef DEBUG_TAB
                    invsDebugString = "InvS reader: read from port failed.";
#endif
                    msleep(1000);
                    balloonMessage("Invensense device removed.");
                    msleep(1000);
                }

                if( srch<MAX_PENS )
                {
                    // NB We don't lock here as it's atomic
                    sharedTable->pen[srch].active = COS_FALSE;
                }

                invensenseCloseDevice();
                portOpen = false;
            }
            else
            {
                // We failed to open the device. wait for 0.5s and try again
                msleep(500);
            }
        }

    }

    return;
}

void invensenseReader::stopCommand(void)
{
    closeDeviceCmd = true;
}

void invensenseReader::startCommand(int port)
{
    portNum        = port;
    closeDeviceCmd = false;
}

void invensenseReader::waitForStop(void)
{
    while( portOpen ) msleep(10);
}


#endif

