
#include <stdint.h>	/* For uint types */

/* x86 is little endian, and the device spec claims that data is */
/* little endian.                                                */

#define ACT_SENSOR_ACCEL1        0x0001
#define ACT_SENSOR_ACCEL2        0x0002
#define ACT_SENSOR_ANALOGUE_GYRO 0x0004
#define ACT_SENSOR_DIGITAL_GYRO  0x0008
#define ACT_SENSOR_TEMPERATURE   0x0010
#define ACT_SENSOR_IMAGE         0x0020

/* Image Sensor Format: Bitmask (0x0002) values */

#define IMAGE_FMT_YCbCr  0x0001
#define IMAGE_FMT_RGB565 0x0002
#define IMAGE_FMT_RGB888 0x0004
#define IMAGE_FMT_JPEG   0x0008

#ifdef _WIN32
typedef struct cosneta_configure_s
#else
typedef struct __attribute__ ((__packed__)) cosneta_configure_s
#endif
{
    int32_t  blocks_per_packet;		/* 1-64 (8): Low for good latency, High for max throughput. Trade-off is around 4 or 8 blocks.	*/
    int32_t  active_sensors;            /* Bitmask (&3B): 0x0001 = Accel 1		0x0002 = Accel2					*/
					/*		  0x0004 = Analogue Gyro	0x0008 = Digital Gyro				*/
					/* 		  0x0010 = Temp Sensor		0x0020 = Image Sensor				*/
    int32_t  accel_samp_rate;		/* Accelerometer sample rate: 1KHz is device maximum                                            */
    int32_t  gyro_samp_rate;		/* Gyro sample arte: 800Hz is device maximum							*/
    int32_t  image_sensor_reduce;	/* 1/N [n=1..8]. Sensor only captures every Nth frame						*/
    int32_t  image_proc_reduce;		/* 1/N [n=1..8]. Processor only captures every Nth frame					*/
					/* Default reduction is 60/(8*8) = 0.9375 Hz							*/
    int32_t  image_format;		/* Bitmask: 1 => YCbCr; 2 => RGB 5:6:5; 4 => RGB 8:8:8  [Default = 2]				*/
    int32_t  reserved[121];		/* Words 8 -> 128 (total size = 512 bytes)							*/

} cosneta_configure_t;

/* The unsigned 16 bit samples from the accelerometers will be as follows	*/
/* u16S1_A1_X-Axis; u16S1_A1_Y-Axis, u16S1_A1_Z-Axis;				*/
/* u16S2_A1_X-Axis; u16S2_A1_Y-Axis, u16S2_A1_Z-Axis;				*/
/* u16S3_A1_X-Axis; u16S3_A1_Y-Axis, u16S3_A1_Z-Axis;				*/

/* unsigned 16 bit Samples from the single Gyro are as follows	*/
/* u16S1_X-Axis, u16S1_Y-Axis, u16S1_Z-Axis			*/
/* u16S2_X-Axis; u16S2_Y-Axis, u16S2_Z-Axis			*/
/* u16S2_X-Axis; u16S2_Y-Axis, u16S2_Z-Axis			*/

#ifdef _WIN32
typedef struct user_input_entry_s
#else
typedef struct __attribute__ ((__packed__)) user_input_entry_s
#endif
{
    int32_t  timestamp;		/* in ms	*/
    int32_t  event_code;		/* TDB		*/

} user_input_entry_t;

/* The image frame is transferred in RGB 5:6:5 format as follows		*/
/*        D7-----------------------------------------------------------------------D0	*/
/* Byte 0 R4(i+0) R3(i+0) R2(i+0) R1(i+0)   R0(i+0) G5(i+0) G4(i+0) G3(i+0)	*/
/* Byte 1 G2(i+0) G1(i+0) G0(i+0) B4(i+0)   B3(i+0) B2(i+0) B1(i+0) B0(i+0)	*/

/* Byte 2 R4(i+1) R3(i+1) R2(i+1) R1(i+1)   R0(i+1) G5(i+1) G4(i+1) G3(i+1)	*/
/* Byte 3 G2(i+1) G1(i+1) G0(i+1) B4(i+1)   B3(i+1) B2(i+1) B1(i+1) B0(i+1)	*/

