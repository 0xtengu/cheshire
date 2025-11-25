// encoder.h - Host API (Fixed Binary Layout)
#ifndef ENCODER_H
#define ENCODER_H

#include <stddef.h>
#include <stdint.h>

typedef enum 
{
    GAME_TTT,
    GAME_COIN,
    GAME_DICE,
    GAME_CONNECT4,
    GAME_RPS,
    GAME_CARD
} game_engine_t;

#define MAX_STATE_BYTES 64

// Must be packed to match pic.h in the implant
typedef struct __attribute__((packed))
{
    uint8_t       data[MAX_STATE_BYTES]; // 64 bytes
    size_t        size;                  // 8 bytes (64-bit)
    int           bits_encoded;          // 4 bytes
    game_engine_t engine;                // 4 bytes
} state_t;                               // Total: 80 bytes (no padding)

void seedInit(uint32_t seed);
int getCapacity(game_engine_t engine);
state_t encode(game_engine_t engine, const char* bits);
int decode(const state_t* state, char* out);
const char* getEngineName(game_engine_t engine);

typedef struct packet_s
{
    state_t            state;
    struct packet_s*   next;
} packet_t;

packet_t* encodeChunkedStream(const void* data, size_t len);
void freeStream(packet_t* head);

#endif

