```
 ██████╗██╗  ██╗███████╗███████╗██╗  ██╗██╗██████╗ ███████╗
██╔════╝██║  ██║██╔════╝██╔════╝██║  ██║██║██╔══██╗██╔════╝
██║     ███████║█████╗  ███████╗███████║██║██████╔╝█████╗  
██║     ██╔══██║██╔══╝  ╚════██║██╔══██║██║██╔══██╗██╔══╝  
╚██████╗██║  ██║███████╗███████║██║  ██║██║██║  ██║███████╗
                                                                       v1.0
  [ POLYMORPHIC GAME-STATE COVERT CHANNEL ]


==[ 0x00 :: MANIFEST ]==========================================================

  Project:      ROULETTE
  Version:      1.0 (Stable)
  Type:         Header-Only Library / PIC Implant Engine
  Target:       x86_64 Linux (System V ABI) [ WINDOWS 11 IN THE FUTURE ]
  License:      MIT

  [ CORE LIBRARY ] 
  src/pic.h           : The "Brain". Header-only implant logic. No libc.
  src/encoder.c/h     : Host-side traffic generation library.
  src/json.c/h        : Wrapper for masquerading binary blobs as JSON.
  src/utils.c/h       : Helpers for bitwise operations.

  [ EXAMPLES ] 
  builder.c           : Demo C2 implementation using the encoder library.
  loader.c            : Demo Loader showing how to inject the implant.
  payload.c           : Dummy shellcode ("PWNED") for testing.
  implant.c           : Demo Agent; uses pic.h to decode traffic.
  

==[ 0x01 :: INTRODUCTION ]======================================================

  Roulette is a header-only C library designed to be integrated into 
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
      uint32_t engine_id;     // 0=TTT, 1=Coin, ... 3=Connect4
  }

  [ STREAM FLOW ]
  
  1. HANDSHAKE (Packet 0):
     - Engine: Tic-Tac-Toe
     - Content: 8 bits of data.
     - Logic:   session_key = (data ^ 0x55).
     
  2. NOISE (Random Intervals):
     - Engines: TTT, Coin, Dice, RPS, Card.
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

  The unified Makefile handles the build chain (Host GCC vs 
  Implant Freestanding GCC + Linker Scripts).

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

==[ 0x06 :: EOF ]===============================================================
```
