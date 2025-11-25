// loader.c 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "json.h"

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s <implant> <traffic>\n", argv[0]);
        printf("  <traffic> can be .dat (binary) or .json (JSON)\n");
        return 1;
    }
    
    // 1. Load implant
    FILE* file = fopen(argv[1], "rb");
    if (!file)
    {
        perror("fopen implant");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long implantSize = ftell(file);
    rewind(file);
    
    printf("[DEBUG] Implant Size: %ld bytes\n", implantSize);
    
    void* implantMem = mmap(
        NULL,
        implantSize,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    fread(implantMem, 1, implantSize, file);
    fclose(file);
    
    // 2. Load traffic (detect JSON vs binary)
    int isJson = (strstr(argv[2], ".json") != NULL);
    void* trafficMem = NULL;
    long trafficSize = 0;
    
    if (isJson)
    {
        printf("[LOADER] Detected JSON input, decoding...\n");
        uint8_t rawBuffer[65536];
        trafficSize = (long)jsonDecodeFileToRaw(argv[2], rawBuffer, sizeof(rawBuffer));
        if (trafficSize == 0)
        {
            printf("[ERROR] Failed to decode JSON\n");
            return 1;
        }
        trafficMem = malloc(trafficSize);
        if (!trafficMem)
        {
            printf("[ERROR] Memory allocation failed\n");
            return 1;
        }
        memcpy(trafficMem, rawBuffer, trafficSize);
    }
    else
    {
        printf("[LOADER] Loading binary traffic...\n");
        file = fopen(argv[2], "rb");
        if (!file)
        {
            perror("fopen traffic");
            return 1;
        }

        fseek(file, 0, SEEK_END);
        trafficSize = ftell(file);
        rewind(file);
        
        trafficMem = malloc(trafficSize);
        if (!trafficMem)
        {
            printf("[ERROR] Memory allocation failed\n");
            fclose(file);
            return 1;
        }

        fread(trafficMem, 1, trafficSize, file);
        fclose(file);
    }
    
    printf("[DEBUG] Traffic Size: %ld bytes\n", trafficSize);
    
    // 3. Output buffer
    void* payloadMem = mmap(
        NULL,
        4096,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    
    // Debug: hexdump first 16 bytes of implant
    printf("[DEBUG] Implant Entry Point Head (First 16 bytes):\n");
    unsigned char* p = (unsigned char*)implantMem;
    for (int i = 0; i < 16 && i < implantSize; i++)
    {
        printf("%02X ", p[i]);
    }
    printf("\n");
    
    printf("[LOADER] Jumping to Implant at %p...\n", implantMem);
    
    typedef int (*implantFunc)(void*, long, char*);
    implantFunc jumpToImplant = (implantFunc)implantMem;
    
    int ret = jumpToImplant(trafficMem, trafficSize, (char*)payloadMem);
    
    printf("[LOADER] RETURNED! Value: 0x%X\n", ret);
    
    free(trafficMem);
    
    return 0;
}

