// json_wrapper.h - JSON encoding/decoding for packets
#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

#include "encoder.h"
#include <stddef.h>

// Encode single packet to JSON string
void jsonEncodePacket(const state_t* packet, char* out, size_t outSize);

// Decode single JSON object to packet
// Returns 1 on success, 0 on failure
int jsonDecodePacket(const char* json, state_t* out);

// Encode entire stream to JSON file
void jsonEncodeStreamToFile(packet_t* stream, const char* filename);

// Decode JSON file to raw packet buffer
// Returns number of bytes written to outBuffer
size_t jsonDecodeFileToRaw(const char* filename, uint8_t* outBuffer, size_t maxSize);

#endif

