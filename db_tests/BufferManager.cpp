#pragma once

#include "BufferManager.h"
#include "Table.h"
#include "BitwiseHandler.h"

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

	if(fileHandle == INVALID_HANDLE_VALUE)std::cerr << "CreateFileA failed, error: " << GetLastError() << std::endl;

	//void* buffer = malloc(128 * table->rowSize + 16); // + 16 for freeMemory array
	//if (!buffer) return;
	//std::memcpy(buffer, table->freeMemory, 16);
	//std::memcpy((char*)buffer + 16, table->values, table->rowSize * 128);

	//std::cout << "checking buffer  ";
	//for (int i = 0; i <= 127; i++)
	//	std::cout << BitwiseHandler::checkBit((uint8_t*)buffer, i);
	//std::cout << "" << std::endl;


	//DWORD numberOfBytesWritten = 0;
	//bool result = WriteFile(
	//	fileHandle,
	//	buffer,
	//	table->rowSize * 128 + 16,
	//	&numberOfBytesWritten,
	//	NULL
	//);

	//if(!result)std::cerr << "WriteFile failed, error: " << GetLastError() << std::endl;

	//std::cout << "wrote " << (int)numberOfBytesWritten << " out of " << table->rowSize * 128 + 16 << std::endl;



	////CHECK IF THE SAVED BYTES ARE CORRECT
	//SetFilePointer(fileHandle, 0, NULL, 0);

	//// Allocate memory for the last 8 bytes
	//void* last8Bytes = malloc(16);
	//if (!last8Bytes) {
	//	std::cerr << "Failed to allocate memory" << std::endl;
	//	CloseHandle(fileHandle);
	//}

	//// Read the last 8 bytes into the allocated memory
	//DWORD bytesRead;
	//result = ReadFile(fileHandle, last8Bytes, 16, &bytesRead, NULL);

	//if (!result)std::cout<<GetLastError()<<std::endl;

	//std::cout << "checking read bytes";
	//for (int i = 0; i <= 127; i++)
	//	std::cout << BitwiseHandler::checkBit((uint8_t*)last8Bytes, i);
	//std::cout << "" << std::endl;


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
	int bucketPerSegment = floor(512.f / double(table->rowSize));
	int bufferSize = table->rowSize * bucketPerSegment;
	int wroteBuckets = 0;
	int segmentPointer = 512 * (table->L1_registers * ceil(128.f / (double)bucketPerSegment) + 1);// + 1 for the first segment wich is tomstones and bloom filter data
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
		wroteBuckets += bucketPerSegment;
		std::cout << "wrote " << (int)numberOfBytesWritten << " bytes out of " << bufferSize << " -- total buckets wrote " << wroteBuckets << " -- segment pointer " << segmentPointer << " (" << segmentPointer / 512 << ")" << std::endl;
		segmentPointer += 512;
	}


	void* bufferT = calloc(8, 1);
	SetFilePointer(fileHandle, 512 * 2 + 8 * 6, 0, NULL);
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


	CloseHandle(fileHandle);

	auto storeColumns = [table](Column* column) {
		void* dataToStore = malloc((column->data->numberOfBytes + 1) * 128);
		if (!dataToStore) std::cout << "could not allocate memory for dataToStore" << std::endl;
		column->data->GetAllValues(dataToStore);

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

		DWORD numberOfBytesWritten = 0;
		int valuesPerSegment = floor(512.f / double(column->data->numberOfBytes + 1));
		int bufferSize = (column->data->numberOfBytes + 1) * valuesPerSegment;
		int wroteValues = 0;
		int segmentPointer = 512 * (table->L1_registers * ceil(128.f / (double)valuesPerSegment) + 1);// + 1 for the first segment wich is tomstones and bloom filter data
		while (wroteValues < 128) {
			SetFilePointer(fileHandle, segmentPointer, NULL, 0);
			static int valuesToWrite;
			if (wroteValues + valuesPerSegment <= 128) {
				valuesToWrite = valuesPerSegment;
			}
			else {
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
			std::cout << "column " << column->name << " wrote " << (int)numberOfBytesWritten << " bytes out of " << bufferSize << " -- total buckets wrote " << wroteValues << " -- segment pointer " << segmentPointer << " (" << segmentPointer / 512 << ")" << std::endl;
			segmentPointer += 512;
		}

		void* bufferT = calloc(5, 1);
		if (!bufferT) std::cout << "could not allocate memory for bufferT" << std::endl;
		SetFilePointer(fileHandle, 512 * 2 + 5 * 0, 0 , NULL);
		bool res = ReadFile(
			fileHandle,
			bufferT,
			5,
			&numberOfBytesWritten,
			NULL
		);
		if (!res) std::cout << "test failed " << GetLastError() << std::endl;
		void* offset = malloc(1);
		void* value = malloc(4);
		std::memcpy(offset, (char*)bufferT + 4, 1);
		std::memcpy(value, (char*)bufferT, 4);

		std::cout << "test values read " << numberOfBytesWritten << "  --offset " << *(int*)offset << "  --value " << *(int*)value << std::endl;

		CloseHandle(fileHandle);

		//free(dataToStore);
		free(fileName);
		free(bufferT);
		free(offset);
		free(value);
	};
	table->columns.IterateWithCallback(storeColumns);

}