#ifndef INVENSENSE_LOW_LEVEL_H
#define INVENSENSE_LOW_LEVEL_H

typedef struct
{
    double quaternion[4];

} invensense_sample_t;

bool invensenseReadSample(invensense_sample_t *sample);
bool invensenseOpenDevice(int portNumber);
void invensenseCloseDevice();

#endif // INVENSENSE_LOW_LEVEL_H
