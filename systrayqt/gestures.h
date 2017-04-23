#ifndef GESTURES_H
#define GESTURES_H

#include "build_options.h"
#include "freerunner_low_level.h"
#include "invensense_low_level.h"
#include "net_connections.h"
#include "dongleData.h"
#include "main.h"

#define PREV_GESTURE_STATES_STORED 10


typedef enum
{
    no_action=0,	// No rotation for now
    yawing_right,
    yawing_left,
    pitching_up,
    pitching_down,
    rolling_clockwise,
    rolling_anticlockwise

} low_level_rot_state_t;

#define ACTION_STR(x) ( ((x)==no_action)             ? "no_action            " :\
                        ((x)==yawing_right)          ? "yawing_right         " :\
                        ((x)==yawing_left)           ? "yawing_left          " :\
                        ((x)==pitching_up)           ? "pitching_up          " :\
                        ((x)==pitching_down)         ? "pitching_down        " :\
                        ((x)==rolling_clockwise)     ? "rolling_clockwise    " :\
                        ((x)==rolling_anticlockwise) ? "rolling_anticlockwise" : "??bad value??" )

typedef struct
{
    low_level_rot_state_t action;
    double                duration;

} action_low_level_t;


class gestures
{
public:
    gestures();
    ~gestures();

    void update(cosneta_sample_t    *sample);
    void update(net_message_t       *sample);
    void update(invensense_sample_t *sample);
    void update(pen_state_t *data);           // Update from the shared table data

    enum pen_modes_e mouseModeSelected(void);
    void             forceCurrentControllerMode(enum pen_modes_e newMode);
    bool        gestureGetKeypress(int *keypress, bool *shift, bool *control);
    bool        getPresentationAction(quint32 *action);

private:
    // Methods
    low_level_rot_state_t currentAngleRateDirection(double angleRate[3]);
    bool                  findGesture(action_low_level_t currentActions[]);
    void                  updateBasedOnButtons(quint32 newButtons);

    // Data
    enum pen_modes_e   penActionMode;
    bool               gesturePending;
    int                lastGestureIndex;
    bool               lastMouseModeButtonDown;
    quint32            lastPenButtons;
    bool               presentationActionPending;
    quint32            presentationAction;
    double             filteredGyro[3];
    action_low_level_t actionList[PREV_GESTURE_STATES_STORED];
    int                lastTimeMS;
};

#if 0
void gestureInterpret( cosneta_sample_t *sample );
bool gestureMouseModeIsOn(void);
bool gestureGetKeypress(int *keypress, bool *shift, bool *control);

extern bool freerunnerGesturesDriveOn;
extern bool freerunnerPointerDriveOn;
#endif

#ifdef DEBUG_TAB

extern QString gestureDebugString;

extern void stopGestureSaving(void);
extern bool startGestureSaving(void);

#endif

#endif // GESTURES_H
