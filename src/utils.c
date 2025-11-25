// utils.c
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int readFile(const char* path, char** buffer, size_t* len)
{
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    *len = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    *buffer = malloc(*len);
    if (!*buffer)
    {
        fclose(file);
        return -1;
    }
    
    if (fread(*buffer, 1, *len, file) != *len)
    {
        free(*buffer);
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}

char* bytesToBits(const unsigned char* data, size_t n)
{
    size_t bitLen = n * 8;
    char* bits = malloc(bitLen + 1);
    if (!bits) return NULL;
    
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
    {
        unsigned char b = data[i];
        for (int j = 7; j >= 0; j--)
        {
            bits[k++] = (b >> j) & 1 ? '1' : '0';
        }
    }
    bits[k] = '\0';
    return bits;
}

