
#include "build_options.h"

#ifdef POC1_READ_SUPPORT

// Just check for Windows 32 bit build for now. Later add _WIN64
#ifdef _WIN32

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <QString>
#include <QThread>
#include <QFile>

#include "Setupapi.h"
#include "WinIoctl.h"
#include "Ntddscsi.h"

#include "main.h"
#include "freerunner_low_level.h"
#include "freerunner_hardware.h"

#define PoC_USB_DEVICE_DRIVER L"USB\\VID_03EB&PID_6129"
#define RD_LBA                5555
#define WR_LBA                6000

// #define DEV_BLK_LBA           6004
// FAT12
//#define DEV_BLK_LBA           18
// FAT32
#define LOGICAL_OFFSET       ((32761*1+32+63)-2)
#define DEV_BLK_LBA           (LOGICAL_OFFSET+3)

#define PoC_NUM_BLOCKS        _T("8")
#define PoC_MAX_SAMPLE_ACCEL  (24*6)        /* Max sample sizes */
#define PoC_MAX_SAMPLE_GYRO   (20*6)
// #define TXFER_COUNT         8            /* >1 to make SCSI transfers more efficient */
#define TXFER_COUNT            1			/* Low latency */
#define SCSI_BLOCKSIZE         512

// #ifdef MANUF_CODE
// else
#include "cfgmgr32.h"

/* SCSI data */
#define READ10  0x28
#define WRITE10 0x2A

// Types


// Globals
static bool    deviceOpened              = false;
static HANDLE  Handle                    = NULL;
static int     devices_used              = 0;
static int     requested_samples_per_sec = 0;

extern QString genericDebugString;  // In freerunnerReader.cpp



static bool configHardwareDevice(int devs, int samples_per_sec)
{
    // And record the configuration parameters
    devices_used              = devs;
    requested_samples_per_sec = samples_per_sec;

    return true;
}

bool openHardwareDevice(void)
{
    HDEVINFO		                 hDevInfo;
    SP_DEVINFO_DATA	                 devInfoData;
    WCHAR                            deviceIDStr[MAX_DEVICE_ID_LEN];
    SP_DEVICE_INTERFACE_DATA         devIfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA *devIfaceDetailData_p;

    if( deviceOpened )
    {
        // Allows a re-configure.
        closeHardwareDevice();
        deviceOpened = false;
    }

    devIfaceDetailData_p=(SP_DEVICE_INTERFACE_DETAIL_DATA *)calloc(4+1024,sizeof(WCHAR));
    if (devIfaceDetailData_p == NULL)
    {
#ifdef DEBUG_TAB
        genericDebugString = "Failed calloc() for devIfaceDetailData_p";
        balloonMessage(genericDebugString); Sleep(1000);
#endif
        return false;
    }

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
#ifdef DEBUG_TAB
        genericDebugString = "Failed SetupDiGetClassDevs()";
        balloonMessage(genericDebugString); Sleep(1000);
#endif
        return false;
    }

    int  idx=0;
    devInfoData.cbSize = sizeof(devInfoData);
    while( SetupDiEnumDeviceInfo(hDevInfo, idx, &devInfoData) )
    {
        /* Is it the cosneta device? (NB This is Win7 and up only) */
        if( CR_SUCCESS == CM_Get_Device_ID(devInfoData.DevInst, deviceIDStr, MAX_DEVICE_ID_LEN, 0) )
        {
            if( wcsstr(deviceIDStr, L"USBSTOR\\DISK&VEN_COSNETA") )
            {
                /* Device found with correct manufacturer and product IDs */
                devIfaceData.cbSize          = sizeof(devIfaceData);
                devIfaceDetailData_p->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                if ( SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, idx, &devIfaceData) )
                {
                    if ( SetupDiGetDeviceInterfaceDetail(hDevInfo, &devIfaceData,
                                                         devIfaceDetailData_p, (4+1024)*sizeof(WCHAR),
                                                         NULL, NULL )                          )
                    {
                        /* Open the device and leave the handle in the global "Handle" */
                        HANDLE hDev = CreateFile(devIfaceDetailData_p->DevicePath,
                                                 GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                                 NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

                        if( hDev != INVALID_HANDLE_VALUE )
                        {
                            // EXIT here on success!!!
                            Handle = hDev;

                            deviceOpened = true;

                            SetupDiDestroyDeviceInfoList(hDevInfo);
                            free(devIfaceDetailData_p);

                            return true; // configHardwareDevice(devs, samples_per_sec);
                        }
#ifdef DEBUG_TAB
                        else
                        {
                            DWORD err=GetLastError();
                            genericDebugString = "Failed to CreateFile for freerunner device.\nErr(5=> access denied): " + QString::number(err);
                            balloonMessage(genericDebugString); Sleep(1000);
                        }
#endif
                    }
#ifdef DEBUG_TAB
                    else
                    {
                        genericDebugString = "Failed to extract file path.\nFailed SetupDiGetDeviceInterfaceDetail()\nErr= " + QString::number(GetLastError());
                        balloonMessage(genericDebugString); Sleep(1000);
                    }
#endif
                }
#ifdef DEBUG_TAB
                else
                {
                    genericDebugString = "Failed to extract file path.\nFailed SetupDiEnumDeviceInterfaces()\nErr= " + QString::number(GetLastError());
                    balloonMessage(genericDebugString); Sleep(1000);
                }
#endif
                //	Failed, but keep iteterating in case there is a similar device that does work
            }
        }

        /* And next in list of devices */
        devInfoData.cbSize = sizeof(devInfoData);
        idx ++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    free(devIfaceDetailData_p);

    deviceOpened = false;

    return false;
}

