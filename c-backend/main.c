#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <wchar.h>
#include <endian.h>
#include <stdint.h>
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ac_proto.h"

static char sigint_captured = 0;

v3f_t *read_v3f(uint8_t *from) {
    v3f_t *res = malloc(sizeof(v3f_t));
    if(!res) {
        perror("Memory! I want more memory!");
        return NULL;
    }
    memmove(&(res->x), from, sizeof(float));
    memmove(&(res->y), from + sizeof(float), sizeof(float));
    memmove(&(res->z), from + 2 * sizeof(float), sizeof(float));
    return res;
}

wchar_t *readStringW(uint8_t *from) {
    wchar_t *msg = malloc((size_t) (from[0] + 1) * 4);
    if(!msg) {
        perror("Did somebody eat all of my memory?");
        return NULL;
    }
    memmove(msg, from + 1, ((size_t) from[0]) * 4);
    msg[from[0]] = 0; //in case half string.
    return msg;
}

char *readString(uint8_t *from) {

    char *msg = malloc((size_t) (from[0] + 1));
    if(!msg) {
        perror("Did somebody eat all of my memory?");
        return NULL;
    }
    memmove(msg, from + 1, (size_t) from[0]);
    msg[from[0]] = 0; //in case half string.
    return msg;
}

void sigint_handler() {
    sigint_captured = 1;
    exit(1);
}

int conn_udp(int port, char *hostname, struct sockaddr_in *ipaddr) {

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket");
    }
    int optval = 1;
    // Let them reuse
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) port);
    struct hostent *serv = gethostbyname(hostname);
    if(!serv) {
        perror("gethostbyname");
        exit(1);
    }

    memcpy(&addr.sin_addr.s_addr, serv->h_addr, (size_t) serv->h_length);

    if(ipaddr) {
        memcpy(ipaddr, &addr, sizeof(*ipaddr));
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return sockfd;
}

typedef struct payload_ {
    size_t size;
    uint8_t *buf;
} payload_t;

payload_t cmd_set_session_info(uint8_t sess_idx,
                               wchar_t *server_name,
                               uint8_t type,
                               uint32_t laps,
                               uint32_t time_sec,
                               uint32_t wait_time_sec) {
    payload_t ret;
    ret.buf = malloc(1024);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    uint8_t *curr_ptr = ret.buf;
    *curr_ptr++ = ACSP_SET_SESSION_INFO;
    *curr_ptr++ = sess_idx;
    size_t server_len = wcslen(server_name);
    if(server_len > 0xff) {
        free(ret.buf);
        ret.buf = NULL;
        ret.size = 0;
        return ret;
    }
    *curr_ptr++ = (uint8_t) server_len;
    memmove(curr_ptr, server_name, server_len * sizeof(wchar_t));
    curr_ptr += server_len * sizeof(wchar_t);
    *curr_ptr++ = type;
    memmove(curr_ptr, &laps, sizeof(uint32_t));
    curr_ptr += sizeof(uint32_t);
    memmove(curr_ptr, &time_sec, sizeof(uint32_t));
    curr_ptr += sizeof(uint32_t);
    memmove(curr_ptr, &wait_time_sec, sizeof(uint32_t));
    curr_ptr += sizeof(uint32_t);
    ret.size = curr_ptr - ret.buf + 1;
    return ret;
}

payload_t cmd_get_session_info() {
    payload_t ret;
    ret.size = 3;
    ret.buf = malloc(3);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    ret.buf[0] = ACSP_GET_SESSION_INFO;
    ret.buf[1] = 0xff;
    ret.buf[2] = 0xff;
    return ret;
}

payload_t cmd_get_car_info(uint8_t car_id) {
    payload_t ret;
    ret.size = 2;
    ret.buf = malloc(2);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    ret.buf[0] = ACSP_GET_CAR_INFO;
    ret.buf[1] = car_id;
    return ret;
}

payload_t cmd_enable_realtime_report(uint16_t interval_ms) {
    payload_t ret;
    ret.size = 3;
    ret.buf = malloc(3);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    ret.buf[0] = ACSP_REALTIMEPOS_INTERVAL;
    *((uint16_t *) (ret.buf + 1)) = interval_ms;
    return ret;
}

