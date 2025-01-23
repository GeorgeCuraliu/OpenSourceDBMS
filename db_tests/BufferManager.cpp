#pragma once

#include "Column.h"
#include "BufferManager.h"
#include "Table.h"
#include "BitwiseHandler.h"
#include "Parameters.h"
#include "FileData.h"

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

	FileData* fileData = new FileData(table->rowSize, table->L1_registers, SEGMENT_SIZE);

	//is the last disk segment has free memory left, this if will fill it
	//first buffer to write that will continue the last one
	//will be used just if is the second L1_register flush of this table
	if ((128 % fileData->valuesPerSegment != 0 || fileData->bufferSize < 4096 ) && table->L1_registers) {

		int offset = 0;
		fileData->fillEmptySpace(offset);
		SetFilePointer(fileHandle, offset, 0, NULL);
		WriteFile(
			fileHandle,
			table->values,
			fileData->valuesToWrite * table->rowSize,
			&numberOfBytesWritten,
			NULL
		);
		
		std::cout << "wrote " << (int)numberOfBytesWritten / fileData->valueSize << " values in the segment with free memory -- table -- " << std::endl;
	}

	while (fileData->wroteValues < 128) {
		SetFilePointer(fileHandle, fileData->segmentPointer, NULL, 0);
		WriteFile(
			fileHandle,
			(uint8_t*)table->values + fileData->wroteValues * table->rowSize,
			fileData->bufferSize,
			&numberOfBytesWritten,
			NULL
		);
		(*fileData)++;
		std::cout << "wrote " << (int)numberOfBytesWritten << " bytes out of " << fileData->bufferSize << " -- total buckets wrote " << fileData->wroteValues << " -- segment pointer " << fileData->segmentPointer << " (" << fileData->segmentPointer / BUFFER_SIZE << ")" << std::endl;
	}




	void* bufferT = calloc(8, 1);
	SetFilePointer(fileHandle, 512 * 1 + 8 * 100, 0, NULL);
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
		void* dataToStore = malloc((column->data->numberOfBytes) * 128);
		//void* bloomFilter = calloc(BLOOM_FILTER_SIZE, 1);
		//void* offsets = malloc(128);
		void* metadata = calloc(L1_METADATA, 1);
		if (!dataToStore) std::cout << "could not allocate memory for dataToStore" << std::endl;

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

		//retrieve first the metadata, if it should exist
		if (table->L1_registers != 0) {
			DWORD readBytes;
			SetFilePointer(fileHandle, 0, 0, NULL);
			bool res = ReadFile(
				fileHandle,
				metadata,
				L1_METADATA,
				&readBytes,
				NULL
			);
			std::cout << "read " << readBytes << " bytes of the bloomFilter to update it" << std::endl;

		}

		column->data->GetAllValuesWithBloom(dataToStore, metadata, BLOOM_FILTER_SIZE, (char*)metadata + BLOOM_FILTER_SIZE + 32 + 128 * table->L1_registers);


		//write the bloom filter data
		DWORD numberOfBytesWritten = 0;
		//SetFilePointer(fileHandle, 0, 0, NULL);
		//WriteFile(
		//	fileHandle,
		//	bloomFilter,
		//	BLOOM_FILTER_SIZE,
		//	&numberOfBytesWritten,
		//	NULL
		//);
		//std::cout << "wrote " << numberOfBytesWritten << " bytes of bloom filter data out of " << BLOOM_FILTER_SIZE << std::endl;




		//void* tombstones = calloc(16, 1);
		for (int i = 512 + table->L1_registers * 16; i < table->L1_registers * 16 + 16; i++)
			((uint8_t*)metadata)[i] = 255;
		SetFilePointer(fileHandle, 0, 0, NULL);
		WriteFile(
			fileHandle,
			metadata,
			L1_METADATA,
			&numberOfBytesWritten,
			NULL
		);
		
		FileData* fileData = new FileData(column->data->numberOfBytes, table->L1_registers, SEGMENT_SIZE * 2);

		//is the last disk segment has free memory left, this if will fill it
		//first buffer to write that will continue the last one
		//will be used just if is the second L1_register flush of this table
		if ((128 % fileData->valuesPerSegment != 0 || fileData->totalValuesPerSegment > 128) && table->L1_registers > 0) {
			int offset = 0;
			fileData->fillEmptySpace(offset);
			SetFilePointer(fileHandle, offset, 0, NULL);
			WriteFile(
				fileHandle,
				dataToStore,
				fileData->valuesToWrite * (column->data->numberOfBytes),
				&numberOfBytesWritten,
				NULL
			);
			std::cout << "wrote " << fileData->wroteValues << " values in the segment with free memory" << std::endl;
		}

		while (fileData->wroteValues < 128) {
			SetFilePointer(fileHandle, fileData->segmentPointer, NULL, 0);
			bool res = WriteFile(
				fileHandle,
				(void*)((uint8_t*)dataToStore + fileData->wroteValues * fileData->valueSize),
				fileData->bufferSize,
				&numberOfBytesWritten,
				NULL
			);
			if (!res) std::cout << "could not write the buffer for file " << fileName << " ERROR " << GetLastError() << std::endl;
			(*fileData)++;
			std::cout << "column " << column->name << " wrote " << (int)numberOfBytesWritten << " bytes out of " << fileData->bufferSize << " -- total buckets wrote " << fileData->wroteValues << " -- segment pointer " << fileData->segmentPointer << " (" << fileData->segmentPointer / BUFFER_SIZE << ")" << std::endl;
		}

		void* bufferT = calloc(4, 1);
		if (!bufferT) std::cout << "could not allocate memory for bufferT" << std::endl;
		SetFilePointer(fileHandle, 512 * 2 + 4 * 100, 0 , NULL);
		bool res = ReadFile(
			fileHandle,
			bufferT,
			4,
			&numberOfBytesWritten,
			NULL
		);
		if (!res) std::cout << "test failed " << GetLastError() << std::endl;
		//void* offset = calloc(4, 1);
		void* value = malloc(4);
		//std::memcpy(offset, (char*)bufferT + 4, 1);
		std::memcpy(value, (char*)bufferT, 4);

		std::cout << "test values read " << (int)numberOfBytesWritten << "  --offset " << "  --value " << *(int*)value << std::endl;

		CloseHandle(fileHandle);

		//free(bloomFilter);
		free(dataToStore);
		free(fileName);
		free(bufferT);
		//free(offset);
		free(value);
		free(metadata);
		//free(tombstones);
	};
	table->columns.IterateWithCallback(storeColumns);
	delete fileData;
}




