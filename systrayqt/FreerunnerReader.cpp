#include <QtGui>
#include <QString>
#include <QThread>
#include <QObject>
#include <QDebug>
#include <QCursor>
#include "stdio.h"


#include <iostream>

#include "main.h"
#include "FreerunnerReader.h"
#include "freerunner_low_level.h"
#include "pen_location.h"
#include "gestures.h"
#include "build_options.h"
#include "hardwareCalibration.h"

#ifdef POC1_READ_SUPPORT


#ifdef DEBUG_TAB

#include "math.h"

QString extraDebugString = "Unset";   // Used in debug displays.

#endif


FreerunnerReader::FreerunnerReader(sysTray_devices_state_t *table, QObject *parent) : QObject(parent)
{
    // Reference for local update
    sharedTable = table;

    // Initial values for calibration. NB For multiple pens we may want arrays here (but hopefully wont need any as wont need to re-callibrate real pens).

    // Gyros
    analogueGyroZeroPoint[0] = COS_ANA_GYRO_ZERO_G1;
    analogueGyroZeroPoint[1] = COS_ANA_GYRO_ZERO_G2;
    analogueGyroZeroPoint[2] = COS_ANA_GYRO_ZERO_G3;
    analogueGyroScale[0] = COS_ANA_GYRO_SCALE_G1;
    analogueGyroScale[1] = COS_ANA_GYRO_SCALE_G2;
    analogueGyroScale[2] = COS_ANA_GYRO_SCALE_G3;

    // Accelerometers
    accelerometerZeroPoint[0][0] = COS_ACC1_ZERO_X;
    accelerometerZeroPoint[0][1] = COS_ACC1_ZERO_Y;
    accelerometerZeroPoint[0][2] = COS_ACC1_ZERO_Z;
    accelerometerZeroPoint[1][0] = COS_ACC2_ZERO_X;
    accelerometerZeroPoint[1][1] = COS_ACC2_ZERO_Y;
    accelerometerZeroPoint[1][2] = COS_ACC2_ZERO_Z;
    accelerometerScale[0][0] = COS_ACC1_SCALE_X;
    accelerometerScale[0][1] = COS_ACC1_SCALE_Y;
    accelerometerScale[0][2] = COS_ACC1_SCALE_Z;
    accelerometerScale[1][0] = COS_ACC2_SCALE_X;
    accelerometerScale[1][1] = COS_ACC2_SCALE_Y;
    accelerometerScale[1][2] = COS_ACC2_SCALE_Z;

    callibrating      = false;
    poc1_read_enabled = false;
}

FreerunnerReader::~FreerunnerReader()
{
}

void FreerunnerReader::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(readThread()) );
}

void FreerunnerReader::stopCommand(void)
{
    quitRequest = true;
}

