#ifndef UPDATER_H
#define UPDATER_H

#include <QWidget>
#include <QCloseEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCryptographicHash>
// Debug
#include <QTime>



#include "build_options.h"
#define NATIVE_ACCESS   /* To remove */

#ifdef NATIVE_ACCESS

#define LOGICAL_OFFSET             ((32761*1+32+63)-2)
#define UPDATE_BLK                 (4+LOGICAL_OFFSET)

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include "Windows.h"


#include "devioctl.h"
#include "Setupapi.h"
#include "WinIoctl.h"
#include "ntdddisk.h"
#include "Ntddscsi.h"
#include "cfgmgr32.h"

#define _NTSCSI_USER_MODE_
#ifndef __GNUC__
#include "scsi.h"
#endif

#define SPT_SENSE_LEN 32

typedef struct
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG                    filler;
    UCHAR                    ucSenseBuf[SPT_SENSE_LEN];

} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

#endif

#endif

#define UPDATE_BLOCK_SZ 512

// The following structure definitions must be maintained to match
// the Flip firmware


#define OKAY  0
#define BAD   0xFFFFFFFF

typedef struct
{
    qint32  status;
    quint32 latest_flip_firmware_version;
    quint32 latest_pen_firmware_version;
    quint32 latest_windows_software_version;
    quint32 latest_mac_software_version;
    quint32 latest_linux_software_version;

} status_t;

typedef struct
{
    quint32 start_sentinal;

    quint32 flip_firmware_version;
    quint32 pen_firmware_version;
    quint32 windows_software_version;
    quint32 mac_software_version;
    quint32 linux_software_version;

    quint32 flip_firmware_size;	/* bytes */
    quint32 pen_firmware_size;
    quint32 windows_software_size;
    quint32 mac_software_size;
    quint32 linux_software_size;

    quint32 flip_firmware_csum;
    quint32 pen_firmware_csum;
    quint32 windows_software_csum;
    quint32 mac_software_csum;
    quint32 linux_software_csum;

    quint32 end_sentinal;

} update_header_t;

typedef enum
{
    COS_FLIP_DEV, COS_FREERUNNER_DEV, UNKNOWN_DEVICE

} cosneta_device_t;

typedef struct
{
    qint32  status;
    qint32  flip_firmware_update_succeeded;
    qint32  pen_firmware_update_succeeded;
    qint32  windows_software_update_succeeded;
    qint32  mac_software_update_succeeded;
    qint32  linux_software_update_succeeded;

} update_result_t;

/* These are block numbers relative to the update file start. */

#define MAX_FIRMWARE_SIZE               (512*1024)
#define MAX_HOST_SOFTWARE_SIZE          (30*1024*1024)

#define FLIP_FIRMWARE_START_BLOCK       0
#define FREERUNNER_FIRMWARE_START_BLOCK (MAX_FIRMWARE_SIZE/UPDATE_BLOCK_SZ)
#define WINDOWS_SOFTWARE_START_BLOCK    (FREERUNNER_FIRMWARE_START_BLOCK+(MAX_FIRMWARE_SIZE/UPDATE_BLOCK_SZ))
#define MAC_SOFTWARE_START_BLOCK        (WINDOWS_SOFTWARE_START_BLOCK + (MAX_HOST_SOFTWARE_SIZE/UPDATE_BLOCK_SZ))
#define LINUX_SOFTWARE_START_BLOCK      (MAC_SOFTWARE_START_BLOCK + (MAX_HOST_SOFTWARE_SIZE/UPDATE_BLOCK_SZ))

#define FLIP_FIRMWARE_START_ADDR       (FLIP_FIRMWARE_START_BLOCK*UPDATE_BLOCK_SZ)
#define FREERUNNER_FIRMWARE_START_ADDR (FREERUNNER_FIRMWARE_START_BLOCK*UPDATE_BLOCK_SZ)
#define WINDOWS_SOFTWARE_START_ADDR    (WINDOWS_SOFTWARE_START_BLOCK*UPDATE_BLOCK_SZ)
#define MAC_SOFTWARE_START_ADDR        (MAC_SOFTWARE_START_BLOCK*UPDATE_BLOCK_SZ)
#define LINUX_SOFTWARE_START_ADDR      (LINUX_SOFTWARE_START_BLOCK*UPDATE_BLOCK_SZ)


