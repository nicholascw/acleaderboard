#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libmng_types.h>
#include <signal.h>
#include <wchar.h>

#include "ac_proto.h"

static char sigint_captured = 0;

void sigint_handler() {
    sigint_captured = 1;
    exit(1);
}

v3f_t *recv_v3f(int *sockfd, struct sockaddr *addr) {
    v3f_t *res = malloc(sizeof(v3f_t));
    if(!res) return NULL;
    recv(*sockfd, &(res->x), sizeof(float), MSG_WAITALL);
    recv(*sockfd, &(res->y), sizeof(float), MSG_WAITALL);
    recv(*sockfd, &(res->z), sizeof(float), MSG_WAITALL);
    return res;
}

wchar_t *recvStringW(const int *sockfd, struct sockaddr *addr) {
    unsigned char *buf = malloc(1);
    struct sockaddr now_addr;
    socklen_t this_addr_len;
    ssize_t len = recvfrom(*sockfd, buf, 1, 0, &now_addr, &this_addr_len);
    if(len != 1) {
        perror("what?");
        return NULL;
    }
    //if(strcmp(now_addr.sa_data, addr->sa_data) != 0)
    //  perror("msg is from a instance that differ from the packet_id instance!\n");
    wchar_t *msg = malloc(((unsigned) *buf + 1) * 4);
    if(!msg) {
        perror("Did somebody eat all my memory?");
        return NULL;
    }
    recvfrom(*sockfd, msg, (unsigned) *buf * 4, MSG_WAITALL, NULL, NULL);
    msg[*buf] = 0;
    return msg;
}

