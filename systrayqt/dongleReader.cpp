#include "build_options.h"

#include "dongleReader.h"
#include "dongleData.h"
#include "main.h"

#ifdef DEBUG_TAB

QString dongleDebugString = "Unset";   // Used in debug displays.

#endif

#include <QFile>
#include <QDir>
#include <QDebug>

#define SCSI_BLOCKSIZE 512

dongleReader::dongleReader(sysTray_devices_state_t *table, QObject *parent) : QObject(parent)
{
    sharedTable = table;
    quit        = false;

    guiPtr      = NULL;

    nativeOpen  = false;
//    dongleFile.close();
}

dongleReader::~dongleReader()
{
}

void dongleReader::setGuiPointer(CosnetaGui *update)
{
    guiPtr = update;

    guiPtr->setFlipPresent(nativeOpen);
}


void dongleReader::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(readThread()) );
}

#ifndef NEED_SCSI_IOCTL

bool dongleReader::readSensorDataFS(char *buffer, int maxLen)
{
    int                    numRead;

    if( ! dongleFile.isOpen() )
    {
        QString                path;
        QDir                   dir;

        // Look for the device
        path = getDonglePath() + QString("/COSNETA/DEV/");
        if( path.length()>=2 && dir.setCurrent(path) )
        {
            dongleFile.setFileName("SENSORS.DAT");

            if( ! dongleFile.open( QIODevice::ReadWrite|QIODevice::Unbuffered) )
            {
                return false;
            }

            // May need some magic with dongleFile.handle()
        }
        else
        {
            return false;
        }
    }

    // Let's keep reading the file over and over again...
    numRead = dongleFile.read(buffer, maxLen);

    if( numRead<=0 )
    {
        // Something wrong. Close file
        dongleFile.close();
        return false;
    }

    // Remove this when we get data from the dongle as it should force a wait
    msleep(25);

    // And rewind so we re-read the same 512 bytes. Qt help states: reset() -
    // Seeks to the start of input for random-access devices. Returns true on success
    if( ! dongleFile.seek(0) )
    {
        dongleFile.close();
    }
    else
    {
        dongleFile.write("Reset",5);
        dongleFile.seek(0);
        dongleFile.flush();
    }

#ifdef DEBUG_TAB
    dongleDebugString = QString("Dongle read: ") +QString::number(numRead) + QString(" bytes");
#endif

    return true;
}


bool dongleReader::readSettingsDataFS(int penNum, pen_settings_t *settings)
{
    QString                path;
    QDir                   dir;
    QFile                  settingsFile;
    int                    numRead;

    // Look for the device
    path = getDonglePath() + QString("/COSNETA/DEV/");
    if( path.length()>=2 && dir.setCurrent(path) )
    {
        QString fname = QString("SETTINGS%1").arg(penNum);

        settingsFile.setFileName(fname);

        if( ! settingsFile.open( QIODevice::ReadWrite|QIODevice::Unbuffered) )
        {
            return false;
        }

        // May need some magic with dongleFile.handle() for windows
    }
    else
    {
        return false;
    }

    // Let's read the file...
    numRead = settingsFile.read((char *)settings, sizeof(pen_settings_t));

    // Close the file
    settingsFile.close();

    return ( numRead == sizeof(pen_settings_t) );
}
#endif



