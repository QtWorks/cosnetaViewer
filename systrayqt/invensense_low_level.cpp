
#include "invensense_low_level.h"
#include "main.h"

#ifdef DEBUG_TAB
#include <QFile>
#include <QDir>
#include <QTextStream>

#include <iostream>

#define APPENDTOLOG(x) if( invensenseLogging ) appendToLog((x))

static QFile       logFile;
static QTextStream logStream;
static bool        stopLogging       = false;   // Make sure it's the same thread that stops the file writes.
static bool        invensenseLogging = false;


bool startInvensenseLogging(void)
{
    if( invensenseLogging ) return true;

    std::cerr << "startInvensenseLogging()" << std::endl;

    // Select a directory
    QDir dir;
    dir.setCurrent(QDir::homePath());

    // Make sure that the file classes are not in use

    if( logFile.isOpen() )    logFile.close();

    // Create and open the files

    logFile.setFileName("invensense.txt");

    if( ! logFile.open(QIODevice::WriteOnly | QIODevice::Text) )
    {
        balloonMessage("Failed to open invensense log file.");
        return false;
    }

    logStream.setDevice(&logFile);

    stopLogging       = false;
    invensenseLogging = true;

    return true;
}

void stopInvensenseLogging(void)
{
    if( ! invensenseLogging ) return;

    std::cerr << "stopInvensenseLogging()" << std::endl;

    stopLogging = true;
}

bool appendToLog(QString text)
{
    if( ! invensenseLogging ) return false;

    logStream << text;

    if( stopLogging )
    {
        logFile.flush();
        logFile.close();

        invensenseLogging = false;
        stopLogging       = false;
    }

    return true;
}

#else

#define APPENDTOLOG(x)

#endif

#if defined(_WIN32)

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

static HANDLE hDevice = INVALID_HANDLE_VALUE;

bool invensenseOpenDevice(int portNumber)
{
    WCHAR comStr[32];

    swprintf_s(comStr,sizeof(comStr)/sizeof(WCHAR),L"\\\\.\\COM%d", portNumber );

    hDevice = CreateFile(comStr, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if( hDevice == INVALID_HANDLE_VALUE )
    {
        APPENDTOLOG("Failed to open COM port.\n");

        return false;
    }

    DCB PortDCB;

    PortDCB.DCBlength = sizeof(DCB);

    // Get the default port setting information.
    GetCommState(hDevice, &PortDCB);

    // Change the DCB structure settings.

    PortDCB.BaudRate     = 115200; // Current baud
    PortDCB.fBinary      = TRUE; // Binary mode; no EOF check
    PortDCB.fParity      = FALSE; // Enable parity checking
    PortDCB.fOutxCtsFlow = FALSE; // No CTS output flow control
    PortDCB.fOutxDsrFlow = FALSE; // No DSR output flow control
    PortDCB.fDtrControl  = DTR_CONTROL_DISABLE;
    // DTR flow control type
    PortDCB.fDsrSensitivity   = FALSE; // DSR sensitivity
    PortDCB.fTXContinueOnXoff = TRUE; // XOFF continues Tx
    PortDCB.fOutX       = FALSE; // No XON/XOFF out flow control
    PortDCB.fInX        = FALSE; // No XON/XOFF in flow control
    PortDCB.fErrorChar  = FALSE; // Disable error replacement
    PortDCB.fNull       = FALSE; // Disable null stripping
    PortDCB.fRtsControl = RTS_CONTROL_DISABLE;
    // RTS flow control
    PortDCB.fAbortOnError = FALSE; // Do not abort reads/writes on
    // error
    PortDCB.ByteSize = 8; // Number of bits/byte, 4-8
    PortDCB.Parity   = NOPARITY; // 0-4=no,odd,even,mark,space
    PortDCB.StopBits = ONESTOPBIT; // 0,1,2 = 1, 1.5, 2

    // Configure the port according to the specifications of the DCB
    // structure.
    if( FALSE == SetCommState(hDevice, &PortDCB) )
    {
        CloseHandle( hDevice );
        hDevice = INVALID_HANDLE_VALUE;

        APPENDTOLOG("Failed to SetCommState (configure) COM port.\n");

        return false;
    }

    COMMTIMEOUTS CommTimeouts;

    // Retrieve the time-out parameters for all read and write operations
    // on the port.
    GetCommTimeouts(hDevice, &CommTimeouts);

    // Change the COMMTIMEOUTS structure settings.
    CommTimeouts.ReadIntervalTimeout         = 0; // MAXDWORD;
    CommTimeouts.ReadTotalTimeoutMultiplier  = 0;
    CommTimeouts.ReadTotalTimeoutConstant    = 0;
    CommTimeouts.WriteTotalTimeoutMultiplier = 10;
    CommTimeouts.WriteTotalTimeoutConstant   = 1000;

    // Set the time-out parameters for all read and write operations
    // on the port.
    if( FALSE == SetCommTimeouts(hDevice, &CommTimeouts) )
    {
        CloseHandle( hDevice );
        hDevice = INVALID_HANDLE_VALUE;

        APPENDTOLOG("Failed to SetCommTimeouts COM port.\n");

        return false;
    }

    return true;
}

void invensenseCloseDevice()
{
    if( hDevice == INVALID_HANDLE_VALUE) return;

    CloseHandle( hDevice );
    hDevice = INVALID_HANDLE_VALUE;
}

static inline bool serialReadChar(unsigned char *out)
{
    DWORD numberOfBytesRead;

    if( ! ReadFile(hDevice, out, 1, &numberOfBytesRead, NULL ) ) return false;

    if( numberOfBytesRead != 1 ) return false;

    return true;
}

#elif defined(__gnu_linux__)

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static int invensense_fd = -1;

static int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;

    if( invensense_fd < 0) return -1;

    memset (&tty, 0, sizeof tty);
    if (tcgetattr (invensense_fd, &tty) != 0)
    {
        balloonMessage(QString("error %s from tcgetattr").arg(strerror(errno)));
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (invensense_fd, TCSANOW, &tty) != 0)
    {
        balloonMessage(QString("error %s from tcsetattr").arg(strerror(errno)));
        return -1;
    }
    return 0;
}

