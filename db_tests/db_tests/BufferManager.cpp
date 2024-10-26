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
		(LPCSTR)(fileName),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if(fileHandle == INVALID_HANDLE_VALUE)std::cerr << "CreateFileA failed, error: " << GetLastError() << std::endl;

	void* buffer = malloc(128 * table->rowSize + 16); // + 16 for freeMemory array
	if (!buffer) return;
	std::memcpy(buffer, table->freeMemory, 16);
	std::memcpy((char*)buffer + 16, table->values, table->rowSize * 128);

	std::cout << "checking buffer  ";
	for (int i = 0; i <= 127; i++)
		std::cout << BitwiseHandler::checkBit((uint8_t*)buffer, i);
	std::cout << "" << std::endl;


	DWORD numberOfBytesWritten = 0;
	bool result = WriteFile(
		fileHandle,
		buffer,
		table->rowSize * 128 + 16,
		&numberOfBytesWritten,
		NULL
	);

	if(!result)std::cerr << "WriteFile failed, error: " << GetLastError() << std::endl;

	std::cout << "wrote " << (int)numberOfBytesWritten << " out of " << table->rowSize * 128 + 16 << std::endl;



	//CHECK IF THE SAVED BYTES ARE CORRECT
	SetFilePointer(fileHandle, 0, NULL, 0);

	// Allocate memory for the last 8 bytes
	void* last8Bytes = malloc(16);
	if (!last8Bytes) {
		std::cerr << "Failed to allocate memory" << std::endl;
		CloseHandle(fileHandle);
	}

	// Read the last 8 bytes into the allocated memory
	DWORD bytesRead;
	result = ReadFile(fileHandle, last8Bytes, 16, &bytesRead, NULL);

	if (!result)std::cout<<GetLastError()<<std::endl;

	std::cout << "checking read bytes";
	for (int i = 0; i <= 127; i++)
		std::cout << BitwiseHandler::checkBit((uint8_t*)last8Bytes, i);
	std::cout << "" << std::endl;

	CloseHandle(fileHandle);

	auto storeColumns = [table](Column* column) {
		void* buffer = calloc((column->data->numberOfBytes + 1) * 128, sizeof(char));
		column->data->GetAllValues(buffer);

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
		fileName[tableNameLen + columnNameLen + 1] = '-';
		fileName[tableNameLen + columnNameLen + 2] = table->L1_registers + 48;
		fileName[tableNameLen + columnNameLen + 3] = '\0';

		// fileName already has the null terminator due to strcat_s
		
		std::cout << "storing data of column " << fileName;

		HANDLE fileHandle = CreateFileA(
			(LPCSTR)(fileName),
			GENERIC_ALL,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		DWORD numberOfBytesWritten = 0;
		WriteFile(fileHandle, buffer, (column->data->numberOfBytes + 1) * 128, &numberOfBytesWritten, NULL);
		std::cout << "   " << (int)numberOfBytesWritten << " bytes wrote" << std::endl;

		CloseHandle(fileHandle);

	};
	table->columns.IterateWithCallback(storeColumns);

}