bool freerunnerDeviceOpened(void)
{
    return deviceOpened;
}


#define RX_DATA_BUFFER_SIZE (sizeof(SCSI_PASS_THROUGH) + SCSI_BLOCKSIZE * TXFER_COUNT)
#define RX_DATA_SIZE        (RX_DATA_BUFFER_SIZE - sizeof(SCSI_PASS_THROUGH))


#ifdef DEBUG_TAB
bool LowlevelWirelessDev_Read(char *buf, int buffSize)
{
    static unsigned char      dataBuffer[RX_DATA_BUFFER_SIZE] = { 0 };
    static SCSI_PASS_THROUGH *spt = (SCSI_PASS_THROUGH *)dataBuffer;

    if( ! deviceOpened )
    {
        if( ! openHardwareDevice() )
        {
            strcpy(buf,"Attempted to read and unable to open device file.");
            return false;
        }
    }

    spt->Length             = sizeof(SCSI_PASS_THROUGH);
    spt->PathId             = 0;
    spt->TargetId           = 1;
    spt->Lun                = 0;
    spt->CdbLength          = 10;
    spt->SenseInfoLength    = 0;
    spt->DataIn             = SCSI_IOCTL_DATA_IN;			// Read data FROM the device
    spt->DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    spt->TimeOutValue       = 2;							// Seconds
    spt->DataBufferOffset   = spt->Length;
    spt->SenseInfoOffset    = 0;

    spt->Cdb[0] =  READ10;  // 0x28
    spt->Cdb[1] =  0;
    spt->Cdb[2] = (DEV_BLK_LBA >> 24) & 255;
    spt->Cdb[3] = (DEV_BLK_LBA >> 16) & 255;
    spt->Cdb[4] = (DEV_BLK_LBA >> 8)  & 255;
    spt->Cdb[5] =  DEV_BLK_LBA        & 255;
    spt->Cdb[6] =  0;
    spt->Cdb[7] = (TXFER_COUNT >> 8) & 255;
    spt->Cdb[8] =  TXFER_COUNT       & 255;
    spt->Cdb[9] =  0;
memset(dataBuffer+sizeof(SCSI_PASS_THROUGH),0xff,SCSI_BLOCKSIZE);
    DWORD dwBytesReturned=0;

    if ( ! DeviceIoControl(Handle, IOCTL_SCSI_PASS_THROUGH,
                           spt, spt->Length,							// inbuffer,  inBufSize
                           spt, spt->Length+spt->DataTransferLength,	// outbuffer, outBufSize
                           &dwBytesReturned, NULL) )
    {
        sprintf(buf,"Failed to read Freerunner data. Error=%d",GetLastError());
        closeHardwareDevice();
        return false;
    }

    if ( dwBytesReturned < spt->Length )
    {
        sprintf(buf,"Error in read Freerunner data.\ndwBytesReturned(%d) < spt->Length (%d)",
                    dwBytesReturned, spt->Length);
        return false;
    }

    // Copy out pointer to the returned data.
    memcpy(buf,&(dataBuffer[sizeof(SCSI_PASS_THROUGH)]),buffSize);
    buf[buffSize-1] = (char)0; // Just to be sure

    return true;
}