// Read from the cosneta device, and populate the shared
// table appropriately.
void dongleReader::readThread()
{
    char                   buffer[512];
    usb_sensor_data_set_t *sensor_data = (usb_sensor_data_set_t *)buffer;
    int                    sensor_pen;
    QDesktopWidget         desktop;
    int                    srch;
    quint32                action;
    int                    keypress;
    bool                   sendShift;
    bool                   sendControl;
#ifdef DEBUG_TAB
    QString                debugString;
#endif

    for(sensor_pen=0; sensor_pen<MAX_PENS; sensor_pen++) pen_was_present[sensor_pen] = false;

#ifdef DEBUG_TAB
    dongleDebugString = "FlipStick not opened.";
#endif

    while( ! quit )
    {
        // Read the data
#ifdef NEED_SCSI_IOCTL
        if( ! readSensorDataIoctl(buffer, sizeof(buffer)) )
#else
        if( ! readSensorDataFS(buffer, sizeof(buffer)) )
#endif
        {
#ifdef DEBUG_TAB
             dongleDebugString = "Flip not opened.";
#endif
            // Wait before we try again
             msleep(500);
        }
        else
        {
#ifdef DEBUG_TAB
            debugString = "";
#endif
            // Have a new set of data
            for(sensor_pen=0; sensor_pen<MAX_PENS; sensor_pen++)
            {
#ifdef DEBUG_TAB
                if( sensor_data->pen_present[sensor_pen] )
                {
//                    debugString += "P ";
                    debugString += QString("P[%1 %2] ")
                            .arg((int)(sensor_data->pen_sensors[sensor_pen].buttons[0]))
                            .arg((int)(sensor_data->pen_sensors[sensor_pen].buttons[1]));
                }
                else
                    debugString += ". ";
#endif
                // Check for joining/leaving pens
                if( sensor_data->pen_present[sensor_pen] && ! pen_was_present[sensor_pen] )
                {
                    sharedTable->addRemoveDeviceMutex->lock();

                    for(srch=0; srch<MAX_PENS; srch++)
                    {
                        if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[srch]) )
                        {
                            COS_PEN_SET_ALL_FLAGS(sharedTable->pen[srch],COS_PEN_ACTIVE);
                            sharedTable->locCalc[srch]->updateScreenGeometry(&desktop);
                            break;
                        }
                    }

                    if( srch >= MAX_PENS ) // Failed to find a slot in the shareTable for the new pen
                    {
#ifdef DEBUG_TAB
                        debugString += ":No free slots for new pen:";
#endif
                        pen_was_present[sensor_pen] = false;
                    }
                    else
                    {
                        // Retreive the current settings for the new pen
#ifdef NEED_SCSI_IOCTL
                        if( readSettingsNativeData(sensor_pen, &(sharedTable->settings[srch]) ) )
#else
                        if( readSettingsDataFS(pen, &(sharedTable->settings[srch]) ) )
#endif
                        {
                            // And set up the table
                            pen_map[sensor_pen]         = srch;
                            pen_was_present[sensor_pen] = true;

                            balloonMessage( QString("New pen: %1")
                                            .arg(sharedTable->settings[srch].users_name) );

                            // Only do this after we have read all the data
                            COS_PEN_SET_ALL_FLAGS(sharedTable->pen[srch],COS_PEN_ACTIVE);
                            sharedTable->pen[srch].buttons = 0;

                            // Tell sppComms to copy out the settings
                            sharedTable->settings_changed_by_systray[srch] = COS_TRUE;

                            // And kick the GUI so it can update the displayed pen settings
                            if( guiPtr != NULL )
                            {
                                guiPtr->penChanged(sensor_pen);
                            }
                        }
                        else
                        {
#ifdef DEBUG_TAB
                            debugString += QString("Failed to read pen %1's settings.")
                                           .arg(srch);
#endif
                            pen_was_present[sensor_pen]           = false;
                            COS_PEN_SET_ALL_FLAGS(sharedTable->pen[srch],COS_PEN_INACTIVE);
                        }
                    }

                    sharedTable->addRemoveDeviceMutex->unlock();
                }
                else if( ! sensor_data->pen_present[sensor_pen] && pen_was_present[sensor_pen] )
                {
                    // Pen has gone away. No lock required to drop a pen
                    balloonMessage( QString("Removed pen: %1")
                                    .arg(sharedTable->settings[pen_map[sensor_pen]].users_name) );

                    // Update the table data
                    COS_PEN_SET_ALL_FLAGS(sharedTable->pen[pen_map[sensor_pen]],COS_PEN_INACTIVE);

                    pen_was_present[sensor_pen] = false;

                    // And kick the GUI so it can update the displayed pen settings
                    if( guiPtr != NULL )
                    {
                        guiPtr->penChanged(sensor_pen);
                    }
                }

                // Update data for active pens
                if( pen_was_present[sensor_pen] )
                {
                    int shrd_tbl_pen = pen_map[sensor_pen];

                    sharedTable->locCalc[shrd_tbl_pen]->update( &(sensor_data->pen_sensors[sensor_pen]) );
#ifdef DEBUG_TAB
                    debugString += QString("Pen %1->%2 name='%3' Buttons=0x%4")
                                          .arg(sensor_pen)
                                          .arg(shrd_tbl_pen)
                                          .arg(sharedTable->settings[shrd_tbl_pen].users_name)
                                          .arg(((int)(sharedTable->pen[shrd_tbl_pen].buttons)),0,16);
#endif
                    // We have a new sample: call code to manage gestures and 3D cursor position

                    sharedTable->gestCalc[shrd_tbl_pen].update( &(sharedTable->pen[shrd_tbl_pen]) );

                    pen_modes_t oldMode = sharedTable->penMode[shrd_tbl_pen];
                    sharedTable->penMode[shrd_tbl_pen] = sharedTable->gestCalc[shrd_tbl_pen].mouseModeSelected();
                    if( guiPtr != NULL  && oldMode != sharedTable->penMode[shrd_tbl_pen] )
                    {
                        guiPtr->penChanged(shrd_tbl_pen);
                    }

                    if( sharedTable->gestCalc[shrd_tbl_pen].gestureGetKeypress(&keypress, &sendShift, &sendControl) )
                    {
                        // File this away in the table

                        sharedTable->gesture_key[shrd_tbl_pen]      = keypress;
                        sharedTable->gesture_key_mods[shrd_tbl_pen] =
                                                (sendShift   ? KEY_MOD_SHIFT : 0 ) |
                                                (sendControl ? KEY_MOD_CTRL  : 0 )  ;
                    }
                    if( sharedTable->gestCalc[shrd_tbl_pen].getPresentationAction(&action) )
                    {
                        // File this away in the table

                        sharedTable->pen[shrd_tbl_pen].app_ctrl_action = action;
                        sharedTable->pen[shrd_tbl_pen].app_ctrl_sequence_num ++;
                    }
                } // pen_was_present (i.e. currently have a pen at this slot)
            } // for each pen
#ifdef DEBUG_TAB
            dongleDebugString = "Dongle data: " + debugString;
#endif
        } // got data
    }
}


