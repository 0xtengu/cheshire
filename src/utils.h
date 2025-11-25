// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Read binary file into a heap buffer
int readFile(const char* path, char** buffer, size_t* len);

// Convert raw bytes to a "10110" bit string (caller frees)
char* bytesToBits(const unsigned char* data, size_t n);

#endif