int main() {
    signal(SIGINT, sigint_handler);
    fprintf(stderr,"NW_ACLeaderBoard v0.1\n"
           "A GPLv3-licensed Assetto Corsa Dedicated Server plugin.\n");

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo("localhost", "12000", &hints, &res);
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        perror("bind() failed! check if another instance is already started, or other plugins are occupying port 12000.");
        exit(1);
    }

    struct sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    while(sigint_captured == 0) {
        unsigned char packet_id;
        ssize_t byte_count = recvfrom(sockfd, &packet_id, 1, MSG_WAITALL, &addr, &addr_len);
        if(byte_count != 1) continue;
        switch(packet_id) {
            case ACSP_ERROR: {
                wchar_t *msg = recvStringW(&sockfd, &addr);
                if(msg) fprintf(stderr, "ACSP_ERROR: %ls\n", msg);
                free(msg);
            }
                break;
            case ACSP_CHAT: {
                unsigned char car_id;
                recvfrom(sockfd, &car_id, 1, 0, NULL, NULL);
                wchar_t *msg = recvStringW(&sockfd, &addr);
                if(msg) fprintf(stderr,"CAR %d: %ls\n", (int) car_id, msg);
                free(msg);
            }
                break;
            case ACSP_CLIENT_LOADED: {
                unsigned char car_id;
                recvfrom(sockfd, &car_id, 1, 0, NULL, NULL);
                fprintf(stderr,"CAR %d loaded.\n", (int) car_id);
            }
                break;
            case ACSP_VERSION: {
                unsigned char proto_ver;
                recvfrom(sockfd, &proto_ver, 1, 0, NULL, NULL);
                fprintf(stderr,"protocol version: %d\n", (int) proto_ver);
            }
                break;
            case ACSP_NEW_SESSION:
                fprintf(stderr,"New session started.\n");
                break;
            case ACSP_SESSION_INFO: {
                fprintf(stderr,"Session Info:\n");
                unsigned char version, sess_index, current_session_index, session_count, type, ambient_temp, road_temp;
                uint16_t time, laps, wait_time;
                int32_t elapsed_ms;

                recv(sockfd, &version, sizeof(unsigned char), MSG_WAITALL);
                // UDP Plugin protocol version, in case missed the first ACSP_VERSION msg sent by the server at startup
                recv(sockfd, &sess_index, sizeof(unsigned char), MSG_WAITALL);
                // The index of the session in the message
                recv(sockfd, &current_session_index, sizeof(unsigned char), MSG_WAITALL);
                // The index of the current session in the server
                recv(sockfd, &session_count, sizeof(unsigned char), MSG_WAITALL);
                // The number of sessions in the server
                wchar_t *server_name = recvStringW(&sockfd, &addr);
                wchar_t *track = recvStringW(&sockfd, &addr);
                wchar_t *track_config = recvStringW(&sockfd, &addr);
                wchar_t *name = recvStringW(&sockfd, &addr);
                recv(sockfd, &type, sizeof(unsigned char), MSG_WAITALL);
                recv(sockfd, &session_count, sizeof(unsigned char), MSG_WAITALL);
                // The number of sessions in the server
                recv(sockfd, &time, sizeof(uint16_t), MSG_WAITALL);
                recv(sockfd, &laps, sizeof(uint16_t), MSG_WAITALL);
                recv(sockfd, &wait_time, sizeof(uint16_t), MSG_WAITALL);
                recv(sockfd, &ambient_temp, sizeof(unsigned char), MSG_WAITALL);
                recv(sockfd, &road_temp, sizeof(unsigned char), MSG_WAITALL);
                wchar_t *weather_graphics = recvStringW(&sockfd, &addr);
                recv(sockfd, &elapsed_ms, sizeof(int32_t), MSG_WAITALL);
                // Milliseconds from the start (this might be negative for races with WaitTime)
                fprintf(stderr,"PROTOCOL VERSION: %d\n", version);
                fprintf(stderr,"SESSION INDEX: %d/%d\nCURRENT SESSION:%d\n", sess_index, session_count, current_session_index);
                fprintf(stderr,"SERVER NAME: %ls\n", server_name);
                fprintf(stderr,"TRACK: %ls\n", track);
                fprintf(stderr,"NAME: %ls\n", name);
                fprintf(stderr,"TYPE: %d\n", type);
                fprintf(stderr,"TIME: %d\n", time);
                fprintf(stderr,"LAPS: %d\n", laps);
                fprintf(stderr,"WAIT TIME: %d\n", wait_time);
                fprintf(stderr,"WEATHER: %ls AMBIENT %dC ROAD %dC\n", weather_graphics, ambient_temp, road_temp);
                fprintf(stderr,"ELAPSED: %d\n", elapsed_ms);
                free(server_name);
                free(track);
                free(track_config);
                free(name);
                free(weather_graphics);
            }
                break;
            case ACSP_END_SESSION: {
                fprintf(stderr,"ACSP_END_SESSION\n");
                wchar_t *filename = recvStringW(&sockfd, &addr);
                fprintf(stderr,"REPORT JSON: %ls\n", filename);
                free(filename);
            }
                break;
            case ACSP_CLIENT_EVENT: {
                unsigned char ev_type, car_id, other_car_id = 255;
                float speed;
                recv(sockfd, &ev_type, sizeof(unsigned char), MSG_WAITALL);
                recv(sockfd, &car_id, sizeof(unsigned char), MSG_WAITALL);
                if(ev_type == ACSP_CE_COLLISION_WITH_CAR)
                    recv(sockfd, &other_car_id, sizeof(unsigned char), MSG_WAITALL);
                recv(sockfd, &speed, sizeof(float), MSG_WAITALL);

                v3f_t *world_pos = recv_v3f(&sockfd, &addr);
                v3f_t *rel_pos = recv_v3f(&sockfd, &addr);

                if(ev_type == ACSP_CE_COLLISION_WITH_ENV)
                    fprintf(stderr,"CAR: %d COLLISION WITH ENV:\n\tSPEED: %f\n\tWORLD_POS: (%f,%f,%f)\n\tREL_POS: (%f,%f,%f)\n",
                           car_id, speed, world_pos->x, world_pos->y, world_pos->z, rel_pos->x, rel_pos->y, rel_pos->z);
                else
                    fprintf(stderr,"CAR: %d COLLISION WITH CAR %d:\n\tSPEED: %f\n\tWORLD_POS: (%f,%f,%f)\n\tREL_POS: (%f,%f,%f)\n",
                           car_id, other_car_id, speed, world_pos->x, world_pos->y, world_pos->z, rel_pos->x,
                           rel_pos->y, rel_pos->z);
                free(world_pos);
                free(rel_pos);
            }
                break;
            case ACSP_CAR_INFO: {
                fprintf(stderr,"ACSP_CAR_INFO\n");
                unsigned char car_id, connect_status;
                recv(sockfd, &car_id, sizeof(unsigned char), MSG_WAITALL);
                // The server re-sends the Id so we can handle many requests at the time and still understand who we
                // are talking about
                recv(sockfd, &connect_status, sizeof(unsigned char), MSG_WAITALL);
                wchar_t *model = recvStringW(&sockfd, &addr);
                wchar_t *skin = recvStringW(&sockfd, &addr);
                wchar_t *driver_name = recvStringW(&sockfd, &addr);
                wchar_t *driver_team = recvStringW(&sockfd, &addr);
                wchar_t *driver_guid = recvStringW(&sockfd, &addr);

                fprintf(stderr,"CAR: %d\n\tModel: %ls [%ls]\n\tDriver: %ls\n\tTeam: %ls\n\tGUID: %ls\n\tSTATUS:%s\n",
                       car_id, model, skin, driver_name, driver_team, driver_guid,
                       (connect_status != 0 ? "Connected" : "Empty"));
                free(model);
                free(skin);
                free(driver_name);
                free(driver_team);
                free(driver_guid);
            }
                break;
            case ACSP_CAR_UPDATE: {
                fprintf(stderr,"ACSP_CAR_UPDATE\n");
                unsigned char car_id, gear;
                uint16_t engine_rpm;
                float normalized_spline_pos;
                recv(sockfd, &car_id, sizeof(unsigned char), MSG_WAITALL);
                v3f_t *pos = recv_v3f(&sockfd, &addr);
                v3f_t *velocity = recv_v3f(&sockfd, &addr);
                recv(sockfd, &gear, sizeof(unsigned char), MSG_WAITALL);
                recv(sockfd, &engine_rpm, sizeof(uint16_t), MSG_WAITALL);
                recv(sockfd, &normalized_spline_pos, sizeof(float), MSG_WAITALL);
                fprintf(stderr,"CAR %d:\n\tPos: (%f,%f,%f)\n\tVelocity: (%f,%f,%f)\n\tGear: %d\n\t Rev: %d\n\t NSP: %f\n",
                       car_id, pos->x, pos->y, pos->z, velocity->x, velocity->y, velocity->z, gear, engine_rpm,
                       normalized_spline_pos);
                free(pos);
                free(velocity);
            }
                break;
            case ACSP_NEW_CONNECTION:
            case ACSP_CONNECTION_CLOSED: {
                fprintf(stderr,packet_id == ACSP_CONNECTION_CLOSED ? "ACSP_CONNECTION_CLOSED\n" : "ACSP_NEW_CONNECTION\n");
                wchar_t *driver_name = recvStringW(&sockfd, &addr);
                wchar_t *driver_guid = recvStringW(&sockfd, &addr);
                unsigned char car_id;
                recv(sockfd, &car_id, sizeof(unsigned char), MSG_WAITALL);
                wchar_t *car_model = recvStringW(&sockfd, &addr);
                wchar_t *car_skin = recvStringW(&sockfd, &addr);
                fprintf(stderr,"DRIVER: %ls GUID: %ls\n", driver_name, driver_guid);
                fprintf(stderr,"CAR: %d MODEL: %ls %ls\n", car_id, car_model, car_skin);
                free(driver_guid);
                free(driver_name);
                free(car_model);
                free(car_skin);
            }
                break;
            case ACSP_LAP_COMPLETED: {
                fprintf(stderr,"ACSP_LAP_COMPLETED\n");
                unsigned char car_id, cuts, cars_count;
                uint32_t lap_time;
                recv(sockfd, &car_id, sizeof(unsigned char), MSG_WAITALL);
                recv(sockfd, &lap_time, sizeof(uint32_t), MSG_WAITALL);
                recv(sockfd, &cuts, sizeof(unsigned char), MSG_WAITALL);
                fprintf(stderr,"CAR: %d\n\tLAP: %d\n\tCUT: %d\n", car_id, lap_time, cuts);
                recv(sockfd, &cars_count, sizeof(unsigned char), MSG_WAITALL);

                // LEADERBOARD
                for(unsigned char i = 0; i < cars_count; i++) {
                    unsigned char rcar_id;
                    uint32_t rtime;
                    uint16_t rlaps;
                    recv(sockfd, &rcar_id, sizeof(rcar_id), MSG_WAITALL);
                    recv(sockfd, &rtime, sizeof(rtime), MSG_WAITALL);
                    recv(sockfd, &rlaps, sizeof(rlaps), MSG_WAITALL);
                    fprintf(stderr,"%d: CAR_ID: %d TIME: %d LAPS: %d\n", i, rcar_id, rtime, rlaps);
                }
                float grip_level;
                recv(sockfd, &grip_level, sizeof(grip_level), MSG_WAITALL);
                fprintf(stderr,"GRIP LEVEL: %f\n", grip_level);
            }
                break;
            default:
                continue;
        }
    }
    return 0;
}