void dongleReader::stopCommand(void)
{
    quit = true;
}


void dongleReader::startCommand(void)
{
}


void dongleReader::waitForStop(void)
{
}

#ifdef NEED_SCSI_IOCTL

#define TXFER_COUNT         1

#define LOGICAL_OFFSET       ((32761*1+32+63)-2)
#define DEV_BLK_LBA           (LOGICAL_OFFSET+3)
#define GROUP_SENSOR_BLK     (6+LOGICAL_OFFSET)
#define PEN_0_SETTINGS_BLOCK (14+LOGICAL_OFFSET)

#ifdef Q_OS_LINUX


#define INQ_REPLY_LEN 96
#define INQ_CMD_CODE  0x12
#define INQ_CMD_LEN   6

#define LB_SIZE       512


#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <time.h>
#include <string.h>     // For strerror()
#include <errno.h>
#include <unistd.h>

static int     cosneta_fd = -1;


void printScsiSenseState(void)
{
    unsigned char sense_buffer[32];
    sg_io_hdr_t   io_hdr;
    unsigned char inqCmdBlk[INQ_CMD_LEN] = {INQ_CMD_CODE, 0, 0, 0, INQ_REPLY_LEN, 0};
    unsigned char inqBuff[INQ_REPLY_LEN];
    int           k;

    if( cosneta_fd < 0 )
    {
       qDebug() << "printScsiSenseState: file descriptor not open!";
       return;
    }

    /* Prepare INQUIRY command */
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len      = sizeof(inqCmdBlk);
    /* io_hdr.iovec_count = 0; */  /* memset takes care of this */
    io_hdr.mx_sb_len    = sizeof(sense_buffer);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len    = INQ_REPLY_LEN;
    io_hdr.dxferp       = inqBuff;
    io_hdr.cmdp         = inqCmdBlk;
    io_hdr.sbp          = sense_buffer;
    io_hdr.timeout      = 20000;     /* 20000 millisecs == 20 seconds */

    if (ioctl(cosneta_fd, SG_IO, &io_hdr) < 0)
    {
        qDebug() << "printScsiSenseState0: Inquiry SG_IO ioctl error:" << strerror(errno);
        return;
    }

    if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK)
    {
        if (io_hdr.sb_len_wr > 0)
        {
            printf("INQUIRY sense data: ");
            for (k = 0; k < io_hdr.sb_len_wr; ++k)
            {
                if ((k > 0) && (0 == (k % 10)))
                    printf("\n  ");
                printf("0x%02x ", sense_buffer[k]);
            }
            printf("\n");
        }
        if (io_hdr.masked_status) printf("INQUIRY SCSI status=0x%x\n", io_hdr.status);
        if (io_hdr.host_status)   printf("INQUIRY host_status=0x%x\n", io_hdr.host_status);
        if (io_hdr.driver_status) printf("INQUIRY driver_status=0x%x\n", io_hdr.driver_status);
    }
    else   /* assume INQUIRY response is present */
    {
        char * p = (char *)inqBuff;
        printf("Some of the INQUIRY command's response:\n");
        printf("    %.8s  %.16s  %.4s\n", p + 8, p + 16, p + 32);
        printf("INQUIRY duration=%u millisecs, resid=%d\n",io_hdr.duration, io_hdr.resid);
    }
}