void FreerunnerReader::readThread()
{
    cosneta_sample_t  sample;
    int               keypress;
    bool              sendShift;
    bool              sendControl;
    QPoint            mouse_loc;
    QDesktopWidget    desktop;
    int               srch;

    quitRequest = false;

    while( ! quitRequest )
    {
#ifdef DEBUG_TAB
        if( useMouseData )
        {
            while( useMouseData && ! quitRequest )
            {
                sharedTable->addRemoveDeviceMutex->lock();

                for(srch=0; srch<MAX_PENS; srch++)
                {
                    if( sharedTable->pen[srch].active != COS_TRUE )
                    {
                        sharedTable->pen[srch].active = COS_TRUE;
                        sharedTable->locCalc[srch]->updateScreenGeometry(&desktop);
                        break;
                    }
                }

                sharedTable->addRemoveDeviceMutex->unlock();

                if( srch >= MAX_PENS ) // Failed to find
                {
                    balloonMessage("No free slots for mouse input.");
                }
                else
                {
                    sharedTable->pen[srch].active  = COS_TRUE;
                    sharedTable->pen[srch].buttons = 0;

                    while( useMouseData && ! quitRequest )
                    {
                        // Read mouse input
                        mouse_loc = QCursor::pos();

                        // Update data
                        sharedTable->pen[srch].screenX = mouse_loc.x();
                        sharedTable->pen[srch].screenY = mouse_loc.y();
                        sharedTable->pen[srch].buttons = mouseButtons;

                        // Wait 20 mSec
                        msleep(20);
                    }
                }
            }
        }
        else if( generateSampleData )
        {
            // Initialise positions and velocities
            double x[MAX_PENS], y[MAX_PENS], vx[MAX_PENS], vy[MAX_PENS];
            double angle;
            int    onOffDelay[8];
            int    delayTillMenu[8];
            bool   penOn[8];
            bool   optionOn[8];
            int    p;

            for(p=0; p<MAX_PENS; p++)
            {
                x[p] = qrand() % desktop.width();
                y[p] = qrand() % desktop.height();

                angle = 0.01 * (qrand() % 36000);
                vx[p] = 0.5 * sin(angle);
                vy[p] = 0.5 * cos(angle);

                penOn[p]         = false;
                onOffDelay[p]    = qrand() & 511;
                delayTillMenu[p] = qrand() & 1023;
            }

            // Keep on updating, till quit request

            while( ! quitRequest && generateSampleData && ! useMouseData )
            {
                // Update current state

                for(p=0; p<MAX_PENS; p++)
                {
                    x[p] += vx[p];
                    if( x[p]<0 || x[p]>desktop.width() )
                    {
                        vx[p] = -vx[p];
                        x[p] +=  vx[p];
                    }
                    y[p] += vy[p];
                    if( y[p]<0 || y[p]>desktop.height() )
                    {
                        vy[p] = -vy[p];
                        y[p] +=  vy[p];
                    }

                    onOffDelay[p] --;
                    if( onOffDelay[p] < 0 )
                    {
                        penOn[p]      = ! penOn[p];
                        onOffDelay[p] = qrand() & 255;
                    }

                    delayTillMenu[p] --;
                    if( delayTillMenu[p]<1 )
                    {
                        optionOn[p]      = true;
                        delayTillMenu[p] = qrand() & 1023;
                    }
                    else
                    {
                        optionOn[p] = false;
                    }
                }

                // Copy to shared

                for(p=0; p<MAX_PENS; p++)
                {
                    sharedTable->pen[p].active  = COS_TRUE;
                    sharedTable->pen[p].screenX = x[p];
                    sharedTable->pen[p].screenY = y[p];
                    sharedTable->pen[p].buttons = penOn[p] ? 1 : 0 | optionOn[p] ? 8 : 0;
                }

                // wait
                msleep(10);
            }

            // We've bombed out of here, so reset the state for the next pass
            // which will use the actual hardware state; so all unused.

            for(p=0; p<MAX_PENS; p++)
            {
                sharedTable->pen[p].active  = COS_FALSE;
            }
        }
        else
        {
#endif
        // This will generally wait for the device to be plugged in.
        // May want to change the open to only open if not already done (in USB code).
#ifdef DEBUG_TAB
        if( ! generateSampleData && poc1_read_enabled && openHardwareDevice() && ! useMouseData )
#else
        if( openHardwareDevice() )
#endif
        {
            balloonMessage("Freerunner device detected.");

            // Find a free slot
            sharedTable->addRemoveDeviceMutex->lock();

            for(srch=0; srch<MAX_PENS; srch++)
            {
                if( sharedTable->pen[srch].active != COS_TRUE )
                {
                    sharedTable->pen[srch].active = COS_TRUE;
                    sharedTable->locCalc[srch]->updateScreenGeometry(&desktop);
                    break;
                }
            }

            sharedTable->addRemoveDeviceMutex->unlock();
#ifdef DEBUG_TAB
            while( srch<MAX_PENS &&          // That is, we found a slot ;)
                   ! quitRequest && poc1_read_enabled && ! generateSampleData &&
                   readSample( &sample ) )
#else
            while( srch<MAX_PENS &&          // That is, we found a slot ;)
                   ! quitRequest &&
                   readSample( &sample ) )
#endif
            {
                // Apply calibration
                callibrateSample(&sample);

                // maintain mouse position (NB updates shared data area.)
                sharedTable->locCalc[srch]->update( &sample );

                // We have a new sample: call code to manage gestures and 3D cursor position

                sharedTable->gestCalc[srch].update( &sample );
#if 0 // Move the output to apps ot mouse to appComms
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
#ifdef DEBUG_TAB
                static int rate_reduce = 0;
                rate_reduce ++;
                if( rate_reduce> 100 )
                {
                    QString dbg;
                    dbg  = "Using pen: " + QString::number(srch);
                    dbg += "\nPen active states: [";
                    for(int p=0; p<MAX_PENS; p++)
                    {
                        dbg += QString::number(sharedTable->pen[p].active) + " ";
                    }
                    dbg += dbg + "]\nMouse loc: (";
                    dbg += QString::number(sharedTable->pen[srch].screenX) + ",";
                    dbg += QString::number(sharedTable->pen[srch].screenX) + ") - ";
                    dbg += "Pen #" + QString::number(srch);
                    if(callibrating) dbg += "\nCALLIBRATING...";
                    extraDebugString = dbg;
                    rate_reduce      = 0;
                }
#endif
            }

            if( ! quitRequest && poc1_read_enabled ) balloonMessage("Freerunner device removed.");

            if( srch<MAX_PENS )
            {
                // NB We don't lock here as it's atomic
                sharedTable->pen[srch].active = COS_FALSE;
            }

            // If we are here, the device was opened successfully
            closeHardwareDevice();
        }

        msleep(100);
#ifdef DEBUG_TAB
        }
#endif
    }
    
    return;
}



