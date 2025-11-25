```
 ██████╗██╗  ██╗███████╗███████╗██╗  ██╗██╗██████╗ ███████╗
██╔════╝██║  ██║██╔════╝██╔════╝██║  ██║██║██╔══██╗██╔════╝
██║     ███████║█████╗  ███████╗███████║██║██████╔╝█████╗  
██║     ██╔══██║██╔══╝  ╚════██║██╔══██║██║██╔══██╗██╔══╝  
╚██████╗██║  ██║███████╗███████║██║  ██║██║██║  ██║███████╗
                                                                       v1.0
  [ POLYMORPHIC GAME-STATE COVERT CHANNEL ]


==[ 0x00 :: MANIFEST ]==========================================================

  Project:      Cheshire
  Version:      1.0 (Stable)
  Type:         Header-Only Library / PIC Implant Engine
  Target:       x86_64 Linux (System V ABI) [ WINDOWS 11 IN THE FUTURE ]
  License:      MIT

  [ CORE LIBRARY ] 
  src/pic.h           : The "Brain". Header-only implant logic. No libc.
  src/encoder.c/h     : Host-side traffic generation library.
  src/json.c/h        : Wrapper for masquerading binary blobs as JSON.
  src/utils.c/h       : Helpers for bitwise operations.

  [ EXAMPLE ] 
  example/builder.c   : Demo C2 implementation using the encoder library.
  example/loader.c    : Demo Loader showing how to inject the implant.
  example/payload.c   : Dummy shellcode ("PWNED") for testing.
  example/implant.c   : Demo Agent; uses pic.h to decode traffic.

==[ 0x01 :: INTRODUCTION ]======================================================

  Cheshire is a header-only C library designed to be integrated into 
  C2 Frameworks and Loaders. It provides a covert channel protocol that 
  tunnels binary payloads inside the state data of simple games (Tic-Tac-Toe, 
  Connect4, etc).
  
  This repo contains the core library and a Proof-of-Concept 
  implementation consisting of a builder and a loader.

  Key Features:
  1. Polymorphism: Traffic signature changes every run.
  2. Stealth:      Uses Packed Structs and JSON wrapping to blend in.
  3. Independence: The implant is Position Independent Code (PIC) with zero
                   external dependencies.
                   
==[ 0x02 :: THE PROTOCOL ]======================================================

  [ PACKET STRUCTURE (80 Bytes) ]
  The wire format is strictly packed to ensure binary compatibility between
  the high-level C2 and the raw-assembly Implant.

  struct __attribute__((packed)) {
      uint8_t  data[64];      // The Game Board state
      uint64_t size;          // Logical size of game data
      uint32_t bits_encoded;  // Entropy count
      uint32_t engine_id;     // 0=TTT, 1=Coin, 2=Dice, 3=Connect4, 4=RPS, 5=Card
  }

  [ STREAM FLOW ]
  
  1. HANDSHAKE (Packet 0):
     - Engine: Tic-Tac-Toe
     - Content: Encoded TTT board (14 bits); first 8 bits carry the key.
     - Logic:   session_key = (first_8_bits ^ 0x55).
     
  2. NOISE (Random Intervals):
     - Engines: Coin, Dice, RPS, Card.
     - Content: Random garbage.
     - Logic:   Implant decodes but ignores.

  3. PAYLOAD (Data Chunks):
     - Engine: Connect Four (High capacity).
     - Header: 8 bits (Sequence ID).
     - Body:   48 bits (6 Bytes of Encrypted Payload).
     - Logic:  Implant places Body at (Seq_ID * 6) in memory.


==[ 0x03 :: OBFUSCATION ]====================================================

  [ CIPHER ]
  XOR-Shift Stream with Rotating Key Schedule.
  - Initial Key: Negotiated in Handshake.
  - Algo: Buffer[i] ^= Key; Key = ROL(Key, 1) + 0x55;
  - Implementation: Inline Assembly in `pic.h` to avoid compiler artifacts.

  [ JSON WRAPPING ]
  Traffic can be wrapped in a generic JSON structure for entropy
  scanners looking for raw binary files.
  
  {
    "id": 101,
    "timestamp": 1732501200,
    "state": "base64_encoded_packet_data..."
  }


==[ 0x04 :: BUILD & USAGE ]=====================================================

  The unified Makefile handles the build chain (host-side tools vs 
  implant-side build using freestanding-style GCC + linker scripts).

  $ make all
  
  [ ARTIFACTS ]
  build/bin/builder      <-- Traffic Generator (Demo C2)
  build/bin/loader       <-- Loader Harness (Demo Stager)
  build/bin/implant.bin  <-- Compiled Implant (PIC / Logic)
  build/bin/payload.bin  <-- Dummy Payload (Raw Shellcode)

  [ RUNNING THE DEMO ]
  
  1. Create Traffic (Binary Mode):
     # Generates traffic.dat using payload.bin as the secret
     $ ./build/bin/builder traffic.dat @build/bin/payload.bin
     
  2. Create Traffic (JSON Mode):
     # Generates traffic.json (masquerading as API data)
     $ ./build/bin/builder traffic.json @build/bin/payload.bin --json
     
  3. Execute Implant:
     # Loader injects the implant, which then processes the traffic file
     $ ./build/bin/loader build/bin/implant.bin traffic.json


==[ 0x05 :: LIMITATIONS ]=======================================================

  1. Payload Size: 
     Restricted by the 8-bit Sequence ID in the Connect4 header.
     Max = 256 chunks * 6 bytes = 1,536 bytes.
     (Sufficient for stagers, small shellcode, or Metasploit tcp_bind).

  2. Security:
     The Bootstrap Key (0x55) is hardcoded. If the implant is RE'd,
     historical traffic can be decrypted if captured. 
     Recommendation: Rotate keys per campaign.

  3. Error Correction:
     The protocol handles reordering and drops (gaps in memory), but 
     does not request retransmission. 
     [ CURRENTLY FAILS IF PAYLOAD PACKETS DROP ]

==[ 0x06 :: RED TEAM QUICKSTART ]======================================

  Explains how to theoretically plug Cheshire into an engagement. 

  [ 1. WHAT CHESHIRE GIVES YOU ]

  At a high level, Cheshire provides:

  - A tiny implant brain (pic.h) that:
      - Consumes a stream of fixed-size game-state packets (pic_state_t).
      - Derives a per-run session key from a Tic-Tac-Toe handshake.
      - Reassembles 6-byte chunks from Connect4 packets into a payload.
      - Decrypts the final buffer using the XOR + ROL + 0x55 stream cipher.
  - A demo loader that:
      * Maps the implant into executable memory (implant.bin).
      * Reads a traffic file (binary or JSON-wrapped).
      * Calls the implant entrypoint (start(void*, long, char*)).
      * Transfers execution to the reconstructed payload.
  - Host-side helpers to generate polymorphic, game-themed traffic that can
    be serialized as raw bytes or JSON.

  [ 2. USING THE DEMO TOOLING ]

  The fastest way to get value is to treat the included tools as an example
  C2 + loader pipeline.

  2.1 Swap in your payload

  - Edit `example/payload.c` to emit your shellcode/stager.
  - Keep the total payload size under ~1.5 KB:
      * Max chunks  = 256 (8-bit Sequence ID)
      * Bytes/chunk = 6
      * Capacity    ≈ 256 * 6 = 1,536 bytes

  - Rebuild the shellcode-related artifacts with make.

  2.2 Generate covert traffic

  - Binary mode:

      $ ./build/bin/builder traffic.dat @build/bin/payload.bin

    Produces a raw stream of 80-byte packets (handshake, noise, payload)
    suitable for file or socket transport.

  - JSON mode:

      $ ./build/bin/builder traffic.json @build/bin/payload.bin --json

    Wraps each 80-byte packet as a JSON object with a base64 `state` field,
    masquerading as mundane API/telemetry data.

  2.3 Run the loader + implant

  - Execute the demo loader against the generated traffic:

      $ ./build/bin/loader build/bin/implant.bin traffic.json

    The loader will:
      - Map implant.bin into executable memory.
      - Read and decode the traffic stream (binary or JSON).
      - Invoke the implant's start() function with the stream buffer.
      - Allow the implant to reconstruct, decrypt, and execute the payload.

  This path is intended as a lab/demo harness and a reference implementation.
  For real engagements, rename and re-implement.

  [ 3. EMBEDDING CHESHIRE INTO CUSTOM C2 / LOADERS ]

  If you want to skip the demo tools (`builder` / `loader`) and wire the
  library directly into your own framework:

  3.1 Host / C2 side

  - Link against:
    - src/encoder.c
    - src/utils.c
    - src/json.c   (optional, if you want JSON output)

  - API surface:
      - getCapacity(engine)      : how many bits a given engine can carry.
      - encode(engine, bits)     : encode a bitstring into a `state_t`.
      - decode(state_t)          : recover the bitstring from a state.
      - encodeChunkedStream(buf, len):
            - Takes an arbitrary byte buffer.
            - Emits a linked list of game-state packets containing:
                - 1 × TTT handshake
                - N × noise frames (coin/dice/RPS/card)
                - M × CONNECT4 payload chunks

  - Serialization:
      - You are free to serialize the packets as:
          - Raw 80-byte records, or
          - JSON objects with base64 `state` via the JSON helper.

  3.2 Implant / target side

  - Include src/pic.h and either:
      - Use the provided example/implant.c, or
      - Write your own wrapper that calls:

            int picProcessChunkedStream(void* streamPtr,
                                       sz_t  streamLen,
                                       char* outBuffer);

  - Requirements:
      - streamPtr must point to a contiguous buffer containing an integer
        number of pic_state_t packets.
      - streamLen is the total size in bytes of that buffer.
      - outBuffer must be large enough to hold the reconstructed payload:
          - 1,536 bytes is the theoretical maximum.
          - The current implementation caps at 4,096 bytes; allocating that
            much is a safe default.

  - On success, picProcessChunkedStream() returns the number of decrypted
    bytes written to outBuffer. The caller is responsible for deciding
    how (and whether) to execute those bytes.

==[ 0x07 :: EOF ]===============================================================
```
