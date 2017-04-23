#ifndef PEN_LOCATION_H
#define PEN_LOCATION_H

#include <QPoint>
#include <QDesktopWidget>
#include "../cosnetaapi/cosnetaAPI.h"
#include "../cosnetaapi/sharedMem.h"
#include "freerunner_low_level.h"
#include "invensense_low_level.h"
#include "dongleData.h"

typedef struct
{
    double x;
    double y;
    double z;
    double roll;
    double pitch;
    double yaw;

} location_3D_t;


class pen_location
{
public:
    pen_location(pen_state_t *out, pen_settings_t *settings);
    ~pen_location();

    void updateScreenGeometry(QDesktopWidget *desktop);

    void update(cosneta_sample_t    *sample);
    void update(invensense_sample_t *sample);
    void update(pen_sensor_data_t   *sample);

    void getCurrent3Dpos(location_3D_t *loc);
    void getCurrent2Dpos(QPoint *pos);

private:
    QRect           screenInPixels;
    QPointF         mousePosition;
    pen_state_t    *outputState;       // This is what we maintain
    pen_settings_t *penSetings;

    bool           LNDactive;
    double         LND_X;
    double         LND_Y;

    double         LND_X_Scale;
    double         LND_Y_Scale;

    double         cursorOffsetX;
    double         cursorOffsetY;

};

#endif // PEN_LOCATION_H
