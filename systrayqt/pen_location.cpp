
#include "pen_location.h"
#include "main.h"

#include "float.h"

#include <QDebug>

#ifdef __GNUC__
#include "math.h"
#endif


#include <iostream>

#define DEG_FROM_RAD(rads) ((rads)*180.0/3.1415926536)

pen_location::pen_location(pen_state_t *out, pen_settings_t *settings)
{
    outputState = out;
    penSetings  = settings;

    mousePosition = QPointF(0.0,0.0);

    cursorOffsetX = 0.0;
    cursorOffsetY = 0.0;

    LNDactive  = false;

    LND_X = 0.0;
    LND_Y = 0.0;

    LND_X_Scale = 1.0;
    LND_Y_Scale = 1.0;
}

pen_location::~pen_location()
{
}

void pen_location::updateScreenGeometry(QDesktopWidget *desktop)
{
    // Extract a rectangle of the screen dimensions
    screenInPixels = desktop->screenGeometry();

    // Start with the current mouse position
    mousePosition = QCursor::pos();
}

void pen_location::update(cosneta_sample_t *sample)
{
    static QPointF delta(0.0,0.0);
    QPointF        unfiltered;

    // Basically, we add a value scaled from the pitch and yaw rates to the
    // current cursor position. The scaling is based on screen size and
    // the mouseSensitivity setting.
    unfiltered.setX( (sample->angle_rate[2])*0.002*screenInPixels.width() );
    unfiltered.setY( (sample->angle_rate[1])*0.005*screenInPixels.height() );

    delta = 0.99*delta + 0.01*unfiltered;   // Quite a lot (and a wee bit laggy). Mainly for maximum sensitivity mode.

    // Mouse sensitivity: 1% -> 100%
    if( mouseSensitivity < 2 ) mouseSensitivity = 2;
    mousePosition -= delta * ((mouseSensitivity/100.0));    // Inverted here so buttons are on top...

    // And normalise the 2D coordinates to the screen area
    if( mousePosition.x()<0.0 ) mousePosition.setX(0.0);
    if( mousePosition.y()<0.0 ) mousePosition.setY(0.0);

    if( mousePosition.x()>screenInPixels.width() )  mousePosition.setX(screenInPixels.width());
    if( mousePosition.y()>screenInPixels.height() ) mousePosition.setY(screenInPixels.height());

    // Fill in output data
    outputState->acc[0] = sample->accel1[0];
    outputState->acc[1] = sample->accel1[1];
    outputState->acc[2] = sample->accel1[2];

    outputState->rot_rate[0] = sample->angle_rate[0];
    outputState->rot_rate[1] = sample->angle_rate[1];
    outputState->rot_rate[2] = sample->angle_rate[2];

    outputState->screenX = mousePosition.x();
    outputState->screenY = mousePosition.y();

//    std::cout << "Inv ptr: (" << outputState->screenX << "," << outputState->screenY << ")\n";

    outputState->buttons = sample->buttons;
}

#define mEULAR_ANGLE_Z_FROM_QUAT_YXZ_CONVNETION(q)  \
                             ( atan2( -2*(q[1]*q[2]-q[0]*q[3]), 1-2*(q[1]*q[1]+q[3]*q[3]) ) )