void BufferManager::SearchLevel1(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>& foundValues, int comparator){
	
	size_t tableNameLen = std::strlen(table->name);
	size_t columnNameLen = std::strlen(column->name);
	//void* tombstones = calloc(32, 1);
	void* metadata = malloc(L1_METADATA);
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

	DWORD readBytes;

	//SetFilePointer(fileHandle, BLOOM_FILTER_SIZE, 0, NULL);
	bool res; //= ReadFile(
	//	fileHandle,
	//	(char*)metadata + BLOOM_FILTER_SIZE,
	//	32,
	//	&readBytes,
	//	NULL
	//);

	//if (!res)std::cout << "coulnt read tombstones -- read " << readBytes << " -- " << GetLastError() << std::endl;

	SetFilePointer(fileHandle, 0, 0, NULL);
	res = ReadFile(
		fileHandle,
		metadata,
		L1_METADATA,
		&readBytes,
		NULL
	);

	std::cout << "read " << readBytes << " bytes of metadata out of " << BLOOM_FILTER_SIZE << std::endl;

	for (int i = 0; i < argumentsNumber; i++) {
		bool result = BloomFilter::CheckValue(metadata, values[i], column->data->numberOfBytes, BLOOM_FILTER_SIZE);
		if (result) std::cout << "value could be in L1 registers" << std::endl;
		else std::cout << "value not there" << std::endl;

		if (result || (comparator & (BIGGER | LESS | NOT))) {


			///////////////////////////////////////
			//WILL FAIL IN CASE REGISTER SIZE IS BIGGER THAN BUFFER_SIZE!!!!!!!!!!
			///////////////////////////////////////
			int bufferSize;
			if ((column->data->numberOfBytes) * 128 * table->L1_registers < BUFFER_SIZE)
				bufferSize = (column->data->numberOfBytes) * 128 * table->L1_registers;
			else
				bufferSize = BUFFER_SIZE;

			void* buffer = calloc(bufferSize, 1);
			DWORD numberOfBytesRead;

			SetFilePointer(fileHandle, SEGMENT_SIZE * 2, 0, 0);
			res = ReadFile(
				fileHandle,
				buffer,
				bufferSize,
				&numberOfBytesRead,
				NULL
				);

			for (int j = 0; j < 128 * table->L1_registers; j++) {
				//void* value = malloc(column->data->numberOfBytes);
				//void* offset = calloc(4, 1);
				if (!buffer) return;
				//std::memcpy(value, (uint8_t*)buffer + j * (column->data->numberOfBytes), column->data->numberOfBytes);
				//std::memcpy(offset, (uint8_t*)buffer + j * (column->data->numberOfBytes) + column->data->numberOfBytes, 1);

				int comp = VoidMemoryHandler::COMPARE((uint8_t*)buffer + j, values[i], column->data->dataType);
				if (comp & comparator && BitwiseHandler::checkBit((uint8_t*)metadata + 512, j)) {
					std::cout << "found match at offset " << *(int*)((char*)metadata)[512 + 32 + j] << " " << j << std::endl;
					foundValues.push_back(*(int*)((char*)metadata)[512 + 32 + j]);
				}
				//free(value);
				//free(offset);
			}
			free(buffer);
		}
	}
	free(fileName);
	free(metadata);
	CloseHandle(fileHandle);
	//free(tombstones);
}

