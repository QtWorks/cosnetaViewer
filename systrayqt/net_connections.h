#ifndef NET_CONNECTIONS_H
#define NET_CONNECTIONS_H

// Encryption
#define KEY_LENGTH 16

// Versions
#define PROTOCOL_VERSION 1

// Message types
#define NET_INVALID_PACKET_TYPE     0x00

#define NET_JOIN_REQ_PACKET         0x01
#define NET_JOIN_REQ_PACKET_NO_ECHO 0x04
#define NET_JOIN_ACCEPT             0x02
#define NET_JOIN_REJECT             0x03
#define NET_PEN_LEAVE_PACKET        0x07
#define NET_PEN_LEAVE_ACK           0x08
#define NET_RADAR_PULSE             0x05
#define NET_RADAR_ECHO              0x06
#define NET_DATA_PACKET             0x11
#define PEN_NAME_SZ 64

typedef struct
{
    char          pen_name[PEN_NAME_SZ];
    unsigned char colour[4];
    char          pen_num;

} join_data_t;

#define SESSION_NAME_SZ 64
typedef struct
{
    char          session_name[SESSION_NAME_SZ];
    unsigned char allowed_new_connections[2];

} radar_data_t;

#define BT_LEFT_MS   0x01
#define BT_RIGHT_MS  0x02
#define BT_MIDDLE_MS 0x04

typedef struct
{
    unsigned char x[2];
    unsigned char y[2];
    unsigned char buttons;
    unsigned char app_ctrl_action[4];
    unsigned char app_ctrl_sequence_num[4];

} data_packet_t;

typedef struct
{
    char salt;
    char messageType;
    char protocolVersion;

    union
    {
        join_data_t   join;
        data_packet_t data;
        radar_data_t  radar;

    } msg;

    char messageTypeCheck;

} net_message_t;

#endif // NET_CONNECTIONS_H
