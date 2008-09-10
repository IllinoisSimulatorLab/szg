// For Ascension Technology's Bird Net IP protocol.

#define BN_MAX_ADDR        120

/* defines for packet type = command/response codes*/
#define MSG_WAKE_UP                        10
#define RSP_WAKE_UP                        20
#define MSG_SHUT_DOWN                11
#define RSP_SHUT_DOWN                21
#define MSG_GET_STATUS                101
#define MSG_SEND_SETUP                102
#define MSG_SINGLE_SHOT                103
#define MSG_RUN_CONTINUOUS        104
#define MSG_STOP_DATA                105
#define MSG_SEND_DATA                106
#define RSP_GET_STATUS                201
#define RSP_SEND_SETUP                202
#define RSP_SINGLE_SHOT                203
#define RSP_RUN_CONTINUOUS        204
#define RSP_STOP_DATA                205
#define RSP_SEND_DATA                206
#define DATA_PACKET                        210
#define DATA_PACKET_ACK                211
#define RSP_ILLEGAL                        40
#define RSP_UNKNOWN                        50
#define MSG_SYNC_SEQUENCE        30
#define RSP_SYNC_SEQUENCE        31
/* defines for error code*/
#define ESEQUENCE                                1
#define ESEQSOME                                2
#define ESEQLOTS                                3
#define ESEQEQUAL                                4
#define EUNEXPECTED                        5
#define EBADPACKET                        6
#define EILLEGALSTATUS                7
#define EILLEGALSETUP                8
#define ENOTREADY                                9
// define birdnet structures as packed
#ifndef AR_USE_SGI
#pragma pack(1)
#endif

/* define birdnet header */
struct BN_HEADER {
        unsigned short                 sequence;
        unsigned short                 milliseconds;
        unsigned long                  seconds;
        unsigned char                         type;
        unsigned char                         xtype;
        unsigned char                        protocol;
        unsigned char                         errorCode;
        unsigned short          errorCodeExtension;
        unsigned short                 numBytes;
        };

/* birdnet system status */
struct BN_SYSTEM_STATUS {
        unsigned char sys;
        unsigned char error;
        unsigned char naddrs;
        unsigned char nservers;
        unsigned char transmitter;
        char                           rate[6];
        unsigned char chassisID;
        unsigned char chassisDevices;
        unsigned char firstAddress;
        unsigned char softRev[2];
        unsigned char fbbstatus[BN_MAX_ADDR];
        };
/*defines for sys byte*/
#define BN_SYS_RUNNING                0x80
#define BN_SYS_ERROR                        0x40
#define BN_SYS_FBB_ERROR        0x20
#define BN_SYS_LOCAL_ERROR        0x10
#define BN_SYS_LOCAL_POWER        0x08
#define BN_SYS_MASTER                0x04
#define BN_SYS_SYNCTYPE                0x02
#define BN_SYS_CRTSYNC                0x01


/*define for fbbstatus  */
#define BN_FBB_ACCESSABLE        0x80
#define BN_FBB_RUNNING                0x40
#define BN_FBB_RCVR                        0x20
#define BN_FBB_ERC                                0x10
#define BN_FBB_XMT3                        0x08
#define BN_FBB_XMT2                        0x04
#define BN_FBB_XMT1                        0x02
#define BN_FBB_XMT0                        0x01

struct BN_BIRD_HEADER {
        unsigned char                        status;
        unsigned char                        model;
        unsigned short int         softwareRev;
        unsigned char                        errorCode;
        unsigned char                        setup;
        unsigned char                        dataFormat;
        unsigned char                        reportRate;
        unsigned short int        scaling;
        unsigned char                        hemisphere;
        unsigned char                        FBBaddress;
        unsigned short int        spare1;
        unsigned short int        spare2;
        };

/* defines for bird_status.status */
#define BN_FLOCK_ERROR                0x80
#define BN_FLOCK_RUNNING        0x40
#define BN_FLOCK_BUTTONS        0x08
#define BN_FLOCK_RCVR                0x04
#define BN_FLOCK_XMTR                0x02
#define BN_FLOCK_XMTR_ON        0x01
/*defines for bird_status.setup */
#define BN_FLOCK_SUDDEN                        0x20
#define BN_FLOCK_XYZREF                        0x10
#define BN_FLOCK_APPENDBUT                0x08
#define BN_FLOCK_ACN_FILTER        0x04
#define BN_FLOCK_ACW_FILTER        0x02
#define BN_FLOCK_DC_FILTER                0x01
/* defines for model*/
#define BN_MODEL_STANDALONE        1
#define BN_MODEL_ERC                                2
#define BN_MODEL_MSTAR                        3
#define BN_MODEL_PCBIRD                        4
#define BN_MODEL_SPACEPAD                5
#define BN_MODEL_MOTIONSTAR        6
#define BN_MODEL_WIRELESS                 7
/* defines for xmtr type*/
#define BN_XMTR_ERT 0x80
#define BN_XMTR_ACTIVE 0x10
/* defines for hemisphere*/
#define BN_HEM_FRONT                0
#define BN_HEM_REAR     1
#define BN_HEM_UPPER    2
#define BN_HEM_LOWER    3
#define BN_HEM_LEFT     4
#define BN_HEM_RIGHT    5
/* defines for format */
#define BN_NODATA         0
#define BN_POS                         1
#define BN_ANGLES                2
#define BN_MATRIX                3
#define BN_POSANG                4
#define BN_POSMAT                5
#define BN_QUAT                7
#define BN_POSQUAT        8
#define BN_THRUDATA        14
#define BN_BADDATA        15

struct FILTER_TABLE {
        unsigned short int        entry[7];
        };

struct ANGLES_TABLE {
        unsigned short int        angle[3];
        };

struct BN_BIRD_STATUS {
        struct BN_BIRD_HEADER        header;
        struct FILTER_TABLE                alphaMin;
        struct FILTER_TABLE                alphaMax;
        struct FILTER_TABLE                Vm;
        struct ANGLES_TABLE                xyzReferenceFrame;
        struct ANGLES_TABLE                xyzAngleAlign;
        };

struct BN_DATA {
        char addr;
        char format;
        char data[24];
        char button[2];
        };

/* define for data record format*/
#define BUTTON_FLAG        0x80

#ifndef AR_USE_SGI
#pragma pack()
#endif