/* First byte is a command. The rest is data. */
bool LowlevelWirelessDev_Write(char cmd, char *buf, int buffSize)
{
    unsigned char     *dataBuffer;
    SCSI_PASS_THROUGH *spt;
    int                dataBufferSize;

    if( ! deviceOpened )
    {
        balloonMessage("Attempted to read when device file was not open.");
        return false;
    }

    dataBufferSize = sizeof(SCSI_PASS_THROUGH) + (1+((buffSize+1)/SCSI_BLOCKSIZE))*SCSI_BLOCKSIZE; // spt + rounded up number of blocks

    dataBuffer = (unsigned char *)calloc(dataBufferSize,sizeof(char));

    if( ! dataBuffer )
    {
        balloonMessage("Failed to allocate SCSI transmit buffer");
        return false;
    }

    /* Fill in the databuffer... */
    memset(dataBuffer,0,dataBufferSize);

    /* Fill in SCSI control stuff */
    spt = (SCSI_PASS_THROUGH *)dataBuffer;

    spt->Length             = sizeof(SCSI_PASS_THROUGH);
    spt->PathId             = 0;
    spt->TargetId           = 1;
    spt->Lun                = 0;
    spt->CdbLength          = 10;
    spt->SenseInfoLength    = 0;
    spt->DataIn             = SCSI_IOCTL_DATA_OUT;			// Write data TO the device
    spt->DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    spt->TimeOutValue       = 2;							// Seconds
    spt->DataBufferOffset   = spt->Length;
    spt->SenseInfoOffset    = 0;

    spt->Cdb[0] =  WRITE10;  // 0x2A
    spt->Cdb[1] =  0;
    spt->Cdb[2] = (DEV_BLK_LBA >> 24) & 255;
    spt->Cdb[3] = (DEV_BLK_LBA >> 16) & 255;
    spt->Cdb[4] = (DEV_BLK_LBA >> 8)  & 255;
    spt->Cdb[5] =  DEV_BLK_LBA        & 255;
    spt->Cdb[6] =  0;
    spt->Cdb[7] = (TXFER_COUNT >> 8) & 255;
    spt->Cdb[8] =  TXFER_COUNT       & 255;
    spt->Cdb[9] =  0;

    /* And copy in the data to send */
    dataBuffer[1+sizeof(SCSI_PASS_THROUGH)] = cmd;

    memcpy(dataBuffer+1+sizeof(SCSI_PASS_THROUGH), buf, buffSize);

    /* Now use the Windaes API to send it. */

    DWORD dwBytesReturned=0;

    if ( ! DeviceIoControl(Handle, IOCTL_SCSI_PASS_THROUGH,
                           spt, spt->Length,							// inbuffer,  inBufSize
                           spt, spt->Length+spt->DataTransferLength,	// outbuffer, outBufSize
                           &dwBytesReturned, NULL) )
    {
        char b[128];
        sprintf(b,"Failed to write data to Freerunner. Error=%d",GetLastError());
        balloonMessage(b);

        free(dataBuffer);

        return false;
    }

    /* No interest in any return. Clean up and return. */

    free(dataBuffer);

    return true;
}