#define BLUE8_FROM_RGB565_U8(i)     ((i)[1] & 0xF8)				// Good, Maybe a little green added

//#define GREEN8_FROM_RGB565_U8(i) ( (((i)[1] & 0x07)<<5) | (((i)[0] & 0xE0)>>3))	// 
//#define GREEN8_FROM_RGB565_U8(i) (((i)[0] & 0xE0))	// Obviously not the top bits
//#define GREEN8_FROM_RGB565_U8(i) (((i)[1] & 0x03)<<6)	// No red, no blue, a bit noisey green though [maybe a little red)
#define GREEN8_FROM_RGB565_U8(i) (((i)[1] & 0x03)<<6)|(((i)[0] & 0xE0)>>2)	// Okay-ish, maybe a little bit of red

#define RED8_FROM_RGB565_U8(i)                            (((i)[0] & 0x1F)<<3)	// Good, maybe a very little green
//#define RED8_FROM_RGB565_U8(i)   0
//#define GREEN8_FROM_RGB565_U8(i) 0
//#define BLUE8_FROM_RGB565_U8(i)  0


#define RED8_FROM_RGB565_U16(i)   (((i) & 0xF800)>>8) 
#define GREEN8_FROM_RGB565_U16(i) (((i) & 0x07E0)>>3)
#define BLUE8_FROM_RGB565_U16(i)  (((i) & 0x001F)<<3) 

#define RED8_FROM_RGB888_U8(i)    ((i)[0])
#define GREEN8_FROM_RGB888_U8(i)  ((i)[1])
#define BLUE8_FROM_RGB888_U8(i)   ((i)[2])

//#define RED8_FROM_RGB888_U8(i)    0
//#define GREEN8_FROM_RGB888_U8(i)  0
//#define BLUE8_FROM_RGB888_U8(i)   0

#define STATUS_StartOfImageFrame 1

#ifdef _WIN32
typedef struct cosneta_payload_s
#else
typedef struct __attribute__ ((__packed__)) cosneta_payload_s
#endif
{
    int32_t  status;		/* Bit 0: 1 on start of image frame. 0 on subsequent payloads (or no image samples present).	*/
    int32_t  timestamp;		/* Timestamp in ms (wraps over at 0xFFFFFFFF).							*/
    int32_t  accel_1_data_sz;	/* Number of accelerometer 1 data bytes.							*/
    int32_t  accel_2_data_sz;	/* Number of accelerometer 2 data bytes.							*/
    int32_t  gyro_data_sz;	/* Number of gyro data bytes.									*/
    int32_t  user_input_sz;	/* Number of bytes of user input (button press) data.						*/
    int32_t  image_data_sz;	/* Number of image bytes.									*/
    int32_t  temperature;	/* Format TBD											*/
    uint16_t data[2];		/* This is: accel_1[3], accel_2[3], gyro[3], button press, image and then padding.		*/

} cosneta_payload_t;

/* For user input */
#define COS_BUTTON_1 (1<<0)
#define COS_BUTTON_2 (1<<1)
#define COS_BUTTON_3 (1<<2)
#define COS_BUTTON_4 (1<<3)
#define COS_BUTTON_5 (1<<4)
#define COS_BUTTON_6 (1<<5)

typedef struct cosneta_user_input_s
{
    uint32_t  event_timestamp;
    uint32_t  new_button_state;

} cosneta_user_input_t;

// SCSI write 10
//bit→
//↓byte 	7 	6 	5 	4 	3 	2 	1 	0
//0 	Operation code = 2Ah
//1 	WRPROTECT 	DPO 	FUA 	Reserved 	FUA_NV 	Obsolete
//2–5 	LBA
//6 	Reserved 	Group Number
//7–8 	Transfer length
//9 	Control

// SCSI read 10
//	bit→
//	↓byte 	7 	6 	5 	4 	3 	2 	1 	0
//	0 	Operation code = 28h
//	1 	LUN [3]			DPO [1]	FUA [1]	Reserved [2]	RelAdr [1]
//	2–5 	LBA
//	6 	Reserved
//	7–8 	Transfer length
//	9 	Control
