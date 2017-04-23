
#ifdef __gnu_linux__

#include "build_options.h"

#ifdef POC1_READ_SUPPORT

#include <QString>

#include "unistd.h"
#include "string.h"
#include "stdio.h"
#include "errno.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <time.h>

#include "main.h"

#include "freerunner_low_level.h"
#include "freerunner_hardware.h"

// Defines

#define LB_SIZE         512
#define BLKS_PER_PACKET 64

#define RD_LBA                5555
#define WR_LBA                6000
#define DEV_BLK_LBA             18


// Global data (to this file)
static int     sample_rate_hz = 0;
static int     devices_used   = 0;
static int     cosneta_fd     = -1;

extern QString genericDebugString;  // In freerunnerReader.cpp



#ifdef DEBUG_TAB
bool LowlevelWirelessDev_Read(char *buf, int buffSize)
{
    int                    ret;
    sg_io_hdr_t            io;
    unsigned char          sense_b[SG_MAX_SENSE];		// Status info from ioctl (old driver max size, hopefully okay)
    unsigned char          scsiRead10[10] = {
                                              READ_10,              // Command
                                              0,                     // Flags: WRPROTECT [3], DPO [1], FUA [1], Reserved [1], FUA_NV [1], Obsolete [1]
                                              (DEV_BLK_LBA >> 24) & 255,  // Block address on SCSI "drive"
                                              (DEV_BLK_LBA >> 16) & 255,
                                              (DEV_BLK_LBA >> 8)  & 255,
                                               DEV_BLK_LBA        & 255,
                                              0,                     // Group number
                                              0, 1,	// Length (read 1 * LS_SIZE(512) bytes = 512)
                                              0                      // Control
                                            };
    if( cosneta_fd < 0 )
    {
        if( ! openHardwareDevice() )
        {
            strncpy(buf,"LowlevelWirelessDev_Read: exit as failed to open file descriptor!",
                    buffSize);
            return false;
        }
    }

    // Fill in the io header for this scsi read command
    bzero(&io,sizeof(io));
    io.interface_id    = 'S';                     // Always set to 'S' for sg driver
    io.cmd_len         = sizeof(scsiRead10);      // Size of SCSI command
    io.cmdp            = scsiRead10;              // SCSI command buffer
    io.dxfer_direction = SG_DXFER_FROM_DEV;       // Data transfer direction(no data) - -ve => new interface FROM => read
    io.dxfer_len       = 1*LB_SIZE;               // byte count of data transfer (blk size is 512)
    io.dxferp          = buf;                     // Data transfer buffer
    io.sbp             = sense_b;                 // Sense buffer
    io.mx_sb_len       = sizeof(sense_b);         // Max sense buffer size (for error)
    io.flags           = SG_FLAG_DIRECT_IO;       // Request direct IO (should still work if not honoured)
    io.timeout         = 5000;                    // Timeout (5s)

    // Start with an empty buffer (if not filled)
    buf[0] = (char)0;

    // Read the next packet (NB doesn't read past the end of a line in a single action)
    ret = ioctl(cosneta_fd, SG_IO, &io);
    if( ret < 0 )
    {
        snprintf(buf,buffSize,"readSample: exit as ioctl failed on read! Ret = %d  readThread: (errno=%d: %s) ",ret,errno,
                 strerror(errno));
        closeHardwareDevice();
        return false;
    }
    if( io.masked_status != 0 ) /* ie not GOOD */
    {
        strncpy(buf,"readSample: masked_status=",buffSize);
        switch( io.masked_status )
        {
        case 0x01: strncat(buf,"CHECK_CONDITION",buffSize);		 break;
        case 0x02: strncat(buf,"CONDITION_GOOD",buffSize);		 break;
        case 0x04: strncat(buf,"BUSY",buffSize);                 break;
        case 0x08: strncat(buf,"INTERMEDIATE_GOOD",buffSize);    break;
        case 0x0a: strncat(buf,"INTERMEDIATE_C_GOOD",buffSize);  break;
        case 0x0c: strncat(buf,"RESERVATION_CONFLICT",buffSize); break;
        case 0x11: strncat(buf,"COMMAND_TERMINATED",buffSize);   break;

        default:   strncat(buf,"Unknown error num",buffSize);
        }

        return false;
    }

    // Just in case we get unterminated garbage from the board.
    buf[buffSize-1] = (char)0;

    return true;
}