payload_t cmd_send_chat(uint8_t car_id, wchar_t *msg) {
    payload_t ret;
    ret.buf = malloc(1024);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    ret.buf[0] = ACSP_SEND_CHAT;
    ret.buf[1] = car_id;
    size_t msg_len = wcslen(msg);
    if(msg_len > 0xff) {
        free(ret.buf);
        ret.buf = NULL;
        ret.size = 0;
        fprintf(stderr, "Message too long!\n");
        return ret;
    }
    ret.buf[2] = (uint8_t) msg_len;
    memmove(ret.buf + 3, msg, msg_len * sizeof(wchar_t));
    ret.size = 3 + sizeof(wchar_t) * msg_len;
    return ret;
}

payload_t cmd_broadcast_chat(wchar_t *msg) {
    payload_t ret;
    ret.buf = malloc(1024);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    ret.buf[0] = ACSP_BROADCAST_CHAT;
    size_t msg_len = wcslen(msg);
    if(msg_len > 0xff) {
        free(ret.buf);
        ret.buf = NULL;
        ret.size = 0;
        fprintf(stderr, "Message too long!\n");
        return ret;
    }
    ret.buf[1] = (uint8_t) msg_len;
    memmove(ret.buf + 2, msg, msg_len * sizeof(wchar_t));
    ret.size = 2 + sizeof(wchar_t) * msg_len;
    return ret;
}

payload_t cmd_kick(uint8_t user_id) {
    payload_t ret;
    ret.buf = malloc(2);
    if(!ret.buf) {
        ret.buf = NULL;
        ret.size = 0;
        perror("malloc()");
        return ret;
    }
    ret.size = 2;
    ret.buf[0] = ACSP_KICK_USER;
    ret.buf[1] = user_id;
    return ret;
}


