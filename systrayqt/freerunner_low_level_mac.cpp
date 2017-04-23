
// Build for MAC OSX expects both of these

#if defined(__APPLE__) && defined(__MACH__)

#include "build_options.h"

#ifdef POC1_READ_SUPPORT

#include <QString>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <sys/param.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOMediaBSDClient.h>
#include <CoreFoundation/CoreFoundation.h>


#include <stdio.h>

#include "main.h"

#include "freerunner_low_level.h"
#include "freerunner_hardware.h"

// Defines

#define LB_SIZE         512
#define BLKS_PER_PACKET 64

#define RD_LBA                5555
#define WR_LBA                6000
#define DEV_BLK_LBA           6004


// Global data (to this file)
static int     sample_rate_hz = 0;
static int     devices_used   = 0;
static int     cosneta_fd     = -1;

extern QString genericDebugString;  // In freerunnerReader.cpp


bool openHardwareDevice(int devs, int samples_per_sec)
{
    mach_port_t              masterPort;
    kern_return_t            kernResult;
    CFMutableDictionaryRef   classesToMatch;
    io_iterator_t            mediaIterator;

    kernResult = IOMasterPort( MACH_PORT_NULL, &masterPort );
    if ( kernResult != KERN_SUCCESS )
    {
        printf( "IOMasterPort returned %d\n", kernResult );
        return false;
    }
    // CD media are instances of class kIOCDMediaClass.
    classesToMatch = IOServiceMatching( kIOCDMediaClass );
    if ( classesToMatch == NULL )
        printf( "IOServiceMatching returned a NULL dictionary.\n" );
    else
    {
        // Each IOMedia object has a property with key kIOMediaEjectableKey
        // which is true if the media is indeed ejectable. So add this
        // property to the CFDictionary for matching.
        CFDictionarySetValue( classesToMatch,
                        CFSTR( kIOMediaEjectableKey ), kCFBooleanTrue );
    }
    kernResult = IOServiceGetMatchingServices( masterPort,
                                classesToMatch, &mediaIterator );
    if ( (kernResult != KERN_SUCCESS) || (mediaIterator == NULL) )
    {
//        printf( "No ejectable CD media found.\n kernResult = %d\n",
//                    kernResult );
        return false;
    }

//    fprintf(stderr,"openHardwareDevice: Not yet implemented on MAC!");

    return false;
}

void closeHardwareDevice(void)
{
    if( cosneta_fd < 0 )
    {
        fprintf(stderr,"closeHardwareDevice: ignore as no open file descriptor!\n");
        return;
    }
}

bool freerunnerDeviceOpened(void)
{
    return cosneta_fd >= 0;
}

bool readSample( cosneta_sample_t *out)
{
    fprintf(stderr,"readSample: Not yet implemented on MAC!\n");

    return false;
}

#ifdef DEBUG_TAB

#include <QTextStream>

extern bool        dumpSamplesToFile;
extern QTextStream accel1TextOut;
extern QTextStream accel2TextOut;
extern QTextStream gyroTextOut;

/* Commands used with write */

bool LowlevelWirelessDev_Read(char *buf, int buffSize)
{
    strncpy(buf,"LowlevelWirelessDev_Read: Not yet implemented on MAC!",buffSize);

    return false;
}

bool LowlevelWirelessDev_Write(char cmd, char *buf, int buffSize)
{
    strncpy(buf,"LowlevelWirelessDev_Write: Not yet implemented on MAC!",buffSize);

    return false;
}

#endif


#endif // POC1_READ_SUPPORT

#endif