/* First byte is a command. The rest is data. */
bool LowlevelWirelessDev_Write(char cmd, char *buf, int buffSize)
{
    sg_io_hdr_t         io;
    unsigned char       scsiWrite10[10] = {
                                           WRITE_10,                   // Command
                                           0,                          // Flags: WRPROTECT [3], DPO [1], FUA [1], Reserved [1], FUA_NV [1], Obsolete [1]
                                           (DEV_BLK_LBA >> 24) & 255,  // Block address on SCSI "drive"
                                           (DEV_BLK_LBA >> 16) & 255,
                                           (DEV_BLK_LBA >> 8)  & 255,
                                            DEV_BLK_LBA        & 255,
                                           0,                          // Group number
                                           0, 0,                       // Length
                                           0                           // Control
                                          };
    unsigned char       out[64];			// Improbable output buffer for SCSI command
    unsigned char       command[512];

    if( cosneta_fd < 0 )
    {
        strncpy(buf,"LowlevelWirelessDev_Write: exit as no open file descriptor!",buffSize);
        return false;
    }

    // Create command data
    command[0] = cmd;
    memcpy(command+1, buf, buffSize<511?buffSize:511);

    scsiWrite10[0] = WRITE_10;
    scsiWrite10[7] = 0; // sizeof(cfg)/LB_SIZE / 256;
    scsiWrite10[8] = 1; // sizeof(cfg)/LB_SIZE & 255;

    bzero(&io,sizeof(io));
    io.interface_id     = (int)'S';
    io.dxfer_direction  = SG_DXFER_TO_DEV;            /* int */
    io.cmd_len          = sizeof(scsiWrite10);        /* unsigned char: 6-16 */
    io.mx_sb_len        = 1; //BLKS_PER_PACKET;       /* Size of scsi output (not used, I hope): unsigned char  */
    io.iovec_count      = 0;                          /* Number of scatter/gather elements (in use space): unsigned short  (0=>single block) */
    io.dxfer_len        = sizeof(command);            /* transfer length: unsigned int (number of logical blocks to xfer) */
    io.dxferp           = command;                    /* transfer data: void *  */
    io.cmdp             = &(scsiWrite10[0]);          /* points to the SCSI command to be executed: unsigned char *   */
    io.sbp              = out;                        /* scsi output (not used, I hope): unsigned char *   */
    io.timeout          = 10000;                      /* unsigned int  unit: millisecs, ie this is 10s */
    io.flags            = SG_FLAG_DIRECT_IO;          /* unsigned int  */
    io.pack_id          = 0;                          /* Not normally used: int */
    io.usr_ptr          = NULL;                       /* Not used: void *  */

    if( ioctl( cosneta_fd, SG_IO, &io) < 0 )
    {
        genericDebugString = "Failed cosneta configure ioctl().";
        balloonMessage(genericDebugString);

        return false;
    }

    if( io.masked_status != 0 ) /* ie not GOOD */
    {
        QString msg = "readSample: masked_status=";

        switch( io.masked_status )
        {
        case 0x01: msg += "CHECK_CONDITION";      break;
        case 0x02: msg += "CONDITION_GOOD";	      break;
        case 0x04: msg += "BUSY";                 break;
        case 0x08: msg += "INTERMEDIATE_GOOD";    break;
        case 0x0a: msg += "INTERMEDIATE_C_GOOD";  break;
        case 0x0c: msg += "RESERVATION_CONFLICT"; break;
        case 0x11: msg += "COMMAND_TERMINATED";   break;

        default:   msg += "Unknown error num";
        }

        return false;
    }

    return true;
}