void pen_location::update(invensense_sample_t *sample)
{
    // First calculate a horizontal plane rotation and a pitch angle
    // (Assuming that we have a quaternion of the form: [w,x,y,z] )

    double roll = acos(sample->quaternion[0]); // From invensense teapot demo code

    double horizDist = sqrt(sample->quaternion[0]*sample->quaternion[0] +
                            sample->quaternion[1]*sample->quaternion[1]  );
    //double horizRot = atan2(sample->quaternion[2],horizDist);
    double horizRot = - mEULAR_ANGLE_Z_FROM_QUAT_YXZ_CONVNETION(sample->quaternion);
    double vertRot  = atan2(sample->quaternion[1],sample->quaternion[0]);

    // Populate angles and synthesise angle rates (for gestures)

    outputState->rot_rate[0] = horizRot - outputState->rot[0];
    outputState->rot_rate[1] = vertRot  - outputState->rot[1];
    outputState->rot_rate[2] = 0.0;

    outputState->rot[0] = horizRot;
    outputState->rot[1] = vertRot;
    outputState->rot[2] = 0.0;

    // Mouse sensitivity: 1% -> 100%
    //if( mouseSensitivity < 2 ) mouseSensitivity = 2;

    mousePosition.setX( cursorOffsetX+(DEG_FROM_RAD(horizRot))*(screenInPixels.width()/40.0) );
    mousePosition.setY( cursorOffsetY+(DEG_FROM_RAD(vertRot))*(screenInPixels.height()/20.0) );

    // And normalise the 2D coordinates to the screen area
    if( mousePosition.x()<0.0 )
    {
        cursorOffsetX -= mousePosition.x();
        mousePosition.setX(0.0);
    }
    else  if( mousePosition.x()>screenInPixels.width() )
    {
        cursorOffsetX -= (mousePosition.x() - screenInPixels.width());
        mousePosition.setX(screenInPixels.width());
    }

    if( mousePosition.y()<0.0 )
    {
        cursorOffsetY -= mousePosition.y();
        mousePosition.setY(0.0);
    }
    else if( mousePosition.y()>screenInPixels.height() )
    {
        cursorOffsetY -= (mousePosition.y() - screenInPixels.height());
        mousePosition.setY(screenInPixels.height());
    }

    outputState->screenX = mousePosition.x();
    outputState->screenY = mousePosition.y();

    outputState->buttons = 0;
}


// This is the real deal. This interprets the data received on the flipstick
// for a particular pen that is sent over usb to this task.

