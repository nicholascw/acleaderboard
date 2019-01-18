#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>

typedef struct handshaker_ {
    int identifier;
    int version;
    int operationId;
} handshaker_t;

typedef struct handshakerResponse_ {
    char carName[50];
    char driverName[50];
    int identifier;
    int version;
    char trackName[50];
    char trackConfig[50];
} h_response_t;

const unsigned char ACSP_NEW_SESSION = 50;
const unsigned char ACSP_NEW_CONNECTION = 51;
const unsigned char ACSP_CONNECTION_CLOSED = 52;
const unsigned char ACSP_CAR_UPDATE = 53;
const unsigned char ACSP_CAR_INFO = 54; // Sent as response to ACSP_GET_CAR_INFO command
const unsigned char ACSP_END_SESSION = 55;
const unsigned char ACSP_LAP_COMPLETED = 73;
const unsigned char ACSP_VERSION = 56;
const unsigned char ACSP_CHAT = 57;
const unsigned char ACSP_CLIENT_LOADED = 58;
const unsigned char ACSP_SESSION_INFO = 59;
const unsigned char ACSP_ERROR = 60;

// EVENTS
const unsigned char ACSP_CLIENT_EVENT = 130;

// EVENT TYPES
const unsigned char ACSP_CE_COLLISION_WITH_CAR = 10;
const unsigned char ACSP_CE_COLLISION_WITH_ENV = 11;

// COMMANDS
const unsigned char ACSP_REALTIMEPOS_INTERVAL = 200;
const unsigned char ACSP_GET_CAR_INFO = 201;
const unsigned char ACSP_SEND_CHAT = 202; // Sends chat to one car
const unsigned char ACSP_BROADCAST_CHAT = 203; // Sends chat to everybody
const unsigned char ACSP_GET_SESSION_INFO = 204;
const unsigned char ACSP_SET_SESSION_INFO = 205;
const unsigned char ACSP_KICK_USER = 206;
const unsigned char ACSP_NEXT_SESSION = 207;
const unsigned char ACSP_RESTART_SESSION = 208;
const unsigned char ACSP_ADMIN_COMMAND = 209; // Send message plus a stringW with the command

typedef struct Vector3f {
    float x, y, z;
} v3f_t;