bool dongleReader::readSensorNativeOpen(void)
{
    char               *dev_path = "/dev/sde";

//    qDebug() << "TODO: select a Flip device in /dev based on manufacturer number and product ID.";

    if( cosneta_fd >= 0 )
    {
        close(cosneta_fd);
    }

    cosneta_fd = open(dev_path, O_RDWR);

    // Open the device for use
    if( cosneta_fd < 0 )
    {
        return false;
    }

    qDebug() << "Donglereader device opened. ";

    return true;
}

bool dongleReader::readDebugRead(char *buf, int maxLen)
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
                                              0,                               // Group number
                                              (TXFER_COUNT >> 8) & 255,
                                               TXFER_COUNT       & 255,	       // Length (read 1 * LS_SIZE(512) bytes = 512)
                                              0                                // Control
                                            };
    if( cosneta_fd < 0 )
    {
        if( ! readSensorNativeOpen() )
        {
            qDebug() << "LowlevelWirelessDev_Read: exit as failed to open file descriptor!";
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
        qDebug() << "readSample: exit as ioctl failed on read! Ret = " << ret <<
                    "readThread: (errno=" << errno << ":" << strerror(errno) << ")";
        close(cosneta_fd);
        return false;
    }
    if( io.masked_status != 0 ) /* ie not GOOD */
    {
//        printf("readSample: masked_status=");
        switch( io.masked_status )
        {
        case 0x01: qDebug() << "CHECK_CONDITION";	   break;
        case 0x02: qDebug() << "CONDITION_GOOD";	   break;
        case 0x04: qDebug() << "BUSY";                 break;
        case 0x08: qDebug() << "INTERMEDIATE_GOOD";    break;
        case 0x0a: qDebug() << "INTERMEDIATE_C_GOOD";  break;
        case 0x0c: qDebug() << "RESERVATION_CONFLICT"; break;
        case 0x11: qDebug() << "COMMAND_TERMINATED";   break;

        default:   qDebug() << "Unknown error num";
        }

        //printf("\nSCSI sense state: ");
        printScsiSenseState();

        return false;
    }

    // Make sure it's NULL terminated to be safe.
    buf[maxLen-1] = (char)0;

    return true;
}