void pen_location::update(pen_sensor_data_t *sample)
{
    double quaternion[4];
    double q;

    if( ! sample || ! outputState ) return;

    // Decode the invensense data packet, as per the invensense code
    for(int i=0; i<4; i++)
    {
        q = sample->quaternion[i*4 + 3]*(1<<24) +
            sample->quaternion[i*4 + 2]*(1<<16) +
            sample->quaternion[i*4 + 1]*(1<<8)  +
            sample->quaternion[i*4 + 0];

        if( q>2147483648 ) q -= 4294967296;

        quaternion[i] = q*1.0/(1<<30);
    }

    // First calculate a horizontal plane rotation and a pitch angle
    // (Assuming that we have a quaternion of the form: [w,x,y,z] )

    double roll = acos(quaternion[0]); // From invensense teapot demo code

//    double horizDist = sqrt(quaternion[0]*quaternion[0] + quaternion[1]*quaternion[1] );
    //double horizRot = atan2(quaternion[2],horizDist);
    double horizRot = - mEULAR_ANGLE_Z_FROM_QUAT_YXZ_CONVNETION(quaternion);
    double vertRot  = atan2(quaternion[1],quaternion[0]);

    // Populate angles and synthesise angle rates (for gestures)

    outputState->rot_rate[0] = horizRot - outputState->rot[0];
    outputState->rot_rate[1] = vertRot  - outputState->rot[1];
    outputState->rot_rate[2] = roll;

    outputState->rot[0] = horizRot;
    outputState->rot[1] = vertRot;
    outputState->rot[2] = 0.0;

    // Copy pen button data over
    outputState->buttons = sample->buttons[0] | (sample->buttons[1]&255)<<8;
#ifdef DEBUG_TAB
    if( outputState->buttons ) qDebug() << "Buttons" << outputState->buttons;
#endif

    // Mouse sensitivity: 1% -> 100%
    //if( mouseSensitivity < 2 ) mouseSensitivity = 2;

    if( (outputState->buttons & BUTTON_TIP_NEAR_SURFACE) == BUTTON_TIP_NEAR_SURFACE )
    {
        // Mouse mode

        short x_vel, y_vel;

        x_vel = (sample->ms_vel_x[0]<<8)|sample->ms_vel_x[1];
        y_vel = (sample->ms_vel_y[0]<<8)|sample->ms_vel_y[1];

        if( x_vel != 0 || y_vel != 0 ) qDebug() << "Vel XY:" << x_vel << y_vel;

        if( ! LNDactive ) // Wasn't active on last pass
        {
            qDebug() << "Transition to LND mode. sensitivity =" <<
                        (100 - penSetings->sensitivity) << "CursorOffset:" <<
                        cursorOffsetX << cursorOffsetY << "Buttons =" << outputState->buttons;

            // Initialise the LND coordinates
            LND_X = cursorOffsetX+(DEG_FROM_RAD(horizRot))*(screenInPixels.width()/40.0);
            LND_Y = cursorOffsetY+(DEG_FROM_RAD(vertRot))*(screenInPixels.height()/20.0);

            LNDactive = true;
        }

        LND_X += (x_vel * LND_X_Scale);
        LND_Y += (y_vel * LND_Y_Scale);

        if( LND_X<0 )                            LND_X = 0;
        else if( LND_X>=screenInPixels.width() ) LND_X = screenInPixels.width()-1;

        if( LND_Y<0 )                             LND_Y = 0;
        else if( LND_Y>=screenInPixels.height() ) LND_Y = screenInPixels.height()-1;

        mousePosition.setX( LND_X );
        mousePosition.setY( LND_Y );

        outputState->screenX = LND_X;
        outputState->screenY = LND_Y;
    }
    else
    {
        // Inverted scale. Switch it around.
        int sens = 100 - penSetings->sensitivity;
        // Too simple quadratic fits s.t. for  x_scale: 0->20.0, 50->40.0, 100->80.0
        // Too simple quadratic fits s.t. for  x_scale: 0->10.0, 50->20.0, 100->40.0
        double x_scale = (sens * sens)/250.0 + (sens)/5.0  + 20.0;
        double y_scale = (sens * sens)/500.0 + (sens)/10.0 + 10.0;

        if( LNDactive ) // was active on last pass
        {
            qDebug() << "Transition back to non LND mode. sensitivity =" << sens;

            // Transition back to orientation mode, but with the cursor at the same point
            cursorOffsetX = mousePosition.x() - (DEG_FROM_RAD(horizRot))*(screenInPixels.width()/x_scale);
            cursorOffsetY = mousePosition.y() - (DEG_FROM_RAD(vertRot))*(screenInPixels.height()/y_scale);

            qDebug() << "new cursorOffset:" << cursorOffsetX << cursorOffsetY;

            LNDactive = false;
        }

        // Filter this a bit
        double newMouseX = cursorOffsetX+(DEG_FROM_RAD(horizRot))*(screenInPixels.width()/x_scale);
        double newMouseY = cursorOffsetY+(DEG_FROM_RAD(vertRot))*(screenInPixels.height()/y_scale);

        mousePosition.setX( mousePosition.rx()*0.7 + newMouseX*0.3 );
        mousePosition.setY( mousePosition.ry()*0.7 + newMouseY*0.3 );

        // And normalise the 2D coordinates to the screen area (drag box)
        if( mousePosition.x()<0.0 )
        {
            cursorOffsetX -= mousePosition.x();
            mousePosition.setX(0.0);
        }
        else  if( mousePosition.x()>screenInPixels.width() )
        {
            cursorOffsetX -= (mousePosition.x() - screenInPixels.width());
            mousePosition.setX(screenInPixels.width());
        }

        if( mousePosition.y()<0.0 )
        {
            cursorOffsetY -= mousePosition.y();
            mousePosition.setY(0.0);
        }
        else if( mousePosition.y()>screenInPixels.height() )
        {
            cursorOffsetY -= (mousePosition.y() - screenInPixels.height());
            mousePosition.setY(screenInPixels.height());
        }

        // Filter the update (again). TODO: why twice?
        outputState->screenX = (mousePosition.x()*9 + outputState->screenX)/10;
        outputState->screenY = (mousePosition.y()*9 + outputState->screenY)/10;
    }
}

void pen_location::getCurrent3Dpos(location_3D_t *loc)
{
    return;
}

void pen_location::getCurrent2Dpos(QPoint *pos)
{
    pos->setX(mousePosition.x());
    pos->setY(mousePosition.y());

    return;
}