static void set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (invensense_fd, &tty) != 0)
    {
        balloonMessage(QString("error %s from tcgetattr").arg(strerror(errno)));
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (invensense_fd, TCSANOW, &tty) != 0)
        balloonMessage(QString("error %s setting term attributes").arg(strerror(errno)));
}

bool invensenseOpenDevice(int portNumber)
{
    char portName[32];

    sprintf(portName, "/dev/ttyrfcomm%d", portNumber);

    invensense_fd = open (portName, O_RDWR | O_NOCTTY | O_SYNC);
    if (invensense_fd < 0)
    {
//        balloonMessage(QString("error opening /dev/ttyACM0: %s").arg( strerror(errno)));
        return false;
    }

    set_interface_attribs (invensense_fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (invensense_fd, 0);                    // set no blocking
}

void invensenseCloseDevice()
{
    if( invensense_fd < 0) return;

    close( invensense_fd );
}

static bool inline serialReadChar(unsigned char *out)
{
    if( invensense_fd < 0) return false;

    int n = read (invensense_fd, out, 1);

    return (n == 1);
}


#elif  defined(__APPLE__) && defined(__MACH__)

bool invensenseOpenDevice(int portNumber)
{
}

void invensenseCloseDevice()
{
}

static inline bool serialReadChar(unsigned char *out)
{
}


#endif

#define SIO_RD_BUFF_SIZE 20

// Serial packet data byte index (by inspection of UC3)
// [0] = '$'
// [1] = 1: debug packet
//       2: quaternion packet
//       3: calibration request
//       4: calibration start
//       5: calibration data
//       6: calibration end
// [2] = packet count

// For quaternion data (from UC3 & teapot too):
// [ 3-6]:  quat[0] (*1.0/(1<<30))
// [ 7-10]: quat[1] (*1.0/(1<<30))
// [11-14]: quat[2] (*1.0/(1<<30))
// [15-18]: quat[3] (*1.0/(1<<30))

bool invensenseReadSample(invensense_sample_t *sample)
{
    static unsigned char rdBuf[SIO_RD_BUFF_SIZE];

    // Read a packet
    unsigned char head;
    unsigned char type = 0;

    while( type != 2 )
    {
        if( ! serialReadChar( &head ) ) return false;

        APPENDTOLOG( QString("HEADER: %1").arg(int(head)) );

        for(int i=0;i<(3+SIO_RD_BUFF_SIZE) && head != '$'; i++)
        {
            serialReadChar( &head );

            APPENDTOLOG( QString("%1,").arg(int(head)) );
        }

        APPENDTOLOG("\n");

        switch(head)
        {
        case '$': break;

        case 'E': balloonMessage("Error reported by invensense board.");
                  APPENDTOLOG("Error reported by invensense board.");
                  return false;
        default:  balloonMessage("Unexpected char (on $ or E) from invensense board.");
                  APPENDTOLOG("Unexpected char (on $ or E) from invensense board.");
                  return false;
        }

        // We have a $

        if( head == '$' && ! serialReadChar( &type ) ) return false;

        APPENDTOLOG(QString("TYPE: %1\n").arg(int(type)));
    };

    // We have a $2
    unsigned char count;

    if( ! serialReadChar( &count ) ) return false;

    APPENDTOLOG(QString("COUNT: %1\n").arg(int(type)));

    // Read the quaternion data
    for(int i=0; i<SIO_RD_BUFF_SIZE; i++)
    {
        if( ! serialReadChar( rdBuf+i ) ) return false;
    }

    // Decode the data packet
    double q;

    APPENDTOLOG("Quat:");
    for(int i=0; i<4; i++)
    {
        q = rdBuf[i*4 + 0]*(1<<24) + rdBuf[i*4 + 1]*(1<<16) + rdBuf[i*4 + 2]*(1<<8) + rdBuf[i*4 + 3];
        if( q>2147483648 ) q -= 4294967296;

        sample->quaternion[i] = q*1.0/(1<<30);

        APPENDTOLOG(QString("%1%2").arg( sample->quaternion[i] ).arg( (i<3?", ":"\n")) );
    }
    // Quaternions match console output from teapot demo.

    // And done
    return true;
}