// Calibration

// Static calibration. Just do the zero points for now (don't try with g yet)

void FreerunnerReader::startCallibration(void)
{
    accelSum[0][0] = 0.0;
    accelSum[0][1] = 0.0;
    accelSum[0][2] = 0.0;

    accelSum[1][0] = 0.0;
    accelSum[1][1] = 0.0;
    accelSum[1][2] = 0.0;

    gyroSum[0] = 0.0;
    gyroSum[1] = 0.0;
    gyroSum[2] = 0.0;

    numSamples = 0;

    callibrating = true;
}

void FreerunnerReader::endCallibration(void)
{
    if( ! callibrating ) return;

    // Zero points are the averages
    accelerometerZeroPoint[0][0] = (accelSum[0][0]) / numSamples;
    accelerometerZeroPoint[0][1] = (accelSum[0][1]) / numSamples;
    accelerometerZeroPoint[0][2] = (accelSum[0][2]) / numSamples;

    accelerometerZeroPoint[1][0] = (accelSum[1][0]) / numSamples;
    accelerometerZeroPoint[1][1] = (accelSum[1][1]) / numSamples;
    accelerometerZeroPoint[1][2] = (accelSum[1][2]) / numSamples;

    analogueGyroZeroPoint[0] = (gyroSum[0]) / numSamples;
    analogueGyroZeroPoint[1] = (gyroSum[1]) / numSamples;
    analogueGyroZeroPoint[2] = (gyroSum[2]) / numSamples;

    callibrating = false;
}

void FreerunnerReader::accumulateStaticCallibration(cosneta_sample_t *sample)
{
    accelSum[0][0] += sample->accel1[0];
    accelSum[0][1] += sample->accel1[1];
    accelSum[0][2] += sample->accel1[2];

    accelSum[1][0] += sample->accel2[0];
    accelSum[1][1] += sample->accel2[1];
    accelSum[1][2] += sample->accel2[2];

    gyroSum[0] += sample->angle_rate[0];
    gyroSum[1] += sample->angle_rate[1];
    gyroSum[2] += sample->angle_rate[2];

    numSamples ++;
}

// Apply current callibration data
void FreerunnerReader::callibrateSample(cosneta_sample_t *sample)
{
    if( callibrating ) accumulateStaticCallibration(sample);

    sample->accel1[0] = ((sample->accel1[0])- accelerometerZeroPoint[0][0])*accelerometerScale[0][0];
    sample->accel1[1] = ((sample->accel1[1])- accelerometerZeroPoint[0][1])*accelerometerScale[0][1];
    sample->accel1[2] = ((sample->accel1[2])- accelerometerZeroPoint[0][2])*accelerometerScale[0][2];

    sample->accel2[0] = ((sample->accel2[0])- accelerometerZeroPoint[1][0])*accelerometerScale[1][0];
    sample->accel2[1] = ((sample->accel2[1])- accelerometerZeroPoint[1][1])*accelerometerScale[1][1];
    sample->accel2[2] = ((sample->accel2[2])- accelerometerZeroPoint[1][2])*accelerometerScale[1][2];

    sample->angle_rate[0] = ((sample->angle_rate[0])-analogueGyroZeroPoint[0])*analogueGyroScale[0];
    sample->angle_rate[1] = ((sample->angle_rate[1])-analogueGyroZeroPoint[1])*analogueGyroScale[1];
    sample->angle_rate[2] = ((sample->angle_rate[2])-analogueGyroZeroPoint[2])*analogueGyroScale[2];
}





