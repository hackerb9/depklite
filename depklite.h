#ifndef DEPKLITE_H
#define DEPKLITE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// depklite, a derivative of OpenTESArena's ExeUnpacker.
// Used for decompressing DOS executables compressed with PKLITE.

int32_t  depklite_unpack(FILE *fp, unsigned char *decompBuff, int buffsize, int compressedDataOffset, bool useDecryption);

int printBitData(int byteIndex, GetNextBit_Data data);

#endif
