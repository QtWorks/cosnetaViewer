
#include "build_options.h"

#include <QString>
#include <QTime>

#include <QDebug>

// Global timer to ensure rate calculations work on slow machines.
extern QTime globalTimer;

#ifdef __GNUC__
#include <math.h>
#endif

#include "main.h"
#include "freerunner_low_level.h"
#include "gestures.h"

// Accessible from the GUI - the flags override the device set state
QString gestureDebugString        = "Unset";

// The angle rates used here are scaled to rad/s after filtering.
static double roll_r_threashold  = 0.2;
static double pitch_r_threashold = 0.2;
static double yaw_r_threashold   = 0.2;


// Use native keys for initial table as Qt doesn't provide an easy way to translate.
// Therefore the gesture editor will grab native key codes (not Qt::Key ones).

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#define RIGHT_ARROW VK_RIGHT
#define LEFT_ARROW  VK_LEFT
#define UP_ARROW    VK_UP
#define DOWN_ARROW  VK_DOWN
#define MINUS_KEY   VK_OEM_MINUS
#define PLUS_KEY    VK_OEM_PLUS

#elif defined( __gnu_linux__ )

#define RIGHT_ARROW 0
#define LEFT_ARROW  0
#define UP_ARROW    0
#define DOWN_ARROW  0
#define MINUS_KEY   0
#define PLUS_KEY    0

#elif defined(__APPLE__) && defined(__MACH__)

#define RIGHT_ARROW 0
#define LEFT_ARROW  0
#define UP_ARROW    0
#define DOWN_ARROW  0
#define MINUS_KEY   0
#define PLUS_KEY    0

#endif


#ifdef DEBUG_TAB

#include <QFile>
#include <QDir>
#include <QTextStream>

static bool        dumpGesturesToFile  = false;
static bool        stopDumpingGestures = false;
static QFile       gesturesTextFile;
static QTextStream gesturesTextOut;

void stopGestureSaving(void)
{
    stopDumpingGestures = true;
}

bool startGestureSaving(void)
{
    // Select a directory
    QDir dir;
    dir.setCurrent(QDir::homePath());

    // Get a common suffix

    // Make sure that the file classes are not in use

    if( gesturesTextFile.isOpen() ) gesturesTextFile.close();

    // Create and open the files
    gesturesTextFile.setFileName("gesture_trace.txt");

    if( ! gesturesTextFile.open(QIODevice::WriteOnly | QIODevice::Text) ) return false;

    gesturesTextOut.setDevice(&gesturesTextFile);
    gesturesTextOut << "Gesture Degbug\n==============\n\n";

    // And set the flags
    stopDumpingGestures = false;
    dumpGesturesToFile  = true;

    return true;
}

#endif


typedef struct
{
    int                    IDNum;
    low_level_rot_state_t  action[PREV_GESTURE_STATES_STORED];
    double                 minDuration[PREV_GESTURE_STATES_STORED];		// Seconds
    double                 maxDuration[PREV_GESTURE_STATES_STORED];		// Seconds
    int                    numReq;
    int                    keycode;
    bool                   sendCtrl;
    bool                   sendShift;
    bool                   toggleMouseDrive;
    QString                name;

} actionListDescription_t;

#define MAX_ACTIONS 100

