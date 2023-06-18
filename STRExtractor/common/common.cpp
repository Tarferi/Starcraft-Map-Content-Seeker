#include "common.h"


void compress(char* data, unsigned int length, char** outputData, unsigned int* outputLength, bool* error) {
	compress(data, length, outputData, outputLength, error, 10);
}

void compress(char* data, unsigned int length, char** outputData, unsigned int* outputLength, bool* error, int level) {
	unsigned long uncompressedLength = length;
	unsigned long compressedLength = mz_compressBound(uncompressedLength);
	char* uncompressedData = data;
	MALLOC_N(compressedData, unsigned char, uncompressedLength + 4, {*error = true; return; });


	// Compress the string.
	int cmp_status = mz_compress2(compressedData + 4, &compressedLength, ( const unsigned char*) uncompressedData, uncompressedLength, level); // Maximum compression
	if (cmp_status != _MZ_OK) {
		free(compressedData);
		*error = true;
		return;
	}
	compressedData[0] = ( char) ((length >> 24) & 0xff);
	compressedData[1] = ( char) ((length >> 16) & 0xff);
	compressedData[2] = ( char) ((length >> 8) & 0xff);
	compressedData[3] = ( char) ((length >> 0) & 0xff);

	*outputData = ( char*) compressedData;
	*outputLength = ( unsigned int) compressedLength + 4;
}

void decompress(char* data, unsigned int dataLength, char** outputData, unsigned int* outputLength, bool* error) {
	unsigned char* udata = ( unsigned char*) data;
	unsigned long uncompressedLength = (udata[0] << 24) + (udata[1] << 16) + (udata[2] << 8) + (udata[3] << 0);
	unsigned long compressedLength = dataLength - 4;
	char* compressedData = data + 4;
	MALLOC_N(decompressedData, unsigned char, uncompressedLength, {*error = true; return; });

	// Compress the string.
	int cmp_status = mz_uncompress(decompressedData, &uncompressedLength, ( const unsigned char*) (compressedData), compressedLength);
	if (cmp_status != _MZ_OK) {
		free(decompressedData);
		*error = true;
		return;
	}
	*outputData = ( char*) decompressedData;
	*outputLength = ( unsigned int) uncompressedLength;
}

void destroyFileArray(Array<MapFileStr*>* array) {
	for (unsigned int mapFileIndex = 0; mapFileIndex < array->getSize(); mapFileIndex++) {
		MapFileStr* mf = array->get(mapFileIndex);
		if (mf->fileName != nullptr) {
			free(mf->fileName);
		}
		if (mf->contents != nullptr) {
			free(mf->contents);
		}
		free(mf);
	}
	delete(array);
}