/* We are arm, but PC/MAC/Linux hosts are intel, so an endianness swap is required */
#define SWAP32(x) ( (((x)&0xFF)<<24) | (((x)&0xFF00)<<8) | (((x)&0xFF0000)>>8) | (((x)&0xFF000000)>>24) )



class updater : public QWidget
{
    Q_OBJECT
public:
    explicit updater(QWidget *parent = 0);
    void     startUpdateDialogueIfValid(QString metaData);

public slots:

    void     startSoftwareUpdate(void);
    void     abortUpdate(void);
    void     downloadFinished(QNetworkReply *reply);
    void     downloadDataAvailable();
    void     extractFile(int fileNameLen, uchar *data, int dataLen);

private:

    QPushButton  *yesDoUpdate;
    QPushButton  *noUpdate;
    QProgressBar *downloadProgress;

    QNetworkAccessManager *downloadManager;
    QNetworkReply         *downloadRequestReply;
    bool                   downloadInProgress;
    QFile                 *downloadBlobFile;
    QByteArray             downloadData;
    QCryptographicHash    *downloadHash;

    int           totalBytesDownloaded;
    quint32       updateTimeStamp;
    int           updateFileLength;
    QString       updateShaString;
};

// The following code is the firmwareware update system. This shall be
// re-introduced later (like when we have hardware).
#if 0

class updater : public QWidget
{
    Q_OBJECT
public:
    explicit updater(QWidget *parent = 0);
    ~updater(void);

    void closeEvent(QCloseEvent *e);
    void startUpdate(void);

signals:

public slots:

    void    checkForNewSoftware(void);
    void    applyNewSoftwareUpdate(void);
#ifdef DEBUG_TAB
    void    resetEmbeddedSoftwareTable(void);
    void    downloadUpdatedBootloader(void);
#endif

private:
    // Methods
    bool    updaterOpenDevice(cosneta_device_t devType);
    void    updaterCloseDevice(void);
    bool    readUpdateBlock(char *buffer);
//  bool    writeUpdateBlock(char *sendData, int length);
    bool    updaterFSOpenDevice(void);
    void    updaterFSCloseDevice(void);
    bool    FSwriteUpdateBlock(char *sendData, int length);
    bool    retreiveLatestServerCodeVersions(update_header_t *serverVersions);
    int     trimServerVersionsToUpdates(update_header_t *serverVersions, status_t *status);
    bool    retrieveServerUpdateFiles(update_header_t *serverVersions);
    void    applyUpdatesToDevice(update_header_t *serverVersions);
    bool    sendSingleFileToUpdateFile(QString fileName, int bytesToSend);
    quint32 computeCheckValue(QString filePath);

    // Data
    QGroupBox        *updateBox;
    QLabel           *initUpdateLbl;
    QLabel           *fetchUpdateLbl;
    QLabel           *applyUpdateLbl;
    QProgressBar     *progress;
    QPushButton      *check;
    QPushButton      *doUpdate;
    QPushButton      *closeButton;
#ifdef DEBUG_TAB
    QPushButton      *resetSoftwareButton;
    QPushButton      *updateBootloaderButton;
#endif
    int               updateBytesDone;
    cosneta_device_t  deviceType;

    bool              updatesFound;
    quint32           currentFlipFirmwareVersion;
    quint32           currentPenFirmwareVersion;   // This is the version stored on flip, used to update pens
    quint32           currentFlipApplicationSoftwareVersion;
    int               updateSize;

#ifdef NATIVE_ACCESS
#ifdef Q_OS_WIN
    HANDLE         Handle;
#elif defined(Q_OS_LINUX)
    int            cosneta_fd;
#endif
#endif
    QFile          updateFile;

    // Debug
    QTime          timer;
};

#endif
// End of hardware update code




#endif // UPDATER_H