static int                     action_list_size = 28;
static actionListDescription_t actions[MAX_ACTIONS] =
{
    {
        1,
        {yawing_left, yawing_right, no_action },    // Last action is first in list
        {       0.10,         0.10,      0.50 },	// Min (secs)
        {       5.00,         5.00,   9999.00 },	// Max (secs)
        3,
        RIGHT_ARROW, false, false,
        false,
        "Flick right 1."
    },
    {
        1,
        {yawing_left, no_action, yawing_right, no_action }, // Last action is first in list
        {       0.10,      0.00,         0.10,      0.50 },	// Min (secs)
        {       5.00,      0.30,         5.00,   9999.00 },	// Max (secs)
        4,
        RIGHT_ARROW, false, false,
        false,
        "Flick right 2."
    },
    {
        2,
        {yawing_right,  yawing_left, no_action },   // Last action is first in list
        {        0.10,         0.10,      0.50 },	// Min (secs)
        {        5.00,         5.00,   9999.00 },	// Max (secs)
        3,
        LEFT_ARROW, false, false,
        false,
        "Flick left 1."
    },
    {
        2,
        {yawing_right, no_action,  yawing_left, no_action },    // Last action is first in list
        {        0.10,      0.00,         0.10,      0.50 },	// Min (secs)
        {        5.00,      0.30,         5.00,   9999.00 },	// Max (secs)
        4,
        LEFT_ARROW, false, false,
        false,
        "Flick left 2."
    },
    {
        3,
        {pitching_down, pitching_up, no_action },   // Last action is first in list
        {         0.10,        0.10,      0.50 },	// Min (secs)
        {         5.00,        5.00,   9999.00 },	// Max (secs)
        3,
        DOWN_ARROW, false, false,
        false,
        "Flick down 1."
    },
    {
        3,
        {pitching_down, no_action, pitching_up, no_action },    // Last action is first in list
        {         0.10,      0.00,        0.10,      0.50 },	// Min (secs)
        {         5.00,      0.30,        5.00,   9999.00 },	// Max (secs)
        4,
        DOWN_ARROW, false, false,
        false,
        "Flick down 2."
    },
    {
        4,
        {pitching_up, pitching_down, no_action },   // Last action is first in list
        {       0.10,          0.10,      0.50 },	// Min (secs)
        {       5.00,          5.00,   9999.00 },	// Max (secs)
        3,
        UP_ARROW, false, false,
        false,
        "Flick up 1."
    },
    {
        4,
        {pitching_up, no_action, pitching_down, no_action },    // Last action is first in list
        {       0.10,      0.00,          0.10,      0.50 },	// Min (secs)
        {       5.00,      0.30,          5.00,   9999.00 },	// Max (secs)
        4,
        UP_ARROW, false, false,
        false,
        "Flick up 2."
    },
    {
        5,
        { rolling_anticlockwise, rolling_clockwise, rolling_anticlockwise, rolling_clockwise, no_action }, // Last action is first in list
        {                  0.01,              0.01,                  0.01,              0.01,      0.01 },	// Min (secs)
        {                  5.00,              5.00,                  5.00,              5.00,   9999.00 },	// Max (secs)
        5,
        0, false, false,
        false,
        "Waggle 1a - toggle mouse mode."
    },
    {
        5,
        { rolling_anticlockwise, no_action, rolling_clockwise, no_action, rolling_anticlockwise, no_action, rolling_clockwise, no_action }, // Last action is first in list
        {                  0.01,      0.01,              0.01,      0.01,                  0.01,      0.01,              0.01,      0.01 },	// Min (secs)
        {                  5.00,      1.00,              5.00,      1.00,                  5.00,      1.00,              5.00,   9999.00 },	// Max (secs)
        8,
        0, false, false,
        false,
        "Waggle 1b - toggle mouse mode."
    },
    {
        5,
        { rolling_clockwise, rolling_anticlockwise, rolling_clockwise, rolling_anticlockwise, no_action }, // Last action is first in list
        {              0.01,                  0.01,              0.01,                  0.01,      0.01 },	// Min (secs)
        {              5.00,                  5.00,              5.00,                  5.00,   9999.00 },	// Max (secs)
        5,
        0, false, false,
        false,
        "Waggle 2a - toggle mouse mode."
    },
    {
        5,
        { rolling_clockwise, no_action, rolling_anticlockwise, no_action, rolling_clockwise, no_action, rolling_anticlockwise, no_action }, // Last action is first in list
        {              0.01,      0.01,                  0.01,      0.01,              0.01,      0.01,                  0.01,      0.01 },	// Min (secs)
        {              5.00,      1.00,                  5.00,      1.00,              5.00,      1.00,                  5.00,   9999.00 },	// Max (secs)
        8,
        0, false, false,
        false,
        "Waggle 2b - toggle mouse mode."
    },
    {
        6,
        {pitching_up, yawing_right, pitching_down, yawing_left,   pitching_up, no_action }, // Last action is first in list
        {       0.05,         0.05,          0.05,        0.05,          0.05,      0.05 },	// Min (secs)
        {       5.00,         5.00,          5.00,        5.00,          5.00,      2.00 },	// Max (secs)
        6,
        MINUS_KEY, true, false,
        false,
        "Rot anti-clockwise 1."
    },
    {
        6,
        { yawing_right, pitching_down, yawing_left,   pitching_up, yawing_right, no_action}, // Last action is first in list
        {         0.05,          0.05,        0.05,          0.05,         0.05,      0.05},	// Min (secs)
        {         5.00,          5.00,        5.00,          5.00,         5.00,   9999.00},	// Max (secs)
        6,
        MINUS_KEY, true, false,
        false,
        "Rot anti-clockwise 2."
    },
    {
        6,
        { pitching_down,  yawing_left,  pitching_up, yawing_right,  pitching_down,  no_action }, // Last action is first in list
        {          0.05,         0.05,         0.05,         0.05,        0.05,          0.05 },	// Min (secs)
        {          5.00,         5.00,         5.00,         5.00,        5.00,       9999.00 },	// Max (secs)
        6,
        MINUS_KEY, true, false,
        false,
        "Rot anti-clockwise 3."
    },
    {
        6,
        { yawing_left,  pitching_up, yawing_right, pitching_down,  yawing_left,  no_action }, // Last action is first in list
        {        0.05,         0.05,         0.05,          0.05,         0.05,       0.05 },	// Min (secs)
        {        5.00,         5.00,         5.00,          5.00,         5.00,    9999.00 },	// Max (secs)
        6,
        MINUS_KEY, true, false,
        false,
        "Rot anti-clockwise 4."
    },
    {
        7,
        {pitching_up, yawing_left, pitching_down, yawing_right,   pitching_down, no_action }, // Last action is first in list
        {       0.05,         0.05,          0.05,        0.05,          0.05,      0.05 },	// Min (secs)
        {       5.00,         5.00,          5.00,        5.00,          5.00,      2.00 },	// Max (secs)
        6,
        PLUS_KEY, true, false,
        false,
        "Rot clockwise 1."
    },
    {
        7,
        { yawing_left, pitching_down, yawing_right,   pitching_up, yawing_left, no_action}, // Last action is first in list
        {         0.05,          0.05,        0.05,          0.05,         0.05,      0.05},	// Min (secs)
        {         5.00,          5.00,        5.00,          5.00,         5.00,   9999.00},	// Max (secs)
        6,
        PLUS_KEY, true, false,
        false,
        "Rot clockwise 2."
    },
    {
        7,
        { pitching_down,  yawing_right,  pitching_up, yawing_left,  pitching_down,  no_action }, // Last action is first in list
        {          0.05,         0.05,         0.05,         0.05,        0.05,          0.05 },	// Min (secs)
        {          5.00,         5.00,         5.00,         5.00,        5.00,       9999.00 },	// Max (secs)
        6,
        PLUS_KEY, true, false,
        false,
        "Rot clockwise 3."
    },
    {
        7,
        { yawing_right,  pitching_up, yawing_left, pitching_down,  yawing_right,  no_action }, // Last action is first in list
        {        0.05,         0.05,         0.05,          0.05,         0.05,       0.05 },	// Min (secs)
        {        5.00,         5.00,         5.00,          5.00,         5.00,    9999.00 },	// Max (secs)
        6,
        PLUS_KEY, true, false,
        false,
        "Rot clockwise 4."
    },
    { // Last action is first in list
        8,
        {  yawing_right,   pitching_up,   yawing_left,   pitching_up,  yawing_right, pitching_down,   yawing_left, pitching_down,  yawing_right,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 1."
    },
    { // Last action is first in list
        8,
        {   pitching_up,   yawing_left,   pitching_up,  yawing_right, pitching_down,   yawing_left, pitching_down,  yawing_right,   pitching_up,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 2."
    },
    { // Last action is first in list
        8,
        {   yawing_left,   pitching_up,  yawing_right, pitching_down,   yawing_left, pitching_down,  yawing_right,   pitching_up,   yawing_left,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 3."
    },
    { // Last action is first in list
        8,
        {   pitching_up,  yawing_right, pitching_down,   yawing_left, pitching_down,  yawing_right,   pitching_up,   yawing_left,   pitching_up,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 4."
    },
    { // Last action is first in list
        8,
        {  yawing_right, pitching_down,   yawing_left, pitching_down,  yawing_right,   pitching_up,   yawing_left,   pitching_up,  yawing_right,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 5."
    },
    { // Last action is first in list
        8,
        { pitching_down,   yawing_left, pitching_down,  yawing_right,   pitching_up,   yawing_left,   pitching_up,  yawing_right, pitching_down,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 6."
    },
    { // Last action is first in list
        8,
        {   yawing_left, pitching_down,  yawing_right,   pitching_up,   yawing_left,   pitching_up,  yawing_right, pitching_down,   yawing_left,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        true,
        "Fig 8a clockwise 7."
    },
    { // Last action is first in list
        8,
        { pitching_down,  yawing_right,   pitching_up,   yawing_left,   pitching_up,  yawing_right, pitching_down,   yawing_left, pitching_down,     no_action },
        {          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05,          0.05 },	// Min (secs)
        {          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,          5.00,       9999.00 },	// Max (secs)
        10,
        0, false, false,
        false,
        "Fig 8a clockwise 8."
    }
};


#include "gestures.h"

gestures::gestures()
{
//    penActionMode = NORMAL_OVERLAY;
    penActionMode = DRIVE_MOUSE;

    for(int i=0; i<3; i++)
        filteredGyro[i] = 0.0;

    for(int i=0; i<PREV_GESTURE_STATES_STORED; i++)
    {
        actionList[i].action   = no_action;
        actionList[i].duration = 0.0;
    }

    lastPenButtons             = 0;

    lastMouseModeButtonDown    = false;
    gesturePending             = false;
    lastGestureIndex           = 0;
    presentationActionPending  = false;

    lastTimeMS = globalTimer.elapsed();
}

gestures::~gestures()
{
}

low_level_rot_state_t gestures::currentAngleRateDirection(double angleRate[3])
{
    low_level_rot_state_t currentAction = no_action;
    double                maxYawPitchR;

    double rollRMag  = fabs(angleRate[0]) / 3.0;
    double pitchRMag = fabs(angleRate[1]);
    double yawRMag   = fabs(angleRate[2]);

    // Get a current action.. basically get the biggest rotation rate, and base it on that for now.
    if( rollRMag < roll_r_threashold  && pitchRMag < pitch_r_threashold && yawRMag < yaw_r_threashold )
    {
        currentAction = no_action;
    }
    else
    {
        // Apply a heuristic (take the biggest or roll or yaw&pitch) rates as state.
        maxYawPitchR = ( pitchRMag > yawRMag ) ? pitchRMag : yawRMag;

        if( rollRMag > maxYawPitchR )
        {
            currentAction = ( angleRate[0] > 0.0 ) ? rolling_clockwise : rolling_anticlockwise;
        }
        else if( pitchRMag > yawRMag )
        {
            currentAction = ( angleRate[1] > 0.0 ) ? pitching_up : pitching_down;
        }
        else
        {
            currentAction = ( angleRate[2] > 0.0 ) ? yawing_right : yawing_left;
        }
    }

    return currentAction;
}



// Based on a supplied list of actions, do a lookup

bool gestures::findGesture(action_low_level_t currentActions[])
{
    int  gestNum;
    int  tableActionStep;
    int  currentStep;
    bool found = false;

    for( gestNum=0; gestNum<action_list_size && ! found; gestNum ++ )
    {
        found = true;

        // Loop through each element in this gesture
        for(tableActionStep=0, currentStep=0;
            found && currentStep<actions[gestNum].numReq && currentStep<PREV_GESTURE_STATES_STORED;
            tableActionStep ++)
        {
            if( actions[gestNum].action[tableActionStep]      == currentActions[currentStep].action   &&
                actions[gestNum].maxDuration[tableActionStep] >  currentActions[currentStep].duration &&
                actions[gestNum].minDuration[tableActionStep] <  currentActions[currentStep].duration   )
            {
                // Matched, so we move the current check on by one
                currentStep ++;
            }
            else // Didn't match
            {
                found = false;
            }
        }
    }

    // If we stopped because the entry was found

    if( found )
    {
        lastGestureIndex = gestNum;
        gesturePending   = true;
    }

    return found;
}


void gestures::update(cosneta_sample_t *sample)
{
    low_level_rot_state_t  currentAction;
    static bool            clearOnNextChange = false;

    // Filter
    filteredGyro[0] = 0.995*filteredGyro[0] + 0.005*(sample->angle_rate[0]);
    filteredGyro[1] = 0.995*filteredGyro[1] + 0.005*(sample->angle_rate[1]);
    filteredGyro[2] = 0.995*filteredGyro[2] + 0.005*(sample->angle_rate[2]);

    // Look for a basic direction
    currentAction = currentAngleRateDirection(filteredGyro);

    // Did it change from the last?
    if( currentAction != actionList[0].action )
    {
#ifdef DEBUG_TAB

        if( dumpGesturesToFile )
        {
            gesturesTextOut.setRealNumberPrecision(4);
            gesturesTextOut.setRealNumberNotation(QTextStream::FixedNotation);

            // Dump data to file. Handle #1 differently as no duration yet.

            gesturesTextOut << ACTION_STR(actionList[0].action) << "[";
            int last;
            for(last=PREV_GESTURE_STATES_STORED-1;
                last>=0 && actionList[last].action == no_action;
                last --);
            for(int i=1; i<=last; i++)
            {
                gesturesTextOut << ACTION_STR(actionList[i].action) << " ";
                gesturesTextOut << actionList[i].duration << ";";
                if( i<last ) gesturesTextOut << "\n                      ";
            }
            gesturesTextOut << "]\n";       // NB endl writes a newline AND flushes the stream

            if( stopDumpingGestures )
            {
                gesturesTextOut.flush();
                gesturesTextFile.close();

                dumpGesturesToFile = false;
            }
        }
#endif
        // Assume that we just have vibrartion if it lasts for 1/20 sec or less
        // If jitter, then just overwrite the top of the list...
        if( actionList[0].duration > 0.05 )
        {
            if( clearOnNextChange )
            {
                for(int i=1; i < PREV_GESTURE_STATES_STORED; i++)
                {
                    actionList[i].action   = no_action;
                    actionList[i].duration = 0.0;
                }
                clearOnNextChange = false;
            }
            else
            {
                // Shift the table up
                for(int i=(PREV_GESTURE_STATES_STORED-1); i>0; i--)
                {
                    actionList[i].action   = actionList[i-1].action;
                    actionList[i].duration = actionList[i-1].duration;
                }
            }
        }
#ifdef DEBUG_TAB
        else    // actionList[0].duration is less than 1/20 sec
        {
            gesturesTextOut << "Skip getsure " << ACTION_STR(actionList[0].action);
            gesturesTextOut << " as duration was " << actionList[0].duration << "\n";
        }
#endif
        // Start a new action
        actionList[0].action   = currentAction;
        actionList[0].duration = sample->timeStep_angle_rate;
    }
    else
    {
        actionList[0].duration += sample->timeStep_angle_rate;	// the duration of the current action so far...
    }

#ifdef DEBUG_TAB
    static int skip = 0;        // Not that CPU use is an issue yet, but rather just on principle.
    if( skip++ > 10 )
    {
        QString gestureDebugStringNew;  // Rather than a lock, just reduce the chances of the GUI accessing during update...

        gestureDebugStringNew  = "0: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[0].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[0].duration, 'f', 7);
        gestureDebugStringNew += "\n1: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[1].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[1].duration, 'f', 7);
        gestureDebugStringNew += "\n2: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[2].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[2].duration, 'f', 7);
        gestureDebugStringNew += "\n3: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[3].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[3].duration, 'f', 7);
        gestureDebugStringNew += "\n4: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[4].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[4].duration, 'f', 7);
        gestureDebugStringNew += "\nFiltered Gyro: [" + QString::number(filteredGyro[0], 'f', 7);
        gestureDebugStringNew += ", " + QString::number(filteredGyro[1], 'f', 7);
        gestureDebugStringNew += ", " + QString::number(filteredGyro[2], 'f', 7);
        gestureDebugStringNew += "]\n";
        gestureDebugStringNew += "Raw gyro: [" +  QString::number((sample->angle_rate[0]));
        gestureDebugStringNew += ", " +  QString::number((sample->angle_rate[1]));
        gestureDebugStringNew += ", " +  QString::number((sample->angle_rate[2]));
        gestureDebugStringNew += "]\n";

        gestureDebugString = gestureDebugStringNew;

        skip = 0;
    }
#endif

    // Now look for action occurrances (saving the table index in a global)

    if( ! clearOnNextChange && findGesture(actionList) )
    {
#ifdef DEBUG_TAB
        if( dumpGesturesToFile )
        {
            gesturesTextOut << "Found: " << actions[lastGestureIndex].name << "\n======================\n";
        }
#endif
        // Was this a gesture to toggle mouse to gesture mode
        if( actions[lastGestureIndex].toggleMouseDrive )
        {
            switch(penActionMode)
            {
            case NORMAL_OVERLAY:
                penActionMode = DRIVE_MOUSE;
                balloonMessage("Drive mouse from FreeRunner gesture.");
                break;

            case DRIVE_MOUSE:
                penActionMode = PRESENTATION_CONTROLLER;
                balloonMessage("Presentation mode from FreeRunner gesture.");
                break;

            case PRESENTATION_CONTROLLER:
                penActionMode = NORMAL_OVERLAY;
                balloonMessage("Normal overlay mode from FreeRunner gesture.");
                break;
            }

            // And apply settings overrides
//            if( ! freerunnerPointerDriveOn && driveMouse ) driveMouse = false;

            gesturePending = false;
        }
#if 1
        else
        {
            balloonMessage(actions[lastGestureIndex].name);
        }
#endif
        // Clear the list so that it is only found once

        clearOnNextChange = true; // for(int i=0; i<PREV_STATES_STORED; i++) actionList[i].action = no_action;
    }

    // Note that this per pen. We will manage which pen actually does it in appComs
    if( BUTTON_MODE_SWITCH == (BUTTON_MODE_SWITCH & sample->buttons) )
    {
        if( ! lastMouseModeButtonDown )
        {
            switch(penActionMode)
            {
            case NORMAL_OVERLAY:
                penActionMode = DRIVE_MOUSE;
                balloonMessage("Drive mouse from FreeRunner button.");
                break;

            case DRIVE_MOUSE:
                penActionMode = PRESENTATION_CONTROLLER;
                balloonMessage("Presentation mode from FreeRunner button.");
                break;

            case PRESENTATION_CONTROLLER:
                penActionMode = NORMAL_OVERLAY;
                balloonMessage("Normal overlay mode from FreeRunner button.");
                break;
            }
        }
        lastMouseModeButtonDown = true;
    }
    else
    {
        lastMouseModeButtonDown = false;
    }

    return;
}

void gestures::update(invensense_sample_t *sample)
{
    ;
}

void gestures::updateBasedOnButtons(quint32 newButtons)
{
    // Note that this per pen. We will manage which pen actually does it in appComs

    if( BUTTON_MODE_SWITCH == (BUTTON_MODE_SWITCH & newButtons) )
    {
        // Has the button just been pressed (a leading edge)
        if( BUTTON_MODE_SWITCH != (BUTTON_MODE_SWITCH & lastPenButtons) )
        {
            switch(penActionMode)
            {
            case NORMAL_OVERLAY:
                penActionMode = DRIVE_MOUSE;
                balloonMessage("Drive mouse from remote connection.");
                break;

            case DRIVE_MOUSE:
                penActionMode = PRESENTATION_CONTROLLER;
                balloonMessage("Presentation mode for remote connection.");
                break;

            case PRESENTATION_CONTROLLER:
                penActionMode = NORMAL_OVERLAY;
                balloonMessage("Normal overlay mode from remote connection.");
                break;
            }
        }
    }

    // For each mode, generate any required commands (leading edges on buttons)

    switch( penActionMode )
    {
    case PRESENTATION_CONTROLLER:

        if( (newButtons     & BUTTON_MIDDLE_MOUSE) == BUTTON_MIDDLE_MOUSE &&
            (lastPenButtons & BUTTON_MIDDLE_MOUSE) != BUTTON_MIDDLE_MOUSE   )
//        if( (newButtons     & BUTTON_LEFT_MOUSE) == BUTTON_LEFT_MOUSE &&
//            (lastPenButtons & BUTTON_LEFT_MOUSE) != BUTTON_LEFT_MOUSE   )
        {
            presentationAction        = APP_CTRL_EXIT_PRESENTATION;
            presentationActionPending = true;
        }
        else if( (newButtons     & BUTTON_RIGHT_MOUSE) == BUTTON_RIGHT_MOUSE &&
                 (lastPenButtons & BUTTON_RIGHT_MOUSE) != BUTTON_RIGHT_MOUSE   )
        {
            presentationAction        = APP_CTRL_START_PRESENTATION;
            presentationActionPending = true;
        }
        else if( (newButtons     & BUTTON_SCROLL_UP) == BUTTON_SCROLL_UP &&
                 (lastPenButtons & BUTTON_SCROLL_UP) != BUTTON_SCROLL_UP   )
        {
            presentationAction        = APP_CTRL_PREV_PAGE;
            presentationActionPending = true;
        }
        else if( (newButtons     & BUTTON_SCROLL_DOWN) == BUTTON_SCROLL_DOWN &&
                 (lastPenButtons & BUTTON_SCROLL_DOWN) != BUTTON_SCROLL_DOWN   )
        {
            presentationAction        = APP_CTRL_NEXT_PAGE;
            presentationActionPending = true;
        }
        else if( (newButtons     & BUTTON_OPT_A) == BUTTON_OPT_A &&
                 (lastPenButtons & BUTTON_OPT_A) != BUTTON_OPT_A   )
        {
            presentationAction        = APP_CTRL_START_SHORTCUT;
            presentationActionPending = true;
        }
        // NB Don't clear pending here, it's done on the readout, and that's
        // the callers responsibility.
        break;

    default:
        break;
    }

    lastPenButtons = newButtons;

    return;
}


void gestures::update(net_message_t *sample)
{
    updateBasedOnButtons(sample->msg.data.buttons);

    return;
}

// Interpret wireless pen data (from combined USB packet)
void gestures::update(pen_state_t *data)
{
    // Use angle rates from pen_location calculations
    low_level_rot_state_t  currentAction;
    static bool            clearOnNextChange = false;

    // Filter
    filteredGyro[0] = 0.7*filteredGyro[0] + 0.3*(data->rot_rate[0]);
    filteredGyro[1] = 0.7*filteredGyro[1] + 0.3*(data->rot_rate[1]);
    filteredGyro[2] = 0.7*filteredGyro[2] + 0.3*(data->rot_rate[2]);

    // Look for a basic direction
    currentAction = currentAngleRateDirection(filteredGyro);

    // Did it change from the last?
    if( currentAction != actionList[0].action )
    {
#ifdef DEBUG_TAB

        if( dumpGesturesToFile )
        {
            gesturesTextOut.setRealNumberPrecision(4);
            gesturesTextOut.setRealNumberNotation(QTextStream::FixedNotation);

            // Dump data to file. Handle #1 differently as no duration yet.

            gesturesTextOut << ACTION_STR(actionList[0].action) << "[";
            int last;
            for(last=PREV_GESTURE_STATES_STORED-1;
                last>=0 && actionList[last].action == no_action;
                last --);
            for(int i=1; i<=last; i++)
            {
                gesturesTextOut << ACTION_STR(actionList[i].action) << " ";
                gesturesTextOut << actionList[i].duration << ";";
                if( i<last ) gesturesTextOut << "\n                      ";
            }
            gesturesTextOut << "]\n";       // NB endl writes a newline AND flushes the stream

            if( stopDumpingGestures )
            {
                gesturesTextOut.flush();
                gesturesTextFile.close();

                dumpGesturesToFile = false;
            }
        }
#endif
        // Assume that we just have vibrartion if it lasts for 1/20 sec or less
        // If jitter, then just overwrite the top of the list...
        if( actionList[0].duration > 0.05 )
        {
            if( clearOnNextChange )
            {
                for(int i=1; i < PREV_GESTURE_STATES_STORED; i++)
                {
                    actionList[i].action   = no_action;
                    actionList[i].duration = 0.0;
                }
                clearOnNextChange = false;
            }
            else
            {
                // Shift the table up
                for(int i=(PREV_GESTURE_STATES_STORED-1); i>0; i--)
                {
                    actionList[i].action   = actionList[i-1].action;
                    actionList[i].duration = actionList[i-1].duration;
                }
            }
        }
#ifdef DEBUG_TAB
        else    // actionList[0].duration is less than 1/20 sec
        {
            gesturesTextOut << "Skip getsure " << ACTION_STR(actionList[0].action);
            gesturesTextOut << " as duration was " << actionList[0].duration << "\n";
        }
#endif
        // Start a new action
        actionList[0].action   = currentAction;
        actionList[0].duration = 0.001 * ( globalTimer.elapsed() - lastTimeMS );
    }
    else
    {
        actionList[0].duration += 0.001 * ( globalTimer.elapsed() - lastTimeMS );	// the duration of the current action so far...
    }

    lastTimeMS = globalTimer.elapsed();

#ifdef DEBUG_TAB
    static int skip = 0;        // Not that CPU use is an issue yet, but rather just on principle.
    if( skip++ > 10 )
    {
        QString gestureDebugStringNew;  // Rather than a lock, just reduce the chances of the GUI accessing during update...

        gestureDebugStringNew  = "0: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[0].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[0].duration, 'f', 7);
        gestureDebugStringNew += "\n1: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[1].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[1].duration, 'f', 7);
        gestureDebugStringNew += "\n2: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[2].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[2].duration, 'f', 7);
        gestureDebugStringNew += "\n3: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[3].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[3].duration, 'f', 7);
        gestureDebugStringNew += "\n4: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[4].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[4].duration, 'f', 7);
        gestureDebugStringNew += "\nFiltered Gyro: [" + QString::number(filteredGyro[0], 'f', 7);
        gestureDebugStringNew += ", " + QString::number(filteredGyro[1], 'f', 7);
        gestureDebugStringNew += ", " + QString::number(filteredGyro[2], 'f', 7);
        gestureDebugStringNew += "]\n";
        gestureDebugStringNew += "Raw gyro: [" +  QString::number((data->rot_rate[0]));
        gestureDebugStringNew += ", " +  QString::number((data->rot_rate[1]));
        gestureDebugStringNew += ", " +  QString::number((data->rot_rate[2]));
        gestureDebugStringNew += "]\n";

        gestureDebugString = gestureDebugStringNew;

        skip = 0;
    }
#endif

    // Now look for action occurrances (saving the table index in a global)

    if( ! clearOnNextChange && findGesture(actionList) )
    {
#ifdef DEBUG_TAB
        if( dumpGesturesToFile )
        {
            gesturesTextOut << "Found: " << actions[lastGestureIndex].name << "\n======================\n";
        }
#endif
        // Was this a gesture to toggle mouse to gesture mode
        if( actions[lastGestureIndex].toggleMouseDrive )
        {
            switch(penActionMode)
            {
            case NORMAL_OVERLAY:
                penActionMode = DRIVE_MOUSE;
                balloonMessage("Drive mouse from FreeRunner gesture.");
                break;

            case DRIVE_MOUSE:
                penActionMode = PRESENTATION_CONTROLLER;
                balloonMessage("Presentation mode from FreeRunner gesture.");
                break;

            case PRESENTATION_CONTROLLER:
                penActionMode = NORMAL_OVERLAY;
                balloonMessage("Normal overlay mode from FreeRunner gesture.");
                break;
            }

            // And apply settings overrides
//          if( ! freerunnerPointerDriveOn && driveMouse ) driveMouse = false;

            gesturePending = false;
        }
#if 1
        else
        {
            balloonMessage(actions[lastGestureIndex].name);
        }
#endif
        // Clear the list so that it is only found once

        clearOnNextChange = true; // for(int i=0; i<PREV_STATES_STORED; i++) actionList[i].action = no_action;
    }

    // Generate events from button presses
    updateBasedOnButtons(data->buttons);

    return;
}


enum pen_modes_e gestures::mouseModeSelected(void)
{
    return penActionMode;
}

void gestures::forceCurrentControllerMode(enum pen_modes_e newMode)
{
    switch(newMode )
    {
    case NORMAL_OVERLAY:
    case DRIVE_MOUSE:
    case PRESENTATION_CONTROLLER:

        penActionMode = newMode;
        break;

    default:
        qDebug() << "forceCurrentControllerMode: Internal error, received:" << newMode;
    }
}

bool gestures::gestureGetKeypress(int *keypress, bool *shift, bool *control)
{
    // If we have a pending gesture event, return the keycode for it

    if( gesturePending )
    {
        // in table, Key_Unknown indicates no keypress.
        if( actions[lastGestureIndex].keycode )
        {
            *keypress = actions[lastGestureIndex].keycode;
            *shift    = actions[lastGestureIndex].sendShift;
            *control  = actions[lastGestureIndex].sendCtrl;
        }

        gesturePending = false;

        return freerunnerGesturesDriveOn; // That is, only return true of the gestures are enabled
    }
    else
    {
        return false;
    }
}

bool gestures::getPresentationAction(quint32 *action)
{
    if( ! presentationActionPending )
    {
        return false;
    }
    else
    {
        *action                   = presentationAction;
        presentationActionPending = false;

        return true;
    }
}





#if 0
low_level_rot_state_t currentAngleRateDirection(double angleRate[3])
{
    low_level_rot_state_t currentAction = no_action;
    double                maxYawPitchR;

    double rollRMag  = fabs(angleRate[0]) / 3.0;
    double pitchRMag = fabs(angleRate[1]);
    double yawRMag   = fabs(angleRate[2]);

    // Get a current action.. basically get the biggest rotation rate, and base it on that for now.
    if( rollRMag < roll_r_threashold  && pitchRMag < pitch_r_threashold && yawRMag < yaw_r_threashold )
    {
        currentAction = no_action;
    }
    else
    {
        // Apply a heuristic (take the biggest or roll or yaw&pitch) rates as state.
        maxYawPitchR = ( pitchRMag > yawRMag ) ? pitchRMag : yawRMag;

        if( rollRMag > maxYawPitchR )
        {
            currentAction = ( angleRate[0] > 0.0 ) ? rolling_clockwise : rolling_anticlockwise;
        }
        else if( pitchRMag > yawRMag )
        {
            currentAction = ( angleRate[1] > 0.0 ) ? pitching_up : pitching_down;
        }
        else
        {
            currentAction = ( angleRate[2] > 0.0 ) ? yawing_right : yawing_left;
        }
    }

    return currentAction;
}



// Based on a supplied list of actions, do a lookup

static bool findGesture(action_low_level_t currentActions[])
{
    int  gestNum;
    int  tableActionStep;
    int  currentStep;
    bool found = false;

    for( gestNum=0; gestNum<action_list_size && ! found; gestNum ++ )
    {
        found = true;

        // Loop through each element in this gesture
        for(tableActionStep=0, currentStep=0;
            found && currentStep<actions[gestNum].numReq && currentStep<PREV_STATES_STORED;
            tableActionStep ++)
        {
            if( actions[gestNum].action[tableActionStep]      == currentActions[currentStep].action   &&
                actions[gestNum].maxDuration[tableActionStep] >  currentActions[currentStep].duration &&
                actions[gestNum].minDuration[tableActionStep] <  currentActions[currentStep].duration   )
            {
                // Matched, so we move the current check on by one
                currentStep ++;
            }
            else // Didn't match
            {
                found = false;
            }
        }
    }

    // If we stopped because the entry was found

    if( found )
    {
        lastGestureIndex = gestNum;
        gesturePending   = true;
    }

    return found;
}


// Moved from variables local to gestureInterpret() to allow white box testing.
static double             filteredGyro[3]                = { 0.0, 0.0, 0.0 };
static action_low_level_t actionList[PREV_STATES_STORED] = {{no_action, 0.0 },{no_action, 0.0 },{no_action, 0.0 }};

void gestureInterpret( cosneta_sample_t *sample )
{
    low_level_rot_state_t  currentAction;
    static bool            clearOnNextChange = false;

    // Filter
    filteredGyro[0] = 0.995*filteredGyro[0] + 0.005*COSNETA_GYRO_RATE_G1(sample->angle_rate[0]);
    filteredGyro[1] = 0.995*filteredGyro[1] + 0.005*COSNETA_GYRO_RATE_G2(sample->angle_rate[1]);
    filteredGyro[2] = 0.995*filteredGyro[2] + 0.005*COSNETA_GYRO_RATE_G3(sample->angle_rate[2]);

    // Look for a basic direction
    currentAction = currentAngleRateDirection(filteredGyro);

    // Did it change from the last?
    if( currentAction != actionList[0].action )
    {
#ifdef DEBUG_TAB

        if( dumpGesturesToFile )
        {
            gesturesTextOut.setRealNumberPrecision(4);
            gesturesTextOut.setRealNumberNotation(QTextStream::FixedNotation);

            // Dump data to file. Handle #1 differently as no duration yet.

            gesturesTextOut << ACTION_STR(actionList[0].action) << "[";
            int last;
            for(last=PREV_STATES_STORED-1;
                last>=0 && actionList[last].action == no_action;
                last --);
            for(int i=1; i<=last; i++)
            {
                gesturesTextOut << ACTION_STR(actionList[i].action) << " ";
                gesturesTextOut << actionList[i].duration << ";";
                if( i<last ) gesturesTextOut << "\n                      ";
            }
            gesturesTextOut << "]\n";       // NB endl writes a newline AND flushes the stream

            if( stopDumpingGestures )
            {
                gesturesTextOut.flush();
                gesturesTextFile.close();

                dumpGesturesToFile = false;
            }
        }
#endif
        // Assume that we just have vibrartion if it lasts for 1/20 sec or less
        // If jitter, then just overwrite the top of the list...
        if( actionList[0].duration > 0.05 )
        {
            if( clearOnNextChange )
            {
                for(int i=1; i < PREV_STATES_STORED; i++)
                {
                    actionList[i].action   = no_action;
                    actionList[i].duration = 0.0;
                }
                clearOnNextChange = false;
            }
            else
            {
                // Shift the table up
                for(int i=(PREV_STATES_STORED-1); i>0; i--)
                {
                    actionList[i].action   = actionList[i-1].action;
                    actionList[i].duration = actionList[i-1].duration;
                }
            }
        }
#ifdef DEBUG_TAB
        else    // actionList[0].duration is less than 1/20 sec
        {
            gesturesTextOut << "Skip getsure " << ACTION_STR(actionList[0].action);
            gesturesTextOut << " as duration was " << actionList[0].duration << "\n";
        }
#endif
        // Start a new action
        actionList[0].action   = currentAction;
        actionList[0].duration = sample->timeStep_angle_rate;
    }
    else
    {
        actionList[0].duration += sample->timeStep_angle_rate;	// the duration of the current action so far...
    }

#ifdef DEBUG_TAB
    static int skip = 0;        // Not that CPU use is an issue, but rather just on principle.
    if( skip++ > 10 )
    {
        QString gestureDebugStringNew;  // Rather than a lock, just reduce the chances of the GUI accessing during update...

        gestureDebugStringNew  = "0: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[0].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[0].duration, 'f', 7);
        gestureDebugStringNew += "\n1: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[1].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[1].duration, 'f', 7);
        gestureDebugStringNew += "\n2: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[2].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[2].duration, 'f', 7);
        gestureDebugStringNew += "\n3: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[3].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[3].duration, 'f', 7);
        gestureDebugStringNew += "\n4: Action ";
        gestureDebugStringNew += ACTION_STR(actionList[4].action);
        gestureDebugStringNew += " duration " + QString::number(actionList[4].duration, 'f', 7);
        gestureDebugStringNew += "\nFiltered Gyro: [" + QString::number(filteredGyro[0], 'f', 7);
        gestureDebugStringNew += ", " + QString::number(filteredGyro[1], 'f', 7);
        gestureDebugStringNew += ", " + QString::number(filteredGyro[2], 'f', 7);
        gestureDebugStringNew += "]\n";
        gestureDebugStringNew += "Raw gyro: [" +  QString::number(COSNETA_GYRO_RATE_G1(sample->angle_rate[0]));
        gestureDebugStringNew += ", " +  QString::number(COSNETA_GYRO_RATE_G2(sample->angle_rate[1]));
        gestureDebugStringNew += ", " +  QString::number(COSNETA_GYRO_RATE_G3(sample->angle_rate[2]));
        gestureDebugStringNew += "]\n";

        gestureDebugString = gestureDebugStringNew;

        skip = 0;
    }
#endif

    // Now look for action occurrances (saving the table index in a global)

    if( ! clearOnNextChange && findGesture(actionList) )
    {
#ifdef DEBUG_TAB

        if( dumpGesturesToFile )
        {
            gesturesTextOut << "Found: " << actions[lastGestureIndex].name << "\n======================\n";
        }
#endif
        // Was this a gesture to toggle mouse to gesture mode
        if( actions[lastGestureIndex].toggleMouseDrive )
        {
            driveMouse     = ! driveMouse;

            // And apply settings overrides
            if( ! freerunnerPointerDriveOn && driveMouse ) driveMouse = false;
#if 1
            if( driveMouse ) balloonMessage("Drive mouse from Freerunner.");
            else             balloonMessage("Freerunner gesture mode.");
#endif
            gesturePending = false;
        }
#if 1
        else
        {
            balloonMessage(actions[lastGestureIndex].name);
        }
#endif
        // Clear the list so that it is only found once

        clearOnNextChange = true; // for(int i=0; i<PREV_STATES_STORED; i++) actionList[i].action = no_action;
    }

    // TODO: move this to the appComms module
    static bool lastMouseModeButtonDown = false;
    if( 32 == (32 & sample->buttons) )
    {
        if( ! lastMouseModeButtonDown )
        {
            driveMouse = ! driveMouse;
#if 1
            if( driveMouse ) balloonMessage("Drive mouse from Freerunner.");
            else             balloonMessage("Freerunner gesture mode.");
#endif
        }
        lastMouseModeButtonDown = true;
    }
    else
    {
        lastMouseModeButtonDown = false;
    }

    return;
}

bool gestureMouseModeIsOn(void)
{
    // Has the gesture for mouse mode been enabled ?
    return ! driveMouse;
}

// Consume the most recent gesture event, returning the keypress. Then flag it as used.
bool gestureGetKeypress(int *keypress, bool *shift, bool *control)
{
    // If we have a pending gesture event, return the keycode for it

    if( gesturePending )
    {
        // in table, Key_Unknown indicates no keypress.
        if( actions[lastGestureIndex].keycode )
        {
            *keypress = actions[lastGestureIndex].keycode;
            *shift    = actions[lastGestureIndex].sendShift;
            *control  = actions[lastGestureIndex].sendCtrl;
        }

        gesturePending = false;

        return freerunnerGesturesDriveOn; // That is, only return true of the gestures are enabled
    }
    else
    {
        return false;
    }
}
#endif


