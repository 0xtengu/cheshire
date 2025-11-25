// pic.h - Protocol Definition
#ifndef PIC_H
#define PIC_H

#define PIC_INLINE static inline __attribute__((always_inline))

typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef unsigned long      sz_t;

typedef enum
{
    PIC_GAME_TTT      = 0,
    PIC_GAME_COIN     = 1,
    PIC_GAME_DICE     = 2,
    PIC_GAME_CONNECT4 = 3,
    PIC_GAME_RPS      = 4,
    PIC_GAME_CARD     = 5
} pic_engine_t;

#define PIC_MAX_STATE_BYTES 64

// Force byte-alignment to match host fwrite() exactly
typedef struct __attribute__((packed))
{
    u8           data[PIC_MAX_STATE_BYTES];
    sz_t         size;
    int          bits_encoded;
    pic_engine_t engine;
} pic_state_t;

// --- INLINE ASSEMBLY CRYPTO ---
PIC_INLINE void picCrypt(void* buffer, sz_t len, u8 key)
{
    if (len == 0) return;
    __asm__ volatile (
        "xor %%rax, %%rax\n"
        "1:\n"
        "movb (%%rdi), %%al\n"
        "xorb %%dl, %%al\n"
        "movb %%al, (%%rdi)\n"
        "rolb $1, %%dl\n"
        "addb $0x55, %%dl\n"
        "inc %%rdi\n"
        "dec %%rsi\n"
        "jnz 1b\n"
        : "+D"(buffer), "+S"(len), "+d"(key)
        :
        : "rax", "cc", "memory"
    );
}

// --- DECODERS ---
PIC_INLINE u64 picBaseNToU64(const u8* data, int len, int base)
{
    u64 val = 0;
    u64 mult = 1;

    for (int i = 0; i < len; i++)
    {
        val += data[i] * mult;
        mult *= base;
    }

    return val;
}

PIC_INLINE void picU64ToBits(u64 val, char* out, int n)
{
    for (int i = n - 1; i >= 0; i--)
    {
        out[i] = (val & 1) ? '1' : '0';
        val >>= 1;
    }
    out[n] = '\0';
}

PIC_INLINE int picDecode(const pic_state_t* state, char* outBuf)
{
    switch (state->engine)
    {
        case PIC_GAME_TTT:
        {
            u64 val = picBaseNToU64(state->data, 9, 3);
            picU64ToBits(val, outBuf, 14);
            return 14;
        }
        case PIC_GAME_COIN:
        {
            outBuf[0] = (state->data[0] & 1) ? '1' : '0';
            outBuf[1] = '\0';
            return 1;
        }
        case PIC_GAME_DICE:
        {
            u64 val = state->data[0];
            picU64ToBits(val, outBuf, 2);
            return 2;
        }
        case PIC_GAME_CONNECT4:
        {
            u64 val = picBaseNToU64(state->data, 42, 3);
            picU64ToBits(val, outBuf, 60);
            return 60;
        }
        case PIC_GAME_RPS:
        {
            u64 val = state->data[0];
            if (val >= 8) return 0;
            picU64ToBits(val, outBuf, 3);
            return 3;
        }
        case PIC_GAME_CARD:
        {
            u64 val = state->data[0];
            if (val >= 32) return 0;
            picU64ToBits(val, outBuf, 5);
            return 5;
        }
        default:
            return 0;
    }
}

// --- PROCESSOR ---
PIC_INLINE int picReassembleBits(const char* bits, int bitCount, u8* outBytes, int* bitIdx, u8* currentByte)
{
    int bytesWritten = 0;

    for (int i = 0; i < bitCount; i++)
    {
        if (bits[i] == '1')
        {
            *currentByte |= (1 << (7 - *bitIdx));
        }

        (*bitIdx)++;

        if (*bitIdx == 8)
        {
            outBytes[bytesWritten++] = *currentByte;
            *currentByte = 0;
            *bitIdx = 0;
        }
    }

    return bytesWritten;
}

// --- CHUNKED PROTOCOL ---

typedef struct
{
    u8 seq_num;
    u8 data[6];
    u8 valid;
} pic_chunk_t;

PIC_INLINE int picExtractChunk(const pic_state_t* state, pic_chunk_t* chunk)
{
    if (state->engine != PIC_GAME_CONNECT4) return 0;

    char bits[65];
    int bitCount = picDecode(state, bits);
    if (bitCount != 60) return 0;

    // sequence number (first 8 bits)
    u8 seq = 0;
    for (int i = 0; i < 8; i++)
    {
        if (bits[i] == '1')
        {
            seq |= (1 << (7 - i));
        }
    }
    chunk->seq_num = seq;

    // payload (next 48 bits = 6 bytes)
    int bitIdx = 0;
    u8 currentByte = 0;
    int dataIdx = 0;

    for (int i = 8; i < 56 && dataIdx < 6; i++)
    {
        if (bits[i] == '1')
        {
            currentByte |= (1 << (7 - bitIdx));
        }

        bitIdx++;

        if (bitIdx == 8)
        {
            chunk->data[dataIdx++] = currentByte;
            currentByte = 0;
            bitIdx = 0;
        }
    }

    chunk->valid = 1;
    return 1;
}

PIC_INLINE int picProcessChunkedStream(void* streamPtr, sz_t streamLen, char* outBuffer)
{
    u8* reader = (u8*)streamPtr;
    u8* end = reader + streamLen;

    pic_chunk_t chunks[256];
    int chunkCount = 0;

    int haveKey = 0;
    u8 sessionKey = 0;
    const u8 BOOTSTRAP_KEY = 0x55;

    while (reader + sizeof(pic_state_t) <= end)
    {
        pic_state_t* packet = (pic_state_t*)reader;

        if (!haveKey && packet->engine == PIC_GAME_TTT)
        {
            char bits[65];
            int count = picDecode(packet, bits);
            if (count >= 8)
            {
                u8 encryptedKey = 0;
                for (int i = 0; i < 8; i++)
                {
                    if (bits[i] == '1')
                    {
                        encryptedKey |= (1 << (7 - i));
                    }
                }
                sessionKey = encryptedKey ^ BOOTSTRAP_KEY;
                haveKey = 1;
            }
        }
        else if (haveKey && packet->engine == PIC_GAME_CONNECT4)
        {
            pic_chunk_t chunk;
            if (picExtractChunk(packet, &chunk) && chunkCount < 256)
            {
                chunks[chunkCount++] = chunk;
            }
        }

        reader += sizeof(pic_state_t);
    }

    if (!haveKey) return 0;

    // sort by sequence number
    for (int i = 0; i < chunkCount - 1; i++)
    {
        for (int j = 0; j < chunkCount - i - 1; j++)
        {
            if (chunks[j].seq_num > chunks[j + 1].seq_num)
            {
                pic_chunk_t temp = chunks[j];
                chunks[j] = chunks[j + 1];
                chunks[j + 1] = temp;
            }
        }
    }

    // reassemble
    sz_t totalBytes = 0;
    for (int i = 0; i < chunkCount && totalBytes < 4096; i++)
    {
        for (int j = 0; j < 6 && totalBytes < 4096; j++)
        {
            outBuffer[totalBytes++] = chunks[i].data[j];
        }
    }

    if (totalBytes > 0)
    {
        picCrypt(outBuffer, totalBytes, sessionKey);
    }

    return (int)totalBytes;
}

#endif