bool dongleReader::readSensorNativeRead(char *buf, int maxLen)
{
    int                    ret;
    sg_io_hdr_t            io;
    unsigned char          sense_b[SG_MAX_SENSE];		// Status info from ioctl (old driver max size, hopefully okay)
    unsigned char          scsiRead10[10] = {
                                              READ_10,              // Command
                                              0,                     // Flags: WRPROTECT [3], DPO [1], FUA [1], Reserved [1], FUA_NV [1], Obsolete [1]
                                              (GROUP_SENSOR_BLK >> 24) & 255,  // Block address on SCSI "drive"
                                              (GROUP_SENSOR_BLK >> 16) & 255,
                                              (GROUP_SENSOR_BLK >> 8)  & 255,
                                               GROUP_SENSOR_BLK        & 255,
                                              0,                               // Group number
                                              (TXFER_COUNT >> 8) & 255,
                                               TXFER_COUNT       & 255,	       // Length (read 1 * LS_SIZE(512) bytes = 512)
                                              0                                // Control
                                            };
    if( cosneta_fd < 0 )
    {
        if( ! readSensorNativeOpen() )
        {
            qDebug() << "LowlevelWirelessDev_Read: exit as failed to open file descriptor!";
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
        qDebug() << "readSample: exit as ioctl failed on read! Ret = " << ret <<
                    "readThread: (errno=" << errno << ":" << strerror(errno) << ")";
        close(cosneta_fd);
        return false;
    }
    if( io.masked_status != 0 ) /* ie not GOOD */
    {
//        printf("readSample: masked_status=");
        switch( io.masked_status )
        {
        case 0x01: qDebug() << "CHECK_CONDITION";	   break;
        case 0x02: qDebug() << "CONDITION_GOOD";	   break;
        case 0x04: qDebug() << "BUSY";                 break;
        case 0x08: qDebug() << "INTERMEDIATE_GOOD";    break;
        case 0x0a: qDebug() << "INTERMEDIATE_C_GOOD";  break;
        case 0x0c: qDebug() << "RESERVATION_CONFLICT"; break;
        case 0x11: qDebug() << "COMMAND_TERMINATED";   break;

        default:   qDebug() << "Unknown error num";
        }

        //printf("\nSCSI sense state: ");
        printScsiSenseState();

        return false;
    }

    return true;
}

bool dongleReader::readSettingsNativeData(int penNum, pen_settings_t *settings)
{
#warning "TODO: write readSettingsNativeData() for Linux & OSX"
    return false;
}


#elif defined(Q_OS_MAC)


bool dongleReader::readSensorNativeOpen(void)
{
#warning "TODO: write readSensorNativeOpen() for Linux & OSX"
    return false;
}


bool dongleReader::readSensorNativeRead(char *buf, int maxLen)
{
#warning "TODO: write readSensorNativeRead() for Linux & OSX"
    return false;
}


bool dongleReader::readSettingsNativeData(int penNum, pen_settings_t *settings)
{
#warning "TODO: write readSensorNativeRead() for Linux & OSX"
    return false;
}

#elif defined(Q_OS_WIN)


QString nativeSensorsDebugString;


// /////////////////////////////////////////////////////////////////////////////////
// TODO: Hope to dump the Ioctl based native code and replace with filesystem access
// /////////////////////////////////////////////////////////////////////////////////

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "Setupapi.h"
#include "WinIoctl.h"
#include "Ntddscsi.h"


// #ifdef MANUF_CODE
// else
#include "cfgmgr32.h"

/* SCSI data */
#define READ10  0x28
#define WRITE10 0x2A

// #define FAT12
#define FAT32

#define RX_DATA_BUFFER_SIZE (sizeof(SCSI_PASS_THROUGH) + SCSI_BLOCKSIZE * TXFER_COUNT)

#define IFACE_DETAIL_SZ     ((4+1024)*sizeof(WCHAR))

bool dongleReader::readSensorNativeOpen(void)
{
#ifdef __GNUC__
    return false;
#else
    HDEVINFO		                 hDevInfo;
    SP_DEVINFO_DATA	                 devInfoData;
    WCHAR                            deviceIDStr[MAX_DEVICE_ID_LEN];
    SP_DEVICE_INTERFACE_DATA         devIfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA *devIfaceDetailData_p;

    devIfaceDetailData_p=(SP_DEVICE_INTERFACE_DETAIL_DATA *)calloc(1,IFACE_DETAIL_SZ);
    if (devIfaceDetailData_p == NULL)
    {
        return false;
    }

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
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
                DEVINST parentDevInst;

                if( CR_SUCCESS == CM_Get_Parent(&parentDevInst, devInfoData.DevInst, 0) )
                {
                    WCHAR    parentIDStr[MAX_DEVICE_ID_LEN];
                    if( CR_SUCCESS == CM_Get_Device_ID(parentDevInst, parentIDStr, MAX_DEVICE_ID_LEN, 0) )
                    {
                        // Device 612A is a Flip, and 612B is a FreeRunner
                        if( wcsstr(parentIDStr, FREERUNNER_PID) )
                        {
#if 0
                            qDebug() << "Device" <<  QString::fromWCharArray(parentIDStr) << "is a Freerunner.";
#endif
                        }
                        else if( wcsstr(parentIDStr, FLIP_PID)
#if 1
                                 || wcsstr(parentIDStr, POC2_PID) )  // Allow compatibility with PoC2 hubs.
#else
                                 }                                   // Production code
#endif
                        {
                            qDebug() << "Flip found.";

                            /* Device found with correct manufacturer and product IDs */
                            devIfaceData.cbSize          = sizeof(devIfaceData);
                            devIfaceDetailData_p->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                            if ( SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, idx, &devIfaceData) )
                            {
                                if ( SetupDiGetDeviceInterfaceDetail(hDevInfo, &devIfaceData,
                                                                     devIfaceDetailData_p, IFACE_DETAIL_SZ,
                                                                     NULL, NULL )                          )
                                {
                                    qDebug() << "Hardware ID:" << QString::fromWCharArray(devIfaceDetailData_p->DevicePath);

                                    /* Open the device and leave the handle in the global "Handle" */
                                    HANDLE hDev = CreateFile(devIfaceDetailData_p->DevicePath,
                                                             GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

                                    if( hDev != INVALID_HANDLE_VALUE )
                                    {
                                        // EXIT here on success!!!
                                        Handle = hDev;

                                        SetupDiDestroyDeviceInfoList(hDevInfo);
                                        free(devIfaceDetailData_p);

                                        return true;        // ********** EXIT HERE!!!! **********
                                    }
#ifdef DEBUG_TAB
                                    else
                                    {
                                        DWORD err=GetLastError();
                                        nativeSensorsDebugString = "Failed to CreateFile for flipStick device.\nErr(5=> access denied): " + QString::number(err);
                                        balloonMessage(nativeSensorsDebugString); Sleep(1000);
                                    }
#endif
                                }
#ifdef DEBUG_TAB
                                else
                                {
                                    nativeSensorsDebugString = "Failed to extract flipStick file path.\nFailed SetupDiGetDeviceInterfaceDetail()\nErr= " + QString::number(GetLastError());
                                    balloonMessage(nativeSensorsDebugString); Sleep(1000);
                                }
#endif
                            }
#ifdef DEBUG_TAB
                            else
                            {
                                nativeSensorsDebugString = "Failed to extract flipStick file path.\nFailed SetupDiEnumDeviceInterfaces()\nErr= " + QString::number(GetLastError());
                                balloonMessage(nativeSensorsDebugString); Sleep(1000);
                            }
#endif
                            //	Failed, but keep iteterating in case there is a similar device that does work
                        }
                        else
                        {
                            qDebug() << "Unknown Cosneta device detected:" << QString::fromWCharArray( parentIDStr );
                        }
                    }
                }
                else qDebug() << "Failed to CM_Get_Parent()";
            }