#endif

static bool LowlevelIO_Read(void **buf)
{
    static char               dataBuffer[ sizeof(SCSI_PASS_THROUGH) + SCSI_BLOCKSIZE * TXFER_COUNT] = { 0 };
    static SCSI_PASS_THROUGH *spt = (SCSI_PASS_THROUGH *)dataBuffer;

    if( ! deviceOpened )
    {
        if( ! openHardwareDevice() )
        {
            return false;
        }
    }

    spt->Length             = sizeof(SCSI_PASS_THROUGH);
    spt->PathId             = 0;
    spt->TargetId           = 1;
    spt->Lun                = 0;
    spt->CdbLength          = 10;
    spt->SenseInfoLength    = 0;
    spt->DataIn             = SCSI_IOCTL_DATA_IN;			// Read data FROM the device
    spt->DataTransferLength = TXFER_COUNT*SCSI_BLOCKSIZE;
    spt->TimeOutValue       = 2;							// Seconds
    spt->DataBufferOffset   = spt->Length;
    spt->SenseInfoOffset    = 0;

    spt->Cdb[0] =  READ10;  // 0x28
    spt->Cdb[1] =  0;
    spt->Cdb[2] = (RD_LBA >> 24) & 255;
    spt->Cdb[3] = (RD_LBA >> 16) & 255;
    spt->Cdb[4] = (RD_LBA >> 8)  & 255;
    spt->Cdb[5] =  RD_LBA        & 255;
    spt->Cdb[6] =  0;
    spt->Cdb[7] = (TXFER_COUNT >> 8) & 255;
    spt->Cdb[8] =  TXFER_COUNT       & 255;
    spt->Cdb[9] =  0;

    DWORD dwBytesReturned=0;

    if ( ! DeviceIoControl(Handle, IOCTL_SCSI_PASS_THROUGH,
                           spt, spt->Length,							// inbuffer,  inBufSize
                           spt, spt->Length+spt->DataTransferLength,	// outbuffer, outBufSize
                           &dwBytesReturned, NULL) )
    {
#ifdef DEBUG_TAB
        genericDebugString = "Failed to read Freerunner data.\nErr= " + QString::number(GetLastError());
        balloonMessage(genericDebugString); Sleep(1000);
#endif
        return false;
    }

    if ( dwBytesReturned < spt->Length )
    {
#ifdef DEBUG_TAB
        genericDebugString = "Error in read Freerunner data.\ndwBytesReturned < spt->Length";
        balloonMessage(genericDebugString); Sleep(1000);
#endif
        return false;
    }

    dwBytesReturned-=sizeof(SCSI_PASS_THROUGH); //spt->Length;
    if ( dwBytesReturned < (DWORD)TXFER_COUNT*SCSI_BLOCKSIZE )
    {
#ifdef DEBUG_TAB
        genericDebugString = "Error in read Freerunner data.\ndwBytesReturned < (DWORD)TXFER_COUNT*SCSI_BLOCKSIZE";
        balloonMessage(genericDebugString); Sleep(1000);
#endif
        return false;
    }

    // Copy out pointer to the returned data.
    *buf = (spt+1);

    return true;
}


void closeHardwareDevice(void)
{
    // Only close it if we managed to open it in the first place...
    if( Handle != NULL )
    {
        CloseHandle( Handle );

        deviceOpened = false;
        Handle       = NULL;
    }
}

