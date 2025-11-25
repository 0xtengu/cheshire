#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encoder.h"
#include "utils.h"
#include "json.h"

int main(int argc, char** argv)
{
    int useJson = 0;
    if (argc >= 4 && strcmp(argv[3], "--json") == 0)
    {
        useJson = 1;
    }
    
    if (argc < 3)
    {
        printf("Usage: %s <traffic_file> <message|@file> [--json]\n", argv[0]);
        printf("  --json: Output as JSON instead of binary\n");
        return 1;
    }
    
    if (sizeof(state_t) != 80)
    {
        printf("[ERROR] Struct size mismatch! Expected 80, got %lu.\n", (unsigned long)sizeof(state_t));
        return 1;
    }
    
    seedInit(0);
    
    const char* outputFile = argv[1];
    const char* inputArg = argv[2];
    
    char* payloadData = NULL;
    size_t payloadLen = 0;
    
    if (inputArg[0] == '@')
    {
        printf("[C2] Reading payload from file: %s\n", inputArg + 1);
        if (readFile(inputArg + 1, &payloadData, &payloadLen) != 0)
        {
            printf("[ERROR] Could not read file.\n");
            return 1;
        }
    }
    else
    {
        printf("[C2] Using raw string payload.\n");
        payloadLen = strlen(inputArg);
        payloadData = malloc(payloadLen);
        if (!payloadData)
        {
            printf("[ERROR] Memory allocation failed.\n");
            return 1;
        }
        memcpy(payloadData, inputArg, payloadLen);
    }
    
    printf("[C2] Generating chunked stream (%zu bytes)...\n", payloadLen);
    
    packet_t* stream = encodeChunkedStream(payloadData, payloadLen);
    
    if (!stream)
    {
        printf("[ERROR] Stream generation failed.\n");
        free(payloadData);
        return 1;
    }
    
    if (useJson)
    {
        jsonEncodeStreamToFile(stream, outputFile);
    }
    else
    {
        FILE* file = fopen(outputFile, "wb");
        if (!file)
        {
            perror("fopen output");
            freeStream(stream);
            free(payloadData);
            return 1;
        }
        
        packet_t* curr = stream;
        while (curr)
        {
            fwrite(&curr->state, sizeof(state_t), 1, file);
            curr = curr->next;
        }
        
        fclose(file);
        printf("[C2] Done. Traffic written to %s\n", outputFile);
    }
    
    freeStream(stream);
    free(payloadData);
    
    return 0;
}