#if 0
            else
            {
                qDebug() << "Bad disk path:" << QString::fromWCharArray(deviceIDStr);
            }
#endif
        }

        /* And next in list of devices */
        devInfoData.cbSize = sizeof(devInfoData);
        idx ++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    free(devIfaceDetailData_p);

    return false;
#endif
}





bool dongleReader::readDebugRead(char *buf, int maxLen)
{
    static unsigned char      dataBuffer[RX_DATA_BUFFER_SIZE] = { 0 };
    static SCSI_PASS_THROUGH *spt = (SCSI_PASS_THROUGH *)dataBuffer;

    if( ! nativeOpen )
    {
        nativeSensorsDebugString += "Attempted to read when device file was not open.";
        return false;
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
        nativeSensorsDebugString += buf;
        return false;
    }

    if ( dwBytesReturned < spt->Length )
    {
        sprintf(buf,"Error in read Freerunner data.\ndwBytesReturned(%d) < spt->Length (%d)",
                    dwBytesReturned, spt->Length);
        nativeSensorsDebugString += buf;
        return false;
    }

    // Copy out pointer to the returned data.
    memcpy(buf,&(dataBuffer[sizeof(SCSI_PASS_THROUGH)]),maxLen);
    buf[maxLen-1] = (char)0; // Just to be sure

    return true;
}

