// depklite 
// decompress DOS executables compressed with PKLITE.
// Based on the depklite module from refkeen, which
// in turn derived it from OpenTESArena's ExeUnpacker.
//
// @dozayon has documented the format here:
// https://github.com/afritz1/OpenTESArena/blob/master/docs/pklite_specification.md
// 

#define _GNU_SOURCE							/* for asprintf(), raw strings */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>							/* for uint8_t */
#include <locale.h>							/* for printf("%'d") */
#include <getopt.h>							/* for getopt_long() */
#include <stdarg.h>							/* for va_list, va_arg */

#include "depklite.h"

// Verbose print global variable
bool vflag=false;

// Just like fprintf to stderr, but conditional on vflag.
void verbose(char *fmt, ...)
{
	if (vflag)
	{
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}


// Replacement for Debug::check
static void Debug_check(bool expression, const char *msg)
{
	if (!expression)
	{
		fprintf(stderr, "depklite - %s", msg);
		exit(1);
	}
}

/*** C++11 lambda emulation ***/

// Lambda for getting the next byte from compressed data.
static uint8_t getNextByte(GetNextByte_Data *data)
{
	return data->compressedStart[data->byteIndex++];
}

// Lambda for getting the next bit in the theoretical bit stream.
static bool getNextBit(GetNextBit_Data *data)
{
	const bool bit = (data->bitArray & (1 << data->bitsRead)) != 0;
	data->bitsRead++;

	// Advance the bit array if done with the current one.
	if (data->bitsRead == 16)
	{
		data->bitsRead = 0;

		// Get two bytes in little endian format.
		const uint8_t byte1 = getNextByte(&data->getNextByteDat);
		const uint8_t byte2 = getNextByte(&data->getNextByteDat);
		data->bitArray = byte1 | (byte2 << 8);
	}

	return bit;
};


/*** A simple binary tree for retrieving a decoded value, given a vector of bits. ***/

typedef struct Node
{
	int left;
	int right;
	int value;
} Node;

typedef Node BitTree;

// Returns a decoded value in the tree. Note that rather than getting
// an input vector of bits, this gets a pointer to a bits fetcher which
// is repeatedly called, once per bit.
static const int BitTree_get(const BitTree *bt, GetNextBit_Data *getNextBitDatPtr)
{
	const Node *node = bt;

	// Walk the tree.
	while (true)
	{
		const bool bit = getNextBit(getNextBitDatPtr);
		// Decide which branch to use.
		if (bit)
		{
			// Right.
			Debug_check(node->right != 0, "Bit Tree - No right branch.\n");
			node += node->right;
		}
		else
		{
			// Left.
			Debug_check(node->left != 0, "Bit Tree - No left branch.\n");
			node += node->left;
		}
		// Check if it's a leaf.
		if ((node->left == 0) && (node->right == 0))
		{
			return node->value;
		}
	}
}

#define ST(l,r) { l, r, -1 } // Node which isn't a leaf
#define LF(v) { 0, 0, v } // Leaf

// Bit table from pklite_specification.md, section 4.3.1 "Number of bytes".
// The decoded value for a given vector is (index + 2) before index 11, and
// (index + 1) after index 11.
//
// The array is illustrated as a binary tree, in which
// every non-leaf node is shown with its subtrees as follows
// (although the root node is "reversed" i.e., L and R are swapped):
//
// N
// 	R
// L
static const Node bitTree1[] =
{
	ST(4,1), // "Reversed" node
		ST(1,2),
			LF(2),
		LF(3),
	ST(1,6),
		ST(1,2),
			LF(4),
		ST(1,2),
			LF(5),
		LF(6),
	ST(1,6),
		ST(1,2),
			LF(7),
		ST(1,2),
			LF(8),
		LF(9),
	ST(1,6),
		ST(1,2),
			LF(10),
		ST(1,2),
			LF(11),
		LF(12),
	ST(1,6),
		ST(1,2),
			LF(25),
		ST(1,2),
			LF(13),
		LF(14),
	ST(1,6),
		ST(1,2),
			LF(15),
		ST(1,2),
			LF(16),
		LF(17),
	ST(1,6),
		ST(1,2),
			LF(18),
		ST(1,2),
			LF(19),
		LF(20),
	ST(1,4),
		ST(1,2),
			LF(21),
		LF(22),
	ST(1,2),
		LF(23),
	LF(24),
};

// Bit table from pklite_specification.md, section 4.3.2 "Offset".
// The decoded value for a given vector is simply its index.
//
// The array is illustrated in a similar manner as before.
static const Node bitTree2[] =
{
	ST(2,1), // "Reversed" node
		LF(0),
	ST(1,12),
		ST(1,4),
			ST(1,2),
				LF(1),
			LF(2),
		ST(1,4),
			ST(1,2),
				LF(3),
			LF(4),
		ST(1,2),
			LF(5),
		LF(6),
	ST(1,18),
		ST(1,8),
			ST(1,4),
				ST(1,2),
					LF(7),
				LF(8),
			ST(1,2),
				LF(9),
			LF(10),
		ST(1,4),
			ST(1,2),
				LF(11),
			LF(12),
		ST(1,2),
			LF(13),
		ST(1,2),
			LF(14),
		LF(15),
	ST(1,16),
		ST(1,8),
			ST(1,4),
				ST(1,2),
					LF(16),
				LF(17),
			ST(1,2),
				LF(18),
			LF(19),
		ST(1,4),
			ST(1,2),
				LF(20),
			LF(21),
		ST(1,2),
			LF(22),
		LF(23),
	ST(1,8),
		ST(1,4),
			ST(1,2),
				LF(24),
			LF(25),
		ST(1,2),
			LF(26),
		LF(27),
	ST(1,4),
		ST(1,2),
			LF(28),
		LF(29),
	ST(1,2),
		LF(30),
	LF(31),
};

int32_t depklite_unpack(FILE *fp, unsigned char *decompBuff, int buffsize, int compressedDataOffset, bool useDecryption)
{

	fseek(fp, 0, SEEK_END);
	int32_t fileSize = ftell(fp);
	rewind(fp);

	verbose("Filesize: %'d bytes\nBuffersize: %'d bytes\n", fileSize, buffsize);

	Debug_check(fileSize >= compressedDataOffset, "depklite - Input file is unexpectedly too small!\n");
	Debug_check(fileSize < buffsize, "depklite - Input file is too large for buffer!\n");

	verbose("Decompressing%s %'d bytes, starting at offset %'d...\n",
					useDecryption?" and decrypting":"",
					fileSize-compressedDataOffset, compressedDataOffset);
	
	uint8_t *compressedStart = (uint8_t *)malloc(fileSize - compressedDataOffset);
	Debug_check(compressedStart, "depklite - Out of memory!\n");

	fseek(fp, compressedDataOffset, SEEK_SET);
	fread(compressedStart, fileSize - compressedDataOffset, 1, fp);

	GetNextBit_Data getNextBitData;

	getNextBitData.getNextByteDat.compressedStart = compressedStart;

	// Buffer for the decompressed data (also little endian).
	memset(decompBuff, 0, buffsize);

	// Current position for inserting decompressed data.
	unsigned char *decompPtr = decompBuff;


	// A 16-bit array of compressed data.
	getNextBitData.bitArray = *(uint16_t *)getNextBitData.getNextByteDat.compressedStart;

	// Offset from start of compressed data (start at 2 because of the bit array).
	getNextBitData.getNextByteDat.byteIndex = 2;

	// Number of bits consumed in the current 16-bit array.
	getNextBitData.bitsRead = 0;

	// Continually read bit arrays from the compressed data and interpret each bit. 
	// Break once a compressed byte equals 0xFF in duplication mode.
	while (true)
	{
		if (vflag)
		{
			printBitData(getNextBitData.getNextByteDat.byteIndex, getNextBitData);
		}

		// Decide which mode to use for the current bit.
		if (getNextBit(&getNextBitData))
		{
			// "Duplication" mode.
			// Calculate which bytes in the decompressed data to duplicate and append.
			int copy = BitTree_get(bitTree1, &getNextBitData);

			// Calculate the number of bytes in the decompressed data to copy.
			uint16_t copyCount = 0;

			// Check for the special bit vector case "011100".
			if (copy == 25) // Special value
			{
				// Read a compressed byte.
				const uint8_t encryptedByte = getNextByte(&getNextBitData.getNextByteDat);

				if (encryptedByte == 0xFE)
				{
					// Skip the current bit.
					continue;
				}
				else if (encryptedByte == 0xFF)
				{
					// All done with decompression.
					verbose("Correctly received end of decompression marker.\n");
					break;
				}
				else
				{
					// Combine the compressed byte with 25 for the byte count.
					copyCount = encryptedByte + 25;
				}
			}
			else
			{
				// Use the decoded value from the first bit table.
				copyCount = copy;
			}

			// Calculate the offset in decompressed data. It is a two byte value.
			// The most significant byte is 0 by default.
			uint8_t mostSigByte = 0;

			// If the copy count is not 2, decode the most significant byte.
			if (copyCount != 2)
			{
				// Use the decoded value from the second bit table.
				mostSigByte = BitTree_get(bitTree2, &getNextBitData);
			}

			// Get the least significant byte of the two bytes.
			const uint8_t leastSigByte = getNextByte(&getNextBitData.getNextByteDat);

			// Combine the two bytes.
			const uint16_t offset = leastSigByte | (mostSigByte << 8);

			// Finally, duplicate the decompressed data using the calculated offset and size.
			// Note that memcpy or even memmove is NOT the right way,
			// since overlaps are possible
			unsigned char *duplicateBegin = decompPtr - offset;
			unsigned char *duplicateEnd = duplicateBegin + copyCount;
			for (unsigned char *p = duplicateBegin; p < duplicateEnd; ++p, ++decompPtr)
			{
				*decompPtr = *p;
			}
		}
		else
		{
			if (!useDecryption)
			{		
				// Usage of "Decryption" mode isn't required for Keen Dreams v1.00
				// Get next byte and then append it onto the decompressed data.
				*decompPtr++ = getNextByte(&getNextBitData.getNextByteDat);
			}
			else
			{
				// "Decryption" mode.
				// Read the next byte from the compressed data.
				const uint8_t encryptedByte = getNextByte(&getNextBitData.getNextByteDat);

				// Decrypt an encrypted byte with an XOR operation based
				// on the current bit index. "bitsRead" is between 0 and 15.
				// It is 0 if the 16th bit of the previous array was used to get here.
				const uint8_t decryptedByte = encryptedByte ^ (uint8_t)(16 - getNextBitData.bitsRead);

				// Append the decrypted byte onto the decompressed data.
				*decompPtr++ = decryptedByte;
			}
		}
		// Avoid buffer overrun
		if (decompPtr-decompBuff >= buffsize) { 
			fprintf(stderr, "depklite - Programming error. Buffer of %'d bytes was too small.\n", buffsize);
			break;
		}
	}

	free(compressedStart);
	return decompPtr-decompBuff;
}

char *itob(uint16_t i)
{
	static char binary[17];
	for (int t=15; t>=0; t--)
		{
			binary[t]=(i&1)+'0';
			i=i>>1;
		}
	binary[16]='\0';
	return binary;
}

int printBitData(int byteIndex, GetNextBit_Data data)
{

	//	const bool bit = (data.bitArray & (1 << data.bitsRead)) != 0;
	fprintf(stderr, "%8d\t%s\r", byteIndex, itob(data.bitArray));

	/* fprintf(stderr, "\t\t"); */
	/* for (int t=15; t > data.bitsRead; t--) */
	/* { */
	/* 	fprintf(stderr, " "); */
	/* } */
	/* fprintf(stderr, "^\n"); */

	return 0;
}



long estimateLength(FILE *fp) {
	// Use dozayon's algorithm to slightly overestimate size of decompressed data.
	// https://github.com/afritz1/OpenTESArena/blob/master/docs/pklite_specification.md
	fseek(fp, 0x61, SEEK_SET);
	short int  value;
	fread(&value, 2, 1, fp);
	verbose("value is %d\n" ,value);
	long length = value*0x10 - 0x450;
	rewind(fp);
	return length;
}



void usage() {
	printf(R"(depklite - extract data from DOS executables compressed with PKlite.

Usage:  depklite  [-d] [-o <outfile>]  foo.exe  [offset]

        foo.exe: name of the executable file to decompress.

Options:
        offset:  start of compressed data. (Default is 0).
        -d, --decrypt           Enable decryption of data.
        -o, --output <outfile>  Write to outfile instead of foo.exe.dep.
        -v, --verbose [=<0|1>]  Be more verbose.
)");
}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");

	char *inputFilename=NULL;

	// "640K ought to be enough for anyone." --Bill Gates
	int buffsize=4*1<<20; 
	unsigned char buffer[buffsize];						/* storage for uncompressed data */

	// Options
	bool useDecryption=false;			/* Not needed for Keen Dreams nor 3C5X9CFG */
	char *outputFilename=NULL;		/* Defaults to input name + ".dep" */

	// Offset of compressed data in the executable, which was
	// 800 for Keen Dreams v1.00 (KDREAMS.EXE) and
	// 752 for The Elder Scrolls Arena (A.EXE).
	// We default to zero, which is wrong, but won't truncate data (hopefully).
	int compressedDataOffset = 0;

	// Parse command line opts
	while (1) {
		int option_index = 0;
		static struct option long_options[] =
			{
			 {"output",  required_argument, 0, 'o'},
			 {"decrypt", no_argument,       0, 'd'},
			 {"help",    no_argument,       0, 'h'},
			 {"verbose", optional_argument, 0, 'v'},
			 {0,         0,                 0,  0 }
			};
		
		char c = getopt_long(argc, argv, "o:dhv::",
												 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			useDecryption = true;
			verbose("Enabling decryption\n");
			break;
		case 'o':
			asprintf(&outputFilename, "%s", optarg);
			break;
		case 'v':
			if (optarg == NULL)
				vflag=true;
			else
				vflag=atoi(optarg);
			break;
		case '?':
		case 'h':
			usage();
			exit(0);
			break;
		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind >= argc) {
		usage();
		exit(1);
	}
	else
	{
		// Mandatory filename
		asprintf(&inputFilename, argv[optind++]);

		// Optional offset
		if (optind < argc)
		{
			// Offset can be in decimal or hex (if it begins with 0x).
			sscanf(argv[optind++], "%i", &compressedDataOffset); 
		}
	}

	FILE *fp=fopen(inputFilename, "r");
	if (fp == NULL) {
		perror(inputFilename);
		exit(1);
	}

	buffsize=estimateLength(fp);
	buffsize=4*1<<20;
	verbose("Estimated size is %'d bytes\n", buffsize);
	//buffer=(uint8_t *)(malloc(buffsize));
	//Debug_check(buffer, "depklite - Failed to allocate decompression buffer!\n");

	buffsize=depklite_unpack(fp, buffer, buffsize, compressedDataOffset, useDecryption);

	if (outputFilename==NULL)
	{
		asprintf(&outputFilename, "%s.dep", inputFilename);
	}

	fp=fopen(outputFilename, "w");
	if (fp == NULL) {
		perror(argv[1]);
		exit(1);
	}
	
	printf("Writing %'d bytes to '%s'\n", buffsize, outputFilename);
	fwrite(buffer, buffsize, 1, fp);

	return 0;

}
