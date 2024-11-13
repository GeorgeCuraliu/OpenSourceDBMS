#pragma once

#include "Column.h"
#include "BufferManager.h"
#include "Table.h"
#include "BitwiseHandler.h"
#include "Parameters.h"

void BufferManager::StoreLevel1(Table* table) {

	char* fileName = (char*)calloc(sizeof(table->name) + 1, 1);
	if (!fileName) return;
	std::memcpy(fileName, table->name, std::strlen(table->name));
	fileName[std::strlen(table->name)] = '-';
	fileName[std::strlen(table->name) + 1] = table->L1_registers + 48;
	fileName[std::strlen(table->name) + 2] = '\0';


	HANDLE fileHandle = CreateFileA(
		(LPCSTR)(table->name),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if(fileHandle == INVALID_HANDLE_VALUE)	std::cerr << "CreateFileA failed, error: " << GetLastError() << std::endl;


	//write tombstones first
	DWORD numberOfBytesWritten = 0;
	SetFilePointer(fileHandle, 16 * table->L1_registers, NULL, 0);
	WriteFile(
		fileHandle,
		table->freeMemory,
		16,
		&numberOfBytesWritten,
		NULL
	);
	std::cout << "wrote " << (int)numberOfBytesWritten << " bytes of tombstones out of 16" << std::endl;


	//write the actual data
	int totalBucketsPerSegment = floor(BUFFER_SIZE / double(table->rowSize));
	int bucketsPerSegment = floor(BUFFER_SIZE / double(table->rowSize));
	if (bucketsPerSegment > 128) bucketsPerSegment = 128;
	int bufferSize = table->rowSize * bucketsPerSegment;
	int wroteBuckets = 0;
	int segmentPointer = BUFFER_SIZE * (table->L1_registers * ceil(128.f / (double)bucketsPerSegment)) + SEGMENT_SIZE;// SEGMENT_SIZE for the first segment wich is tomstones and bloom filter data


	if ((128 % bucketsPerSegment != 0  > 0 || bufferSize < 4096 ) && table->L1_registers) {
		int segment = 128 / bucketsPerSegment;
		int writtenValues;
		if (bucketsPerSegment == 128) writtenValues = bucketsPerSegment;
		else writtenValues = 128 % bucketsPerSegment;
		int offset = BUFFER_SIZE * (segment - 1) + writtenValues * table->rowSize + SEGMENT_SIZE;//-1 for the first segment, wich is metadata
		int valuesToWrite = totalBucketsPerSegment - writtenValues;
		if (valuesToWrite > 128) valuesToWrite = 128;
		SetFilePointer(fileHandle, offset, 0, NULL);
		WriteFile(
			fileHandle,
			table->values,
			valuesToWrite * table->rowSize,
			&numberOfBytesWritten,
			NULL
		);
		wroteBuckets += valuesToWrite;
		std::cout << "wrote " << writtenValues << " values in the segment with free memory -- table -- " << std::endl;
	}

	while (wroteBuckets < 128) {
		SetFilePointer(fileHandle, segmentPointer, NULL, 0);
		void* buffer = malloc(bufferSize);
		if (!buffer) return;
		std::memcpy(buffer, (uint8_t*)table->values + wroteBuckets * table->rowSize, bufferSize);
		WriteFile(
			fileHandle,
			buffer,
			bufferSize,
			&numberOfBytesWritten,
			NULL
		);
		wroteBuckets += bucketsPerSegment;
		std::cout << "wrote " << (int)numberOfBytesWritten << " bytes out of " << bufferSize << " -- total buckets wrote " << wroteBuckets << " -- segment pointer " << segmentPointer << " (" << segmentPointer / BUFFER_SIZE << ")" << std::endl;
		segmentPointer += BUFFER_SIZE;
		free(buffer);
	}


	void* bufferT = calloc(8, 1);
	SetFilePointer(fileHandle, 512 * 1 + 8 * 4, 0, NULL);
	bool res = ReadFile(
		fileHandle,
		bufferT,
		8,
		&numberOfBytesWritten,
		NULL
	);
	if (!res) std::cout << "test failed " << GetLastError() << std::endl;
	void* v1 = malloc(4);
	void* v2 = malloc(4);
	std::memcpy(v1, (char*)bufferT, 4);
	std::memcpy(v2, (char*)bufferT + 4, 4);

	std::cout << "test values read " << numberOfBytesWritten << "  --v1 " << *(int*)v1 << "  --v2 " << *(int*)v2 << std::endl;
	free(v1);
	free(v2);


	CloseHandle(fileHandle);
	free(fileName);
	free(bufferT);

	auto storeColumns = [table](Column* column) {
		void* dataToStore = malloc((column->data->numberOfBytes + 1) * 128);
		void* bloomFilter = calloc(SEGMENT_SIZE, 1);
		if (!dataToStore) std::cout << "could not allocate memory for dataToStore" << std::endl;
		//column->data->GetAllValues(dataToStore);

		size_t tableNameLen = std::strlen(table->name);
		size_t columnNameLen = std::strlen(column->name);
		char* fileName = (char*)calloc(tableNameLen + columnNameLen + 2, sizeof(char)); // +1 for null terminator
		if (!fileName) {
			std::cerr << "Memory allocation failed!" << std::endl;
			return -1;  // Handle allocation failure
		}

		std::memcpy(fileName, table->name, tableNameLen);
		fileName[tableNameLen] = '&';
		std::memcpy(fileName + tableNameLen + 1, column->name, columnNameLen);
		fileName[tableNameLen + columnNameLen + 1] = '\0';

		HANDLE fileHandle = CreateFileA(
			(LPCSTR)(fileName),
			GENERIC_ALL,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (fileHandle == INVALID_HANDLE_VALUE) std::cout << "couldnt open the file " << fileName << " ERROR " << GetLastError() << std::endl;

		//retrieve first bloom filtre, if it should exist
		if (table->L1_registers != 0) {
			DWORD readBytes;
			SetFilePointer(fileHandle, 0, 0, NULL);
			bool res = ReadFile(
				fileHandle,
				bloomFilter,
				SEGMENT_SIZE,
				&readBytes,
				NULL
			);
			std::cout << "read " << readBytes << " bytes of the bloomFilter to update it" << std::endl;

		}

		column->data->GetAllValuesWithBloom(dataToStore, bloomFilter, SEGMENT_SIZE);


		//write the bloom filter data
		DWORD numberOfBytesWritten = 0;
		SetFilePointer(fileHandle, 0, 0, NULL);
		WriteFile(
			fileHandle,
			bloomFilter,
			SEGMENT_SIZE,
			&numberOfBytesWritten,
			NULL
		);
		std::cout << "wrote " << numberOfBytesWritten << " bytes of bloom filter data out of " << SEGMENT_SIZE << std::endl;
		
		int totalValuesPerSegment = floor(BUFFER_SIZE / double(column->data->numberOfBytes + 1));
		int valuesPerSegment;
		if (totalValuesPerSegment > 128) valuesPerSegment = 128;
		else valuesPerSegment = totalValuesPerSegment;
		int bufferSize = (column->data->numberOfBytes + 1) * valuesPerSegment;
		int wroteValues = 0;
		int segmentPointer = BUFFER_SIZE * (table->L1_registers * ceil(128.f / (double)valuesPerSegment)) + SEGMENT_SIZE;//the next unused disk fragment

		//is the last disk segment has free memory left, this if will fill it
		//first buffer to write that will continue the last one
		//will be used just if is the second L1_register flush of this table
		if ((128 % valuesPerSegment != 0 || totalValuesPerSegment > 128) && table->L1_registers > 0) {
			int segment = 128 / valuesPerSegment;
			int writtenValues;
			if (valuesPerSegment == 128) writtenValues = valuesPerSegment;
			else writtenValues = 128 % valuesPerSegment;
			int offset = BUFFER_SIZE * (segment - 1) + writtenValues * (column->data->numberOfBytes + 1) + SEGMENT_SIZE;
			int valuesToWrite = totalValuesPerSegment - writtenValues;
			if (valuesToWrite > 128) valuesToWrite = 128;
			SetFilePointer(fileHandle, offset, 0, NULL);
			WriteFile(
				fileHandle,
				dataToStore,
				valuesToWrite * (column->data->numberOfBytes + 1),
				&numberOfBytesWritten,
				NULL
			);
			wroteValues += valuesToWrite;
			std::cout << "wrote " << writtenValues << " values in the segment with free memory" << std::endl;
		}

		while (wroteValues < 128) {
			SetFilePointer(fileHandle, segmentPointer, NULL, 0);
			static int valuesToWrite;
			if (wroteValues + valuesPerSegment <= 128) {
				valuesToWrite = valuesPerSegment;
			}
			else {//the case for remainig values of a buffer
				valuesToWrite = 128 - wroteValues;
			}
			bool res = WriteFile(
				fileHandle,
				(void*)((uint8_t*)dataToStore + wroteValues * (column->data->numberOfBytes + 1)),
				valuesToWrite * (column->data->numberOfBytes + 1),
				&numberOfBytesWritten,
				NULL
			);
			if (!res) std::cout << "could not write the buffer for file " << fileName << " ERROR " << GetLastError() << std::endl;
			wroteValues += valuesToWrite;
			std::cout << "column " << column->name << " wrote " << (int)numberOfBytesWritten << " bytes out of " << bufferSize << " -- total buckets wrote " << wroteValues << " -- segment pointer " << segmentPointer << " (" << segmentPointer / BUFFER_SIZE << ")" << std::endl;
			segmentPointer += BUFFER_SIZE;
		}

		void* bufferT = calloc(5, 1);
		if (!bufferT) std::cout << "could not allocate memory for bufferT" << std::endl;
		SetFilePointer(fileHandle, 512 * 1 + 5 * 100, 0 , NULL);
		bool res = ReadFile(
			fileHandle,
			bufferT,
			5,
			&numberOfBytesWritten,
			NULL
		);
		if (!res) std::cout << "test failed " << GetLastError() << std::endl;
		void* offset = calloc(4, 1);
		void* value = malloc(4);
		std::memcpy(offset, (char*)bufferT + 4, 1);
		std::memcpy(value, (char*)bufferT, 4);

		std::cout << "test values read " << numberOfBytesWritten << "  --offset " << *(int*)offset << "  --value " << *(int*)value << std::endl;

		CloseHandle(fileHandle);

		free(bloomFilter);
		free(dataToStore);
		free(fileName);
		free(bufferT);
		free(offset);
		free(value);
	};
	table->columns.IterateWithCallback(storeColumns);

}




void BufferManager::SearchLevel1(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>& foundValues, int comparator){
	
	size_t tableNameLen = std::strlen(table->name);
	size_t columnNameLen = std::strlen(column->name);
	char* fileName = (char*)calloc(tableNameLen + columnNameLen + 2, sizeof(char)); // +1 for null terminator
	if (!fileName) {
		std::cerr << "Memory allocation failed!" << std::endl;
		return;  // Handle allocation failure
	}

	std::memcpy(fileName, table->name, tableNameLen);
	fileName[tableNameLen] = '&';
	std::memcpy(fileName + tableNameLen + 1, column->name, columnNameLen);
	fileName[tableNameLen + columnNameLen + 1] = '\0';

	std::cout << "searching in file " << fileName << std::endl;

	HANDLE fileHandle;
	fileHandle = CreateFileA(
		(LPCSTR)fileName,
		GENERIC_ALL,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	void* bloomBuffer = calloc(SEGMENT_SIZE, 1);
	DWORD readBytes;

	bool res = ReadFile(
		fileHandle,
		bloomBuffer,
		SEGMENT_SIZE,
		&readBytes,
		NULL
	);

	std::cout << "read " << readBytes << " bytes of bloom filter out of " << SEGMENT_SIZE << std::endl;

	for (int i = 0; i < argumentsNumber; i++) {
		bool result = BloomFilter::CheckValue(bloomBuffer, values[i], column->data->numberOfBytes, SEGMENT_SIZE);
		if (result) std::cout << "value could be in L1 registers" << std::endl;
		else std::cout << "value not there" << std::endl;

		if (result || (comparator & (BIGGER | LESS | NOT))) {

			int bufferSize;
			if ((column->data->numberOfBytes + 1) * 128 * table->L1_registers < BUFFER_SIZE)
				bufferSize = (column->data->numberOfBytes + 1) * 128 * table->L1_registers;
			else
				bufferSize = BUFFER_SIZE;

			void* buffer = calloc(bufferSize, 1);
			DWORD numberOfBytesRead;

			SetFilePointer(fileHandle, SEGMENT_SIZE, 0, 0);
			res = ReadFile(
				fileHandle,
				buffer,
				bufferSize,
				&numberOfBytesRead,
				NULL
				);

			for (int j = 0; j < 128 * table->L1_registers; j++) {
				void* value = malloc(column->data->numberOfBytes);
				void* offset = calloc(4, 1);
				if (!value || !offset || !buffer) return;
				std::memcpy(value, (uint8_t*)buffer + j * (column->data->numberOfBytes + 1), column->data->numberOfBytes);
				std::memcpy(offset, (uint8_t*)buffer + j * (column->data->numberOfBytes + 1) + column->data->numberOfBytes, 1);

				int comp = VoidMemoryHandler::COMPARE(value, values[i], column->data->dataType);
				if (comp & comparator) {
					std::cout << "found match at offset " << *(int*)offset << std::endl;
					foundValues.push_back(*(int*)offset);
				}
				free(value);
				free(offset);
			}
			free(buffer);
		}
	}
	free(fileName);
	free(bloomBuffer);
	CloseHandle(fileHandle);
}