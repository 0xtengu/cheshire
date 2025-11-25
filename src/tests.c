// tests.c 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "encoder.h"
#include "pic.h"

void testEngine(game_engine_t engine)
{
    printf("[UNIT] Testing Engine: %s... ", getEngineName(engine));
    
    int cap = getCapacity(engine);
    char inBits[128] = {0};
    char outBits[128] = {0};
    
    for (int i = 0; i < cap; i++)
    {
        inBits[i] = (rand() % 2) ? '1' : '0';
    }
    
    state_t hostPacket = encode(engine, inBits);
    
    pic_state_t implantPacket;
    memcpy(implantPacket.data, hostPacket.data, sizeof(hostPacket.data));
    implantPacket.engine = (pic_engine_t)hostPacket.engine;
    
    int decodedLen = pic_decode(&implantPacket, outBits);
    
    assert(decodedLen == cap);
    assert(strncmp(inBits, outBits, cap) == 0);
    printf("PASS\n");
}

void testIntegration()
{
    printf("\n[INTEGRATION] Running Full Chunked Kill Chain...\n");
    
    const char* secret = "EXECUTE_ORDER_66_THIS_IS_A_TEST";
    size_t secretLen = strlen(secret);
    
    printf("  1. Host: Generating Chunked Traffic...\n");
    packet_t* stream = encodeChunkedStream(secret, secretLen);
    assert(stream != NULL);
    
    size_t streamSize = 0;
    packet_t* curr = stream;
    while (curr)
    {
        streamSize += sizeof(pic_state_t);
        curr = curr->next;
    }
    
    unsigned char* networkTraffic = malloc(streamSize);
    unsigned char* ptr = networkTraffic;
    
    curr = stream;
    while (curr)
    {
        pic_state_t p;
        memcpy(p.data, curr->state.data, 64);
        p.engine = (pic_engine_t)curr->state.engine;
        
        memcpy(ptr, &p, sizeof(pic_state_t));
        ptr += sizeof(pic_state_t);
        curr = curr->next;
    }
    
    printf("  2. Implant: Processing %zu bytes of traffic...\n", streamSize);
    char recoveredBuffer[1024] = {0};
    
    int recoveredLen = pic_process_chunked_stream(networkTraffic, streamSize, recoveredBuffer);
    
    printf("  3. Verification: ");
    
    if (recoveredLen >= (int)secretLen && memcmp(secret, recoveredBuffer, secretLen) == 0)
    {
        printf("SUCCESS (Payload Matches)\n");
    }
    else
    {
        printf("FAIL\n");
        printf("     Expected: %s\n", secret);
        printf("     Got:      %s\n", recoveredBuffer);
        printf("     Len Expect: %zu, Len Got: %d\n", secretLen, recoveredLen);
        exit(1);
    }
    
    freeStream(stream);
    free(networkTraffic);
}

int main()
{
    srand(time(NULL));
    printf("--- CHUNKED VERIFICATION ---\n");
    
    testEngine(GAME_TTT);
    testEngine(GAME_COIN);
    testEngine(GAME_DICE);
    testEngine(GAME_CONNECT4);
    testEngine(GAME_RPS);
    testEngine(GAME_CARD);
    
    testIntegration();
    
    printf("\nALL SYSTEMS GO.\n");
    return 0;
}

