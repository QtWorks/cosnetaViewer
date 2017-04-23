#include <QApplication>
#include <iostream>

#include "../freerunner_low_level.h"
#include "../gestures.h"
#include "../hardwareCalibration.h"

using namespace std;


// Included here (rathyer than used as a separate module) so that we can access the internal data
#include "../gestures.cpp"


// /////////////////////////////
// Dummy code for main callbacks

void balloonMessage(QString msg)
{
    cout << msg.toStdString() << endl;
}
void setMouseButtons(QPoint *pos, int buttons)
{
    // Do nothing
}
void msleep(unsigned long t)
{
    // Do nothing
}
// Record the output here to alloow for verification of the results
int lastKeyWithModifier = 0;
int lastKeyWithShift    = 0;
int lastKeyWithControl  = 0;
void sendKeypressToOS(int keyWithModifier, bool withShift, bool withControl)
{
    lastKeyWithModifier = keyWithModifier;
    lastKeyWithShift    = withShift;
    lastKeyWithControl  = withControl;
}

// ///////////////////////////////////////
// Dummy code for gestures only unit tests
bool freerunnerDeviceOpened(void)
{
    return true;
}

// allow extra debug during development

#define DUMP_STATE 1

// //////////////////
// Gesture unit tests

// Table of data for test

typedef struct
{
    double    duration;
    double    angle_rate_state[3];
    char     *message;

} gesture_test_state_t;

static gesture_test_state_t gesture_tests[] =
{
    {1.00, { 0.00,  0.00,  0.00}, NULL},
    {0.40, { 0.00,  0.40,  0.00}, NULL},
    {0.10, { 0.00,  0.00,  0.00}, NULL},
    {0.40, { 0.00, -0.40,  0.00}, "Completed: Pause, pitchUp, Pause, PitchDown @ 0.4"},

    {1.00, { 0.00,  0.00,  0.00}, NULL},
    {0.40, { 0.00,  0.40,  0.00}, NULL},
    {0.40, { 0.00, -0.40,  0.00}, "Completed: Pause, pitchUp, PitchDown @ 0.4"},

    {1.00, { 0.00,  0.00,  0.00}, NULL},
    {0.40, { 0.00,  0.80,  0.00}, NULL},
    {0.10, { 0.00,  0.00,  0.00}, NULL},
    {0.40, { 0.00, -0.80,  0.00}, "Completed: Pause, pitchUp, Pause, PitchDown @ 0.8"},

    {1.00, { 0.00,  0.00,  0.00}, NULL},
    {0.40, { 0.00,  0.80,  0.00}, NULL},
    {0.40, { 0.00, -0.80,  0.00}, "Completed: Pause, pitchUp, PitchDown @ 0.8"}

};
#define NUM_GESTURE_TESTS (sizeof(gesture_tests)/sizeof(gesture_test_state_t))


static void gestureUnitTests(void)
{
    cosneta_sample_t sample;
    double           tm;
    int              testNum;

    memset(&sample, 0, sizeof(sample));

    cout << "Gesture Unit Tests." << endl;
    cout << "===================" << endl;

    // Test rate (test data does not specify rate)
    sample.timeStep_angle_rate = 0.001;
    tm                         = 0.00;

    for(testNum=0; testNum<NUM_GESTURE_TESTS; testNum++)
    {
        // Set the angle rates
        sample.angle_rate[0] = (COS_ANA_GYRO_ZERO_G1+(gesture_tests[testNum].angle_rate_state[0]/COS_ANA_GYRO_SCALE_G1));
        sample.angle_rate[1] = (COS_ANA_GYRO_ZERO_G2+(gesture_tests[testNum].angle_rate_state[1]/COS_ANA_GYRO_SCALE_G2));
        sample.angle_rate[2] = (COS_ANA_GYRO_ZERO_G3+(gesture_tests[testNum].angle_rate_state[2]/COS_ANA_GYRO_SCALE_G3));

#ifdef DUMP_STATE
        cout << "Gyros to: [" << COSNETA_GYRO_RATE_G1(sample.angle_rate[0]);
        cout << ", " << COSNETA_GYRO_RATE_G2(sample.angle_rate[1]) << ", ";
        cout << COSNETA_GYRO_RATE_G3(sample.angle_rate[2]) << "]" << endl;
#endif
        for(tm=0.0; tm<=1.0; tm+=sample.timeStep_angle_rate)
        {
            gestureInterpret(&sample);
#ifdef DUMP_STATE
            static low_level_rot_state_t prevFirstAction = no_action;
            if( prevFirstAction != actionList[0].action )
            {
                int last;
                cout << "Action list change:\n                      ";
                for(last=PREV_STATES_STORED-1;
                    last>=0 && actionList[last].action == no_action;
                    last --);
                for(int i=1; i<=last; i++)
                {
                    cout << ACTION_STR(actionList[i].action) << " ";
                    cout << actionList[i].duration << ";\n";
                    if( i<last ) cout << "                      ";
                }
                cout << endl;

                prevFirstAction = actionList[0].action;
            }
#endif
        }

#ifdef DUMP_STATE
//        cout << gestureDebugString.toStdString() << endl;
#endif
        if( gesture_tests[testNum].message ) cout << gesture_tests[testNum].message << endl;
    }
}


// And finally the entry point
int main(int argc, char *argv[])
{
    // Like it says
    gestureUnitTests();


    return 0;
}