bool dongleReader::readSensorNativeRead(char *buf, int maxLen)
{
    static unsigned char      dataBuffer[RX_DATA_BUFFER_SIZE] = { 0 };
    static SCSI_PASS_THROUGH *spt = (SCSI_PASS_THROUGH *)dataBuffer;

    if( ! nativeOpen )
    {
        nativeSensorsDebugString += "Attempted to read when device file was not open.";
        qDebug() << nativeSensorsDebugString;
        return false;
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
    spt->Cdb[2] = (GROUP_SENSOR_BLK >> 24) & 255;
    spt->Cdb[3] = (GROUP_SENSOR_BLK >> 16) & 255;
    spt->Cdb[4] = (GROUP_SENSOR_BLK >> 8)  & 255;
    spt->Cdb[5] =  GROUP_SENSOR_BLK        & 255;
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
        nativeSensorsDebugString += buf;
        qDebug() << nativeSensorsDebugString;
        return false;
    }

    if ( dwBytesReturned < spt->Length )
    {
        sprintf(buf,"Error in read Freerunner data.\ndwBytesReturned(%d) < spt->Length (%d)",
                    dwBytesReturned, spt->Length);
        nativeSensorsDebugString += buf;
        qDebug() << nativeSensorsDebugString;
        return false;
    }

    // Copy out pointer to the returned data.
    memcpy(buf,&(dataBuffer[sizeof(SCSI_PASS_THROUGH)]),maxLen);
    buf[maxLen-1] = (char)0; // Just to be sure

    return true;
}



bool dongleReader::readSettingsNativeData(int penNum, pen_settings_t *settings)
{
    static unsigned char      dataBuffer[RX_DATA_BUFFER_SIZE] = { 0 };
    static SCSI_PASS_THROUGH *spt = (SCSI_PASS_THROUGH *)dataBuffer;

    if( ! nativeOpen )
    {
        nativeSensorsDebugString += "Attempted to read settings when device file was not open.";
        return false;
    }
    if( penNum<0 || penNum>=MAX_PENS)
    {
        nativeSensorsDebugString += QString("Bad penNum (%1) for settings read.").arg(penNum);
        return false;
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
    spt->Cdb[2] = ((PEN_0_SETTINGS_BLOCK+penNum) >> 24) & 255;
    spt->Cdb[3] = ((PEN_0_SETTINGS_BLOCK+penNum) >> 16) & 255;
    spt->Cdb[4] = ((PEN_0_SETTINGS_BLOCK+penNum) >> 8)  & 255;
    spt->Cdb[5] =  (PEN_0_SETTINGS_BLOCK+penNum)        & 255;
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
        nativeSensorsDebugString += QString("Failed to read pen settings data. Error=%1")
                                      .arg(GetLastError());
        return false;
    }

    if ( dwBytesReturned < spt->Length )
    {
        nativeSensorsDebugString += QString("Error in read of pen settings. dwBytesReturned(%1) < spt->Length (%2)")
                                      .arg(dwBytesReturned).arg(spt->Length);
        return false;
    }

    // Copy out pointer to the returned data.
    memcpy(settings,&(dataBuffer[sizeof(SCSI_PASS_THROUGH)]),sizeof(pen_settings_t));

    return true;
}


#endif

#endif

bool dongleReader::readSensorDataIoctl(char *buffer, int maxLen)
{
    if( ! nativeOpen )
    {
        if( ! readSensorNativeOpen() )
        {
         #ifdef DEBUG_TAB
            dongleDebugString = "Failed to open dongle.";

            qDebug() << dongleDebugString;
#endif
            return false;
        }
    }

    if( ! nativeOpen )
    {
        balloonMessage(tr("Flip detected"));
        if( guiPtr ) guiPtr->setFlipPresent(true);

        nativeOpen = true;
    }

    if( ! readSensorNativeRead(buffer, maxLen) )
    {
#if defined(Q_OS_WIN)

        CloseHandle( Handle );

#elif defined(Q_OS_LINUX)

        // close(fd);
#endif
        balloonMessage(tr("Flip removed"));
#ifdef DEBUG_TAB
        dongleDebugString = "Failed to read data from opened dongle device.";
#endif
        if( guiPtr ) guiPtr->setFlipPresent(false);

        nativeOpen = false;
#ifdef DEBUG_TAB
        qDebug() << dongleDebugString;
#endif
        // Remove all the pens
        for(int pen=0; pen<MAX_PENS; pen++)
        {
            pen_was_present[pen] = false;
            COS_PEN_SET_ALL_FLAGS(sharedTable->pen[pen],COS_PEN_INACTIVE);
        }

        msleep(1000);

        return false;
    }

    return true;
}

