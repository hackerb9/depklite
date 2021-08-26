#ifndef DEPKLITE_H
#define DEPKLITE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// Data structure for getNextByte() lambda.
typedef struct
{
	const uint8_t *compressedStart;
	int byteIndex;
} GetNextByte_Data;

// Data structure for getNextBit() lambda.
typedef struct
{
	uint16_t bitArray;
	int bitsRead;
	GetNextByte_Data getNextByteDat;
} GetNextBit_Data;

// depklite, a derivative of OpenTESArena's ExeUnpacker.
// Used for decompressing DOS executables compressed with PKLITE.

int32_t  depklite_unpack(FILE *fp, unsigned char *decompBuff, int buffsize, int compressedDataOffset, bool useDecryption);

int printBitData(int byteIndex, GetNextBit_Data data);

#endif