#endif

// Here we return true is the device has been successfully opened. That is,
// the device has been plugged in. Here we do a wait for the device to
// appear, allowing different operating systems to poll or do asynchronous
// waits as appropriate.

bool openHardwareDevice(void)
{
    char               *dev_path = "/dev/freerunner";
    sg_io_hdr_t         io;

    if( cosneta_fd >= 0 )
    {
        close( cosneta_fd );
    }

    cosneta_fd = open(dev_path, O_RDWR);

    // Open the device for use
    if( cosneta_fd < 0 )
    {
        genericDebugString = "Failed to open the Cosneta device!";
	
        sleep(1);	/* Wait for 1 second before returning to try again */
        
        return false;
    }

    genericDebugString = "Freerunner device opened.";

    return true;
}


// Here we close the device if it was opened. Otherwise we just return.

void closeHardwareDevice(void)
{
    if( cosneta_fd >= 0 )
    {
        close( cosneta_fd );
    }
}


bool freerunnerDeviceOpened(void)
{
    return cosneta_fd >= 0;
}



// Read a single sample from the device

bool readSample(cosneta_sample_t *sample)
{
    static int             sampleNum       = 1;
    static int             numSamplesAvail = 0;
    static int             numGyro;
    static int             numAccel1;
    static int             numAccel2;
    int                    numUserInput;
    static int             currentButtonState = 0;
    static unsigned short *accel1;
    static unsigned short *accel2;
    static unsigned short *gyro;
    cosneta_user_input_t  *userInput;
    static double          lastTime        = 0.0;
    static double          timeStep        = 0.01;
    static bool            haveLastTime    = false;
    int                    index;
    sg_io_hdr_t            io;
    int                    ret;
    static unsigned char   readBuffer[64*LB_SIZE] = { 0 };		// Each read will end up in here
    cosneta_payload_t     *payload = (cosneta_payload_t *)readBuffer;	// Simplify decodes
    unsigned char          sense_b[SG_MAX_SENSE];		// Status info from ioctl (old driver max size, hopefully okay)
    unsigned char          scsiRead10[10] = {
                                             READ_10,               // Command
                                             0,                     // Flags: WRPROTECT [3], DPO [1], FUA [1], Reserved [1], FUA_NV [1], Obsolete [1]
                                             0x00, 0x00, 0x15, 0xB3,// LBA (address) 5555dec = 15b3h
                                             0,                     // Group number
                                             0, BLKS_PER_PACKET,	// Length (read 64 * LS_SIZE(512) bytes = 32768)
                                             0                      // Control
                                            };
    if( cosneta_fd < 0 )
    {
        genericDebugString = "readThread: exit as no open file descriptor!";

        return false;
    }

    bzero(&io, sizeof(io));

    if( sampleNum >= numSamplesAvail )
    {
        double          newTime;
        struct timespec tp;

        // Fill in the io header for this scsi read command
        io.interface_id    = 'S';			// Always set to 'S' for sg driver
        io.cmd_len         = sizeof(scsiRead10);	// Size of SCSI command
        io.cmdp            = scsiRead10;		// SCSI command buffer
        io.dxfer_direction = SG_DXFER_FROM_DEV;		// Data transfer direction(no data) - -ve => new interface FROM => read
        io.dxfer_len       = BLKS_PER_PACKET*LB_SIZE;	// byte count of data transfer (blk size is 512)
        io.dxferp          = readBuffer;		// Data transfer buffer
        io.sbp             = sense_b;			// Sense buffer
        io.mx_sb_len       = sizeof(sense_b);		// Max sense buffer size (for error)
        io.flags           = SG_FLAG_DIRECT_IO;		// Request direct IO (should still work if not honoured)
        io.timeout         = 5000;			// Timeout (5s)

        // Read the next packet (NB doesn't read past the end of a line in a single action)
        ret = ioctl(cosneta_fd, SG_IO, &io);
        if( ret < 0 )
        {
            genericDebugString = "readSample: exit as ioctl failed on read! Ret = "+QString::number(ret)+"\nreadThread: ("+QString::number(errno)+" 25=ENOTTY) ";

            return false;
        }
        if( io.masked_status != 0 ) /* ie not GOOD */
        {
            genericDebugString = "readSample: masked_status = ";
            switch( io.masked_status )
            {
            case 0x01: genericDebugString+="CHECK_CONDITION";		break;
            case 0x02: genericDebugString+="CONDITION_GOOD";		break;
            case 0x04: genericDebugString+="BUSY";                  break;
            case 0x08: genericDebugString+="INTERMEDIATE_GOOD";     break;
            case 0x0a: genericDebugString+="INTERMEDIATE_C_GOOD"; 	break;
            case 0x0c: genericDebugString+="RESERVATION_CONFLICT";	break;
            case 0x11: genericDebugString+="COMMAND_TERMINATED";  	break;

            default:   genericDebugString+="Unknown ("+QString::number(io.masked_status)+")";
            }

            return false;
        }
        // Get the current time
        if( 0 != clock_gettime(CLOCK_REALTIME, &tp) )
        {
            haveLastTime = false;
        }
        else
        {
            newTime = tp.tv_sec + (tp.tv_nsec*1e-9);

            // And maintain the delta-time
            if( haveLastTime )
            {
                timeStep = newTime - lastTime;
            }
            else
            {
                timeStep     = 0.01;
                haveLastTime = true;
            }
            lastTime = newTime;
        }


        // and now let's look at the data we received (numbers of each).
        
        numAccel1    = (int)(payload->accel_1_data_sz) / (sizeof(short)*3);
        numAccel2    = (int)(payload->accel_2_data_sz) / (sizeof(short)*3);
        numGyro      = (int)(payload->gyro_data_sz)    / (sizeof(short)*3);
        numUserInput = (int)(payload->user_input_sz);

        numSamplesAvail = 0;
        
        if( (devices_used & DEVICE_ACCEL1) && (devices_used & DEVICE_ACCEL2) )
        {
            if( numAccel1 != numAccel2 )
            {
                genericDebugString  = "readSample: number of accelerometer readings do not \n  match each other: repeating samples!\n";
                genericDebugString += " NumAccel1=" + QString::number(numAccel1) + " NumAccel2=" + QString::number(numAccel2);
                genericDebugString += " NumGyro=" + QString::number(numGyro); 
            }
            numSamplesAvail = (numAccel1 > numAccel2) ? numAccel1 : numAccel2;
        }
        else
        {
            numSamplesAvail = (devices_used & DEVICE_ACCEL1) ? numAccel1 : numAccel2;
        }
        
        if( (devices_used & (DEVICE_ACCEL1|DEVICE_ACCEL2)) && (devices_used & DEVICE_DIGIGYRO) )
        {
            if( numSamplesAvail != numGyro )
            {
                genericDebugString  = "readSample: number of accelerometer readings do not \n  match num gyro readings: repeating samples!\n";
                genericDebugString += " NumAccel1=" + QString::number(numAccel1) + " NumAccel2=" + QString::number(numAccel2);
                genericDebugString += " NumGyro=" + QString::number(numGyro); 
            }
            numSamplesAvail = numSamplesAvail > numGyro ? numSamplesAvail : numGyro;
        }
        
        if( numSamplesAvail < 1 )
        {
            genericDebugString = "readSample: No samples read";
	    
            return false;
        }
        
        if( ((numAccel1+numAccel2+numGyro)*3*sizeof(short) + sizeof(cosneta_payload_t)) > sizeof(readBuffer) )
        {
            genericDebugString = "readSample: Bad data sizes: bigger than the read buffer!";
	    
            return false;
        }
        
        accel1    = &(payload->data[0]);		// And save some effort on subsequent reads
        accel2    = accel1 + 3*numAccel1;
        gyro      = accel2 + 3*numAccel2;
        userInput = (cosneta_user_input_t *)(gyro + 3*numGyro);

#ifdef DEBUG_TAB

        if( dumpSamplesToFile )
        {
            // Dump accel1, accel2 & gyro data to different files to allow for different numbers of samples

            dumpSamples(accel1TextOut,newTime,accel1,numAccel1);
            dumpSamples(accel2TextOut,newTime,accel2,numAccel2);
            dumpSamples(gyroTextOut,  newTime,gyro,  numGyro);
            dumpUserInput(newTime, (unsigned char *)userInput, numUserInput);
        }
#endif
        // Simply agregate the user input changes (rather than trying to insert them with the acceleration data,
        // which would be needless complexity.

        while( numUserInput > 0 )
        {
            currentButtonState = userInput->new_button_state;

            userInput ++;
            numUserInput -= sizeof(cosneta_user_input_t);
        }

#ifdef DEBUG_TAB

        QString debugString = "";

        debugString += "Buttons: ";
        debugString += ((currentButtonState) & 32) ? "1" : "0";
        debugString += ((currentButtonState) & 16) ? "1" : "0";
        debugString += ((currentButtonState) &  8) ? "1" : "0";
        debugString += ((currentButtonState) &  4) ? "1" : "0";
        debugString += ((currentButtonState) &  2) ? "1" : "0";
        debugString += ((currentButtonState) &  1) ? "1" : "0";
        debugString += "\n\n";

        genericDebugString = debugString;
#endif
        sampleNum = 0;
    }
    
    /* Extract the next sample from the current packet */
    if( devices_used & DEVICE_ACCEL1 )
    {
        index = 3 * numAccel1 * sampleNum / numSamplesAvail;
        sample->accel1[0] = accel1[index + 0]; if( sample->accel1[0] >= 32768 ) sample->accel1[0] -= 65536;
        sample->accel1[1] = accel1[index + 1]; if( sample->accel1[1] >= 32768 ) sample->accel1[1] -= 65536;
        sample->accel1[2] = accel1[index + 2]; if( sample->accel1[2] >= 32768 ) sample->accel1[2] -= 65536;
    }
    
    if( devices_used & DEVICE_ACCEL2 )
    {
        index = 3*((numAccel2 * sampleNum) / numSamplesAvail);
        sample->accel2[0] = accel2[index + 0]; if( sample->accel2[0] >= 32768 ) sample->accel2[0] -= 65536;
        sample->accel2[1] = accel2[index + 1]; if( sample->accel2[1] >= 32768 ) sample->accel2[1] -= 65536;
        sample->accel2[2] = accel2[index + 2]; if( sample->accel2[2] >= 32768 ) sample->accel2[2] -= 65536;
    }

    if( devices_used & DEVICE_DIGIGYRO )
    {
        index = 3*((numGyro   * sampleNum) / numSamplesAvail);
        sample->angle_rate[0] = gyro[index + 0]; if( sample->angle_rate[0] >= 32768 ) sample->angle_rate[0] -= 65536;
        sample->angle_rate[1] = gyro[index + 1]; if( sample->angle_rate[1] >= 32768 ) sample->angle_rate[1] -= 65536;
        sample->angle_rate[2] = gyro[index + 2]; if( sample->angle_rate[2] >= 32768 ) sample->angle_rate[2] -= 65536;
    }

    // Calculate the individual device time steps...
    if( devices_used & DEVICE_DIGIGYRO ) sample->timeStep_angle_rate = timeStep/numGyro;
    if( devices_used & DEVICE_ACCEL1 )   sample->timeStep_accel1     = timeStep/numAccel1;
    if( devices_used & DEVICE_ACCEL2 )   sample->timeStep_accel2     = timeStep/numAccel2;

    sample->buttons = currentButtonState;    // Messages for user inputs are documented as TBD

    sampleNum ++;

    return true;
}


#endif



#endif // POC1_READ_SUPPORT