#ifdef Q_OS_WIN

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#endif

// This is always based on filesystem writes as these are not hidden from
// the hardware by a cache. NB The penNum here is the global number, not
// the donglePen number.

bool dongleReader::updatePenSettings(int penNum,  pen_settings_t *settings)
{
    QFile   settingsFile;
    bool    succeeded = false;
    int     pen;

    // Local pen num
    for(pen=0; pen<MAX_PENS; pen++)
    {
        if( pen_was_present[pen] && pen_map[pen]==penNum )
        {
            break;
        }
    }
    if( pen>=MAX_PENS )
    {
        // This should mean an update for a network pen
        qDebug() << "Update for non-dongle pen" << penNum;

        return false; // Pen not from the dongle, but not a failure
    }

    // NB No need to check penNum as invalid numbers wont have a file present
    QString fname = getCosnetadevicePath() + QString("COSNETA/DEV/SETTING%1.DAT").arg(pen);

    qDebug() << QString("updateSettings to: ") << fname <<
                QString("name '%1', sensitivity: %2")
                        .arg(settings->users_name).arg(settings->sensitivity);

    settingsFile.setFileName(fname);

    if( ! settingsFile.exists() )
    {
        balloonMessage(QString("Failed to update settings: file '%1' does not exist")
                       .arg(fname));
        return false;
    }

#if 0
    fname = QString("E:\\COSNETA\\DEV\\SETTING%1.DAT").arg(penNum);
    HANDLE hDev = CreateFile(fname.toStdWString().data(),
                             GENERIC_READ|GENERIC_WRITE,
                             0, //FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if( hDev == INVALID_HANDLE_VALUE )
    {
        qDebug() << QString("Failed to update settings: failed to open file '%1' for write errno=%2")
                    .arg(fname).arg(GetLastError());

        return false;
    }

    int ret = WriteFile(hDev, settings, sizeof(pen_settings_t), NULL, NULL);
    succeeded = (ret != 0);
    if( ! succeeded )
    {
        qDebug() << "WriteFile() to " << fname << " failed: errno=" << GetLastError()
                 << " WriteFile ret " << ret;
    }
    CloseHandle(hDev);
#else
    if( ! settingsFile.open(QIODevice::ReadWrite|QIODevice::Unbuffered) )
    {
        balloonMessage(QString("Failed to update settings: failed to open file '%1' for write")
                       .arg(fname) );

        return false;
    }

    int numSent = settingsFile.write( ((char *)settings), sizeof(pen_settings_t) );

    if( numSent != sizeof(pen_settings_t) )
    {
        qDebug() << QString("Failed to set settings: file write to '%2' returned %1")
                    .arg(numSent).arg(fname);

        balloonMessage(tr("Failed to update settings."));

        succeeded = false;
    }
    else
    {
        if( ! settingsFile.flush() ) qDebug("Failed to flush file.");

        succeeded = true;
    }
#ifdef Q_OS_WIN
    FlushFileBuffers((HANDLE)settingsFile.handle());
#endif
    settingsFile.close();
#endif
#ifdef Q_OS_WIN
    // Flush to device
    fname = getCosnetadevicePath().replace(2,1,QString("\\"));
    HANDLE hDev = CreateFile(fname.toStdWString().data(),
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if( hDev == INVALID_HANDLE_VALUE )
    {
        qDebug() << QString("Failed to flush settings: failed to open %1 error=%2")
                    .arg(fname).arg(GetLastError());

        return succeeded;
    }

    FlushFileBuffers(hDev);

    CloseHandle(hDev);
#endif
    return succeeded;
}