bool readSample(cosneta_sample_t *sample)
{
    static int                   sampleNum       = 0;
    static int                   numSamplesAvail = 0;
    static double                lastTime        = 0.0;
    static double                timeStep        = 0.01;
    static bool                  haveLastTime    = false;
    cosneta_payload_t           *payload;
    static int                   numGyro;
    static int                   numAccel1;
    static int                   numQuaternion;
    static int                   numUserInput;
    static unsigned short       *accel1;
    static unsigned short       *accel2;
    static unsigned short       *gyro;
    static cosneta_user_input_t *userInput;
    static int                   currentButtonState = 0;

    if( ! deviceOpened ) return false;

    // No more data available. Read another packet.

    if( sampleNum >= numSamplesAvail )
    {
        LARGE_INTEGER tPerSecond;
        LARGE_INTEGER t;   // A point in time
        double        newTime;

        if( ! LowlevelIO_Read((void **)&payload) ) return false;

        // Get delta time, to get a rate estimate
        QueryPerformanceFrequency(&tPerSecond);
        QueryPerformanceCounter(&t);

        newTime  = (double)t.QuadPart/tPerSecond.QuadPart;
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

        QString debugString = "";

        // and now let's look at the data we received (numbers of each).

        numAccel1     = (int)(payload->accel_1_data_sz) / (sizeof(short)*3);
        numQuaternion = (int)(payload->accel_2_data_sz) / (sizeof(short)*3); // In PoC2, this is an invensense packet
        numGyro       = (int)(payload->gyro_data_sz)    / (sizeof(short)*3);
        numUserInput  = (int)(payload->user_input_sz);

        numSamplesAvail = 0;

        if( (devices_used & DEVICE_ACCEL1) && (devices_used & DEVICE_ACCEL2) )
        {
            if( numAccel1 != numQuaternion )
            {
                debugString  = "readSample: number of accelerometer readings do not \n  match each other: repeating samples!\n";
                debugString += " NumAccel1=" + QString::number(numAccel1) + " NumAccel2=" + QString::number(numQuaternion);
                debugString += " NumGyro=" + QString::number(numGyro);
            }
            numSamplesAvail = (numAccel1 > numQuaternion) ? numAccel1 : numQuaternion;
        }
        else
        {
            numSamplesAvail = (devices_used & DEVICE_ACCEL1) ? numAccel1 : numQuaternion;
        }

        if( (devices_used & (DEVICE_ACCEL1|DEVICE_ACCEL2)) && (devices_used & DEVICE_DIGIGYRO) )
        {
            if( numSamplesAvail != numGyro )
            {
                debugString  = "readSample: number of accelerometer readings do not \n  match num gyro readings: repeating samples!\n";
                debugString += " NumAccel1=" + QString::number(numAccel1) + " NumAccel2=" + QString::number(numQuaternion);
                debugString += " NumGyro=" + QString::number(numGyro);
            }
            numSamplesAvail = numSamplesAvail > numGyro ? numSamplesAvail : numGyro;
        }

        if( numSamplesAvail < 1 )
        {
#ifdef DEBUG_TAB
            debugString = "readSample: No samples read";
            balloonMessage(genericDebugString); Sleep(1000);
#endif
            return false;
        }

        if( ((numAccel1+numQuaternion+numGyro)*3*sizeof(short) + sizeof(cosneta_payload_t)) > RX_DATA_SIZE )
        {
#ifdef DEBUG_TAB
            debugString = "readSample: Bad data sizes: bigger than the read buffer!";
            balloonMessage(genericDebugString); Sleep(1000);
#endif
            return false;
        }

        accel1    = &(payload->data[0]);		// And save some effort on subsequent reads
        accel2    = accel1 + 3*numAccel1;
        gyro      = accel2 + 3*numQuaternion;
        userInput = (cosneta_user_input_t *)(gyro + 3*numGyro);

#ifdef DEBUG_TAB

        if( dumpSamplesToFile )
        {
            // Dump accel1, accel2 & gyro data to different files to allow for different numbers of samples

            dumpSamples(accel1TextOut,newTime,accel1,numAccel1);  // This may stop dumping
            dumpSamples(accel2TextOut,newTime,accel2,numQuaternion);
            dumpSamples(gyroTextOut,  newTime,gyro,  numGyro);
            dumpUserInput(newTime, (unsigned char *)userInput, numUserInput);
        }

        // Debug info
        genericDebugString += "Data: \n";

        if( devices_used & DEVICE_ACCEL1 )
        {
            sample->accel1[0] = accel1[0]; if( sample->accel1[0] >= 32768 ) sample->accel1[0] -= 65536;
            sample->accel1[1] = accel1[1]; if( sample->accel1[1] >= 32768 ) sample->accel1[1] -= 65536;
            sample->accel1[2] = accel1[2]; if( sample->accel1[2] >= 32768 ) sample->accel1[2] -= 65536;

            debugString += "Accel1: rate=" + QString::number(timeStep/numAccel1) + " = [";
            debugString += QString::number(sample->accel1[0]) + ", ";
            debugString += QString::number(sample->accel1[1]) + ", ";
            debugString += QString::number(sample->accel1[2]) + "]\n";
        }

        if( devices_used & DEVICE_ACCEL2 )
        {
            sample->accel2[0] = accel2[0]; if( sample->accel2[0] >= 32768 ) sample->accel2[0] -= 65536;
            sample->accel2[1] = accel2[1]; if( sample->accel2[1] >= 32768 ) sample->accel2[1] -= 65536;
            sample->accel2[2] = accel2[2]; if( sample->accel2[2] >= 32768 ) sample->accel2[2] -= 65536;

            debugString += "Accel2: rate=" + QString::number(timeStep/numQuaternion) + " = [";
            debugString += QString::number(sample->accel2[0]) + ", ";
            debugString += QString::number(sample->accel2[1]) + ", ";
            debugString += QString::number(sample->accel2[2]) + "]\n";
        }

        if( devices_used & DEVICE_DIGIGYRO )
        {
            sample->angle_rate[0] = gyro[0]; if( sample->angle_rate[0] >= 32768 ) sample->angle_rate[0] -= 65536;
            sample->angle_rate[1] = gyro[1]; if( sample->angle_rate[1] >= 32768 ) sample->angle_rate[1] -= 65536;
            sample->angle_rate[2] = gyro[2]; if( sample->angle_rate[2] >= 32768 ) sample->angle_rate[2] -= 65536;

            debugString += "gyro: rate=" + QString::number(timeStep/numGyro) + " = [";
            debugString += QString::number(sample->angle_rate[0]) + ", ";
            debugString += QString::number(sample->angle_rate[1]) + ", ";
            debugString += QString::number(sample->angle_rate[2]) + "]\n";
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

    // Extract the next sample and return it to the caller

    int index;

    if( devices_used & DEVICE_ACCEL1 )
    {
        index = 3 * numAccel1 * sampleNum / numSamplesAvail;
        sample->accel1[0] = accel1[index + 0]; if( sample->accel1[0] >= 32768 ) sample->accel1[0] -= 65536;
        sample->accel1[1] = accel1[index + 1]; if( sample->accel1[1] >= 32768 ) sample->accel1[1] -= 65536;
        sample->accel1[2] = accel1[index + 2]; if( sample->accel1[2] >= 32768 ) sample->accel1[2] -= 65536;
    }

    if( devices_used & DEVICE_ACCEL2 )
    {
        index = 3*((numQuaternion * sampleNum) / numSamplesAvail);
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

    if( devices_used & DEVICE_QUARTRNIONS )
    {
        ;
    }

    // Calculate the individual device time steps...
    if( devices_used & DEVICE_DIGIGYRO ) sample->timeStep_angle_rate = timeStep/numSamplesAvail;
    if( devices_used & DEVICE_ACCEL1 )   sample->timeStep_accel1     = timeStep/numSamplesAvail;
    if( devices_used & DEVICE_ACCEL2 )   sample->timeStep_accel2     = timeStep/numSamplesAvail;

    sample->buttons = currentButtonState;    // Messages for user inputs are documented as TBD

    sampleNum ++;

    return true;
}


#endif


#endif // POC1_READ_SUPPORT