#ifdef DEBUG_TAB

QString genericDebugString = "Unset";   // Used in debug displays.

       bool    dumpSamplesToFile  = false;
static bool    stopDumpingSamples = false;
static QFile       accel1TextFile;
static QFile       accel2TextFile;
static QFile       gyroTextFile;
static QFile       userInputTextFile;
       QTextStream accel1TextOut;
       QTextStream accel2TextOut;
       QTextStream gyroTextOut;
static QTextStream userInputTextOut;

// Called from the low level reader.
void dumpSamples(QTextStream &out, double time, unsigned short data[], int numData)
{
    int x,y,z;
    int index;
    int sampleNum;

    if( ! dumpSamplesToFile ) return;   // This will happen after first close call

    if( stopDumpingSamples )
    {
        accel1TextFile.flush();
        accel1TextFile.close();
        accel2TextFile.flush();
        accel2TextFile.close();
        gyroTextFile.flush();
        gyroTextFile.close();
        userInputTextFile.flush();
        userInputTextFile.close();

        dumpSamplesToFile = false;

        return;
    }

    out.setRealNumberPrecision(6);
    out.setRealNumberNotation(QTextStream::FixedNotation);

    for(sampleNum=0, index=0; sampleNum < numData; sampleNum++)
    {
        x = data[index++]; if( x >= 32768 ) x  -= 65536;
        y = data[index++]; if( y >= 32768 ) y  -= 65536;
        z = data[index++]; if( z >= 32768 ) z  -= 65536;

        out << time << ",";
        out << x << ",";
        out << y << ",";
        out << z << "\n";
    }
}
void dumpUserInput(double time, unsigned char data[], int dataSize)
{
    int index;

    if( ! dumpSamplesToFile ) return;   // This will happen after first close call
    if( dataSize < 1 )        return;   // No event, and we get a lot of these

    userInputTextOut << time << ":(" << dataSize <<")";
    for(index=0; index < dataSize; index++)
    {
        userInputTextOut << data[index] << " ";
    }
    userInputTextOut << endl;
}

bool startDataSaving(void)
{
    // We need to be talking to a Freerunner to start this.
    if( ! freerunnerDeviceOpened() )
    {
        balloonMessage("Cannot start data save as no Freerunner detected.");
        return false;
    }

    // Select a directory
    QDir dir;
    dir.setCurrent(QDir::homePath());

    // Get a common suffix

    // Make sure that the file classes are not in use

    if( accel1TextFile.isOpen() )    accel1TextFile.close();
    if( accel2TextFile.isOpen() )    accel2TextFile.close();
    if( gyroTextFile.isOpen() )      gyroTextFile.close();
    if( userInputTextFile.isOpen() ) userInputTextFile.close();

    // Create and open the files
    accel1TextFile.setFileName("accel1.csv");
    accel2TextFile.setFileName("accel2.csv");
    gyroTextFile.setFileName("gyro.csv");
    userInputTextFile.setFileName("userInput.txt");

    if( ! accel1TextFile.open(QIODevice::WriteOnly | QIODevice::Text) )    return false;
    if( ! accel2TextFile.open(QIODevice::WriteOnly | QIODevice::Text) )    return false;
    if( ! gyroTextFile.open(QIODevice::WriteOnly | QIODevice::Text) )      return false;
    if( ! userInputTextFile.open(QIODevice::WriteOnly | QIODevice::Text) ) return false;

    accel1TextOut.setDevice(&accel1TextFile);
    accel2TextOut.setDevice(&accel2TextFile);
    gyroTextOut.setDevice(&gyroTextFile);
    userInputTextOut.setDevice(&userInputTextFile);

    // And set the flags
    stopDumpingSamples = false;
    dumpSamplesToFile  = true;

    return true;
}
void stopDataSaving(void)
{
    stopDumpingSamples = true;
}
#endif


#endif // POC1_READ_SUPPORT
