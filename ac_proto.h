#define ACSP_NEW_SESSION 50
#define ACSP_NEW_CONNECTION 51
#define ACSP_CONNECTION_CLOSED 52
#define ACSP_CAR_UPDATE 53
#define ACSP_CAR_INFO 54 // Sent as response to ACSP_GET_CAR_INFO command
#define ACSP_END_SESSION 55
#define ACSP_LAP_COMPLETED 73
#define ACSP_VERSION 56
#define ACSP_CHAT 57
#define ACSP_CLIENT_LOADED 58
#define ACSP_SESSION_INFO 59
#define ACSP_ERROR 60

// EVENTS
#define ACSP_CLIENT_EVENT 130

// EVENT TYPES
#define ACSP_CE_COLLISION_WITH_CAR 10
#define ACSP_CE_COLLISION_WITH_ENV 11

// COMMANDS
#define ACSP_REALTIMEPOS_INTERVAL 200
#define ACSP_GET_CAR_INFO 201
#define ACSP_SEND_CHAT 202 // Sends chat to one car
#define ACSP_BROADCAST_CHAT 203 // Sends chat to everybody
#define ACSP_GET_SESSION_INFO 204
#define ACSP_SET_SESSION_INFO 205
#define ACSP_KICK_USER 206
#define ACSP_NEXT_SESSION 207
#define ACSP_RESTART_SESSION 208
#define ACSP_ADMIN_COMMAND 209 // Send message plus a stringW with the command

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

typedef struct Vector3f {
    float x, y, z;
} v3f_t;