int main() {
    signal(SIGINT, sigint_handler);
    fprintf(stderr, "┌──────────────────────────────────────────┐\n"
                    "│ NW_ACLeaderBoard v0.1                    │\n"
                    "├──────────────────────────────────────────┤\n"
                    "│ A GPLv3-licensed Assetto Corsa Dedicated │\n"
                    "│ Server plugin for integrating with a PHP │\n"
                    "│ Web leaderboard.                         │\n"
                    "└──────────────────────────────────────────┘\n\n");
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo("localhost", "12000", &hints, &res);
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int) {1}, sizeof(int));
    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        perror("bind() failed! check if another instance is already started, or other plugins are occupying port 12000");
        exit(1);
    }
    struct sockaddr_in ac_recv_addr; // ac's "LOCAL PORT" to be sent with commands
    int sendfd = conn_udp(11000, "localhost", &ac_recv_addr);
    struct sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    uint8_t *packet = malloc(2048);
    while(sigint_captured == 0) {
        memset(packet, 0, 2048);
        ssize_t p_len = recvfrom(sockfd, packet, 2048, 0, &addr, &addr_len);
        uint8_t *packet_ptr = packet + 1;
        switch(packet[0]) {
            case ACSP_ERROR: {
                wchar_t *msg = readStringW(packet_ptr);
                if(msg) fprintf(stderr, "ACSP_ERROR: %ls\n", msg);
                free(msg);
            }
                break;
            case ACSP_CHAT: {
                uint8_t car_id = *packet_ptr++;
                wchar_t *msg = readStringW(packet_ptr);
                if(msg) fprintf(stderr, "CAR %d: %ls\n", (int) car_id, msg);
                free(msg);
            }
                break;
            case ACSP_CLIENT_LOADED: {
                uint8_t car_id = *packet_ptr;
                fprintf(stderr, "CAR %d loaded.\n", (int) car_id);
            }
                break;
            case ACSP_VERSION: {
                uint8_t proto_ver = *packet_ptr;
                fprintf(stderr, "protocol version: %d\n", proto_ver);
            }
                break;
            case ACSP_NEW_SESSION:
                fprintf(stderr, "New session started.\n");
            case ACSP_SESSION_INFO: {
                fprintf(stderr, "Session Info:\n");
                uint8_t version, sess_index, current_session_index, session_count, type, ambient_temp, road_temp;
                uint16_t time, laps, wait_time;
                int32_t elapsed_ms;

                version = *packet_ptr++;
                // UDP Plugin protocol version, in case missed the first ACSP_VERSION msg sent by the server at startup
                sess_index = *packet_ptr++;
                // The index of the session in the message
                current_session_index = *packet_ptr++;
                // The index of the current session in the server
                session_count = *packet_ptr++;
                // The number of sessions in the server
                fprintf(stderr, "\tProtocol Version: %d\n", version);
                fprintf(stderr, "\tSession Index: %d/%d\n"
                                "\tCurrent Session:%d\n", (sess_index + 1), session_count,
                        current_session_index);

                wchar_t *server_name = readStringW(packet_ptr);
                packet_ptr += (*packet_ptr * 4) + 1;

                char *track = readString(packet_ptr);
                packet_ptr += *packet_ptr + 1;
                char *track_config = readString(packet_ptr);
                packet_ptr += *packet_ptr + 1;
                char *name = readString(packet_ptr);
                packet_ptr += *packet_ptr + 1;

                type = *packet_ptr++;

                memmove(&time, packet_ptr, sizeof(uint16_t));
                packet_ptr += sizeof(uint16_t);
                memmove(&laps, packet_ptr, sizeof(uint16_t));
                packet_ptr += sizeof(uint16_t);
                memmove(&wait_time, packet_ptr, sizeof(uint16_t));
                packet_ptr += sizeof(uint16_t);

                ambient_temp = *packet_ptr++;
                road_temp = *packet_ptr++;
                char *weather_graphics = readString(packet_ptr);
                packet_ptr += *packet_ptr + 1;

                memmove(&elapsed_ms, packet_ptr, sizeof(int32_t));
                packet_ptr += sizeof(int32_t);
                // Milliseconds from the start (this might be negative for races with WaitTime)

                fprintf(stderr, "SERVER NAME: %ls\n", server_name);
                fprintf(stderr, "TRACK: %ls\n", track);
                fprintf(stderr, "NAME: %ls\n", name);
                fprintf(stderr, "TYPE: %d\n", type);
                fprintf(stderr, "TIME: %d\n", time);
                fprintf(stderr, "LAPS: %d\n", laps);
                fprintf(stderr, "WAIT TIME: %d\n", wait_time);
                fprintf(stderr, "WEATHER: %s AMBIENT %dC ROAD %dC\n", weather_graphics, ambient_temp, road_temp);
                fprintf(stderr, "ELAPSED: %d\n", elapsed_ms);
                free(server_name);
                free(track);
                free(track_config);
                free(name);
                free(weather_graphics);
            }
                break;
            case ACSP_END_SESSION: {
                fprintf(stderr, "ACSP_END_SESSION\n");
                wchar_t *filename = readStringW(packet_ptr);
                fprintf(stderr, "REPORT JSON: %ls\n", filename);
                free(filename);
            }
                break;
            case ACSP_CLIENT_EVENT: {
                uint8_t ev_type, car_id, other_car_id = 255;
                float speed;
                ev_type = *packet_ptr++;
                car_id = *packet_ptr++;
                if(ev_type == ACSP_CE_COLLISION_WITH_CAR) other_car_id = *packet_ptr++;

                memmove(&speed, packet_ptr, sizeof(float));
                packet_ptr += sizeof(float);

                v3f_t *world_pos = read_v3f(packet_ptr);
                packet_ptr += 3 * sizeof(float);
                v3f_t *rel_pos = read_v3f(packet_ptr);
                // packet_ptr += 3 * sizeof(float);

                if(ev_type == ACSP_CE_COLLISION_WITH_ENV)
                    fprintf(stderr,
                            "CAR %d collides with environment:\n"
                            "\tSpeed: %f\n"
                            "\tWorld Pos: (%f,%f,%f)\n"
                            "\tRelative Pos: (%f,%f,%f)\n",
                            car_id, speed, world_pos->x, world_pos->y, world_pos->z, rel_pos->x, rel_pos->y,
                            rel_pos->z);
                else
                    fprintf(stderr,
                            "CAR %d collides with CAR %d:\n"
                            "\tSpeed: %f\n"
                            "\tWorld Pos: (%f,%f,%f)\n"
                            "\tRelative Pos: (%f,%f,%f)\n",
                            car_id, other_car_id, speed, world_pos->x, world_pos->y, world_pos->z, rel_pos->x,
                            rel_pos->y, rel_pos->z);
                free(world_pos);
                free(rel_pos);
            }
                break;
            case ACSP_CAR_INFO: {
                fprintf(stderr, "ACSP_CAR_INFO\n");
                uint8_t car_id, connect_status;
                car_id = *packet_ptr++;
                // The server re-sends the Id so we can handle many requests at the time and still understand who we
                // are talking about
                connect_status = *packet_ptr++;

                wchar_t *model = readStringW(packet_ptr);
                packet_ptr += (*packet_ptr * 4) + 1;
                wchar_t *skin = readStringW(packet_ptr);
                packet_ptr += (*packet_ptr * 4) + 1;
                wchar_t *driver_name = readStringW(packet_ptr);
                packet_ptr += (*packet_ptr * 4) + 1;
                wchar_t *driver_team = readStringW(packet_ptr);
                packet_ptr += (*packet_ptr * 4) + 1;
                wchar_t *driver_guid = readStringW(packet_ptr);
                // packet_ptr += (*packet_ptr * 4) + 1;

                fprintf(stderr,
                        "CAR %d:\n\tModel: %ls [%ls]\n\tDriver: %ls\n\tTeam: %ls\n\tGUID: %ls\n\tSTATUS:%s\n",
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
                fprintf(stderr, "ACSP_CAR_UPDATE\n");
                uint8_t car_id, gear;
                uint16_t engine_rpm;
                float normalized_spline_pos;
                car_id = *packet_ptr++;
                v3f_t *pos = read_v3f(packet_ptr);
                packet_ptr += 3 * sizeof(float);
                v3f_t *velocity = read_v3f(packet_ptr);
                packet_ptr += 3 * sizeof(float);
                gear = *packet_ptr++;
                memmove(&engine_rpm, packet_ptr, sizeof(uint16_t));
                packet_ptr += sizeof(uint16_t);
                memmove(&normalized_spline_pos, packet_ptr, sizeof(float));
                fprintf(stderr,
                        "CAR %d:\n\tPos: (%f,%f,%f)\n\tVelocity: (%f,%f,%f)\n\tGear: %d\n\t Rev: %d\n\t NSP: %f\n",
                        car_id, pos->x, pos->y, pos->z, velocity->x, velocity->y, velocity->z, gear, engine_rpm,
                        normalized_spline_pos);
                free(pos);
                free(velocity);
            }
                break;
            case ACSP_NEW_CONNECTION:
            case ACSP_CONNECTION_CLOSED: {
                fprintf(stderr,
                        packet[0] == ACSP_CONNECTION_CLOSED ? "ACSP_CONNECTION_CLOSED\n" : "ACSP_NEW_CONNECTION\n");
                wchar_t *driver_name = readStringW(packet_ptr);
                packet_ptr += *packet_ptr * 4 + 1;
                wchar_t *driver_guid = readStringW(packet_ptr);
                packet_ptr += *packet_ptr * 4 + 1;
                uint8_t car_id = *packet_ptr++;
                char *car_model = readString(packet_ptr);
                packet_ptr += *packet_ptr + 1;
                char *car_skin = readString(packet_ptr);
                //packet_ptr += *packet_ptr + 1;

                fprintf(stderr, "DRIVER: %ls GUID: %ls\n", driver_name, driver_guid);
                fprintf(stderr, "CAR: %d MODEL: %s %s\n", car_id, car_model, car_skin);
                free(driver_guid);
                free(driver_name);
                free(car_model);
                free(car_skin);
            }
                break;
            case ACSP_LAP_COMPLETED: {
                fprintf(stderr, "ACSP_LAP_COMPLETED\n");
                uint8_t car_id, cuts, cars_count;
                uint32_t lap_time;

                car_id = *packet_ptr++;
                memmove(&lap_time, packet_ptr, sizeof(uint32_t));
                packet_ptr += sizeof(uint32_t);
                cuts = *packet_ptr++;
                fprintf(stderr, "CAR: %d\n\tLAP: %d\n\tCUT: %d\n", car_id, lap_time, cuts);
                cars_count = *packet_ptr++;
                // LEADERBOARD
                for(uint8_t i = 0; i < cars_count; i++) {
                    uint8_t rcar_id = *packet_ptr++;
                    uint32_t rtime;
                    uint16_t rlaps;
                    memmove(&rtime, packet_ptr, sizeof(uint32_t));
                    packet_ptr += sizeof(uint32_t);
                    memmove(&rlaps, packet_ptr, sizeof(uint16_t));
                    packet_ptr += sizeof(uint16_t);
                    fprintf(stderr, "%d: CAR_ID: %d TIME: %d LAPS: %d\n", i, rcar_id, rtime, rlaps);
                }
                float grip_level;
                memmove(&grip_level, packet_ptr, sizeof(float));
                fprintf(stderr, "GRIP LEVEL: %f\n", grip_level);
            }
                break;
            default:
                fprintf(stderr, "Unkown packet_id received.\n");
        }
    }
    return 0;
}