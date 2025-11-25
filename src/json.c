// json_wrapper.c
#include "json_wrapper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Base64 encoding table
static const char b64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 encode
static void base64Encode(const uint8_t* in, size_t len, char* out)
{
    size_t i, j;
    for (i = 0, j = 0; i < len; i += 3, j += 4)
    {
        uint32_t n = (in[i] << 16) |
                     ((i + 1 < len) ? (in[i + 1] << 8) : 0) |
                     ((i + 2 < len) ? in[i + 2] : 0);

        out[j]   = b64Table[(n >> 18) & 0x3F];
        out[j+1] = b64Table[(n >> 12) & 0x3F];
        out[j+2] = (i + 1 < len) ? b64Table[(n >> 6) & 0x3F] : '=';
        out[j+3] = (i + 2 < len) ? b64Table[n & 0x3F] : '=';
    }
    out[j] = '\0';
}

// Base64 decode helper
static int b64CharValue(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

// Base64 decode
static size_t base64Decode(const char* in, uint8_t* out, size_t maxOut)
{
    size_t i = 0;
    size_t j = 0;

    while (in[i] && in[i] != '"' && in[i] != '}' && j < maxOut)
    {
        if (in[i] == '=') break;

        int a = b64CharValue(in[i++]);
        int b = (in[i] && in[i] != '=') ? b64CharValue(in[i++]) : 0;
        int c = (in[i] && in[i] != '=') ? b64CharValue(in[i++]) : 0;
        int d = (in[i] && in[i] != '=') ? b64CharValue(in[i++]) : 0;

        if (a < 0 || b < 0) break;

        out[j++] = (a << 2) | (b >> 4);
        if (c >= 0 && j < maxOut) out[j++] = (b << 4) | (c >> 2);
        if (d >= 0 && j < maxOut) out[j++] = (c << 6) | d;
    }

    return j;
}

void jsonEncodePacket(const state_t* packet, char* out, size_t outSize)
{
    char b64[256];
    base64Encode((const uint8_t*)packet, sizeof(state_t), b64);
    snprintf(out, outSize, "{\"state\":\"%s\"}", b64);
}

int jsonDecodePacket(const char* json, state_t* out)
{
    const char* p = strstr(json, "\"state\":\"");
    if (!p) return 0;

    p += 9;  // move to base64 start

    size_t decoded = base64Decode(p, (uint8_t*)out, sizeof(state_t));
    return (decoded == sizeof(state_t)) ? 1 : 0;
}

void jsonEncodeStreamToFile(packet_t* stream, const char* filename)
{
    FILE* file = fopen(filename, "w");
    if (!file)
    {
        perror("fopen json output");
        return;
    }

    fprintf(file, "[");

    packet_t* curr = stream;
    int first = 1;

    while (curr)
    {
        if (!first) fprintf(file, ",\n");
        first = 0;

        char json[512];
        jsonEncodePacket(&curr->state, json, sizeof(json));
        fprintf(file, "%s", json);

        curr = curr->next;
    }

    fprintf(file, "]\n");
    fclose(file);

    printf("[JSON] Encoded stream to %s\n", filename);
}

size_t jsonDecodeFileToRaw(const char* filename, uint8_t* outBuffer, size_t maxSize)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        perror("fopen json input");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char* json = malloc(fileSize + 1);
    if (!json)
    {
        fclose(file);
        return 0;
    }

    fread(json, 1, fileSize, file);
    json[fileSize] = '\0';
    fclose(file);

    size_t totalBytes = 0;
    const char* p = json;
    int packetCount = 0;

    // find each "state" field
    while ((p = strstr(p, "\"state\"")) != NULL)
    {
        p = strchr(p, ':');
        if (!p) break;
        p = strchr(p, '"');
        if (!p) break;
        p++;  // at base64 start

        state_t packet;
        size_t decoded = base64Decode(p, (uint8_t*)&packet, sizeof(state_t));

        if (decoded == sizeof(state_t))
        {
            if (totalBytes + sizeof(state_t) <= maxSize)
            {
                memcpy(outBuffer + totalBytes, &packet, sizeof(state_t));
                totalBytes += sizeof(state_t);
                packetCount++;
            }
        }

        p++;  // move forward
    }

    free(json);

    printf("[JSON] Decoded %d packets (%zu bytes) from %s\n",
           packetCount, totalBytes, filename);

    return totalBytes;
}