void BufferManager::DeleteValuesLevel1(Table* table, std::vector<int>& deleteValues){
	auto deleteFromRegistry = [table, &deleteValues](Column* column) {

		size_t tableNameLen = std::strlen(table->name);
		size_t columnNameLen = std::strlen(column->name);
		char* fileName = (char*)calloc(tableNameLen + columnNameLen + 2, sizeof(char)); // +1 for null terminator
		if (!fileName) {
			std::cerr << "Memory allocation failed!" << std::endl;
			return;
		}
		std::memcpy(fileName, table->name, tableNameLen);
		fileName[tableNameLen] = '&';
		std::memcpy(fileName + tableNameLen + 1, column->name, columnNameLen);
		fileName[tableNameLen + columnNameLen + 1] = '\0';

		ClearTombstones(fileName, deleteValues, BLOOM_FILTER_SIZE);
		
		free(fileName);
		};
	table->columns.IterateWithCallback(deleteFromRegistry);

	ClearTombstones(table->name, deleteValues, 0);
}



void BufferManager::ClearTombstones(char* fileName, std::vector<int>& deleteValues, int offset){
	HANDLE fileHandle = nullptr;
	fileHandle = CreateFileA(
		(LPCSTR)fileName,
		GENERIC_ALL,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	SetFilePointer(fileHandle, 0, 0, NULL);

	void* tombstones = calloc(32, 1);
	DWORD readBytes = 0;
	SetFilePointer(fileHandle, offset, 0, NULL);
	ReadFile(
		fileHandle,
		tombstones,
		32,
		&readBytes,
		NULL
	);
	std::cout << "read " << readBytes << " bytes out of 32 from " << fileName << std::endl;

	std::cout << "set tombstones to 0 at ";
	for (auto it = deleteValues.begin(); it != deleteValues.end(); it++) {
		std::cout << *it << " ";
		BitwiseHandler::clearBit((uint8_t*)tombstones, *it);
		//it = deleteValues.erase(it);
	}
	std::cout << "" << std::endl;

	readBytes = 0;
	SetFilePointer(fileHandle, BLOOM_FILTER_SIZE, 0, NULL);
	WriteFile(
		fileHandle,
		tombstones,
		32,
		&readBytes,
		NULL
	);
	CloseHandle(fileHandle);
	free(tombstones);
}