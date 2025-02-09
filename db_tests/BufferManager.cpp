#pragma once

#include "Column.h"
#include "BufferManager.h"
#include "Table.h"
#include "BitwiseHandler.h"
#include "Parameters.h"
#include "FileData.h"

void BufferManager::StoreLevel1(Table* table) {

	//char* fileName = (char*)calloc(sizeof(table->name) + 1, 1);
	//if (!fileName) return;
	//std::memcpy(fileName, table->name, std::strlen(table->name));
	//fileName[std::strlen(table->name)] = '-';
	//fileName[std::strlen(table->name) + 1] = table->L1_registers + 48;
	//fileName[std::strlen(table->name) + 2] = '\0';


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
	//free(fileName);
	free(bufferT);



	auto storeColumns = [table](Column* column) {
		void* dataToStore = malloc((column->data->numberOfBytes) * 128);
		//void* bloomFilter = calloc(L1_BLOOM_FILTER_SIZE, 1);
		//void* offsets = malloc(128);
		void* metadata = calloc(L1_METADATA_SIZE, 1);
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
				L1_METADATA_SIZE,
				&readBytes,
				NULL
			);
			std::cout << "read " << readBytes << " bytes of the bloomFilter to update it" << std::endl;

		}

		column->data->GetAllValuesWithBloom(dataToStore, metadata, L1_BLOOM_FILTER_SIZE, (char*)metadata + L1_BLOOM_FILTER_SIZE + 32 + 128 * table->L1_registers);


		//write the bloom filter data
		DWORD numberOfBytesWritten = 0;
		//SetFilePointer(fileHandle, 0, 0, NULL);
		//WriteFile(
		//	fileHandle,
		//	bloomFilter,
		//	L1_BLOOM_FILTER_SIZE,
		//	&numberOfBytesWritten,
		//	NULL
		//);
		//std::cout << "wrote " << numberOfBytesWritten << " bytes of bloom filter data out of " << L1_BLOOM_FILTER_SIZE << std::endl;




		//void* tombstones = calloc(16, 1);
		for (int i = L1_BLOOM_FILTER_SIZE + table->L1_registers * 16; i < L1_BLOOM_FILTER_SIZE + table->L1_registers * 16 + 16; i++)
			((uint8_t*)metadata)[i] = 255;
		SetFilePointer(fileHandle, 0, 0, NULL);
		WriteFile(
			fileHandle,
			metadata,
			L1_METADATA_SIZE,
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
	void* metadata = malloc(L1_METADATA_SIZE);
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

	//SetFilePointer(fileHandle, L1_BLOOM_FILTER_SIZE, 0, NULL);
	bool res; //= ReadFile(
	//	fileHandle,
	//	(char*)metadata + L1_BLOOM_FILTER_SIZE,
	//	32,
	//	&readBytes,
	//	NULL
	//);

	//if (!res)std::cout << "coulnt read tombstones -- read " << readBytes << " -- " << GetLastError() << std::endl;

	SetFilePointer(fileHandle, 0, 0, NULL);
	res = ReadFile(
		fileHandle,
		metadata,
		L1_METADATA_SIZE,
		&readBytes,
		NULL
	);

	std::cout << "read " << readBytes << " bytes of metadata out of " << L1_BLOOM_FILTER_SIZE << std::endl;

	bool check = false;

	for (int i = 0; i < argumentsNumber; i++) {
		bool result = BloomFilter::CheckValue(metadata, values[i], column->data->numberOfBytes, L1_BLOOM_FILTER_SIZE);
		if (result) std::cout << "value could be in L1 registers" << std::endl;
		else std::cout << "value not there" << std::endl;
		if (result || (comparator & (BIGGER | LESS | NOT))) check = true;

	}

	if (check) {

		FileData* fileData = new FileData(column->data->numberOfBytes, table->L1_registers, SEGMENT_SIZE * 2);

			///////////////////////////////////////
			//WILL FAIL IN CASE REGISTER SIZE IS BIGGER THAN BUFFER_SIZE!!!!!!!!!!
			///////////////////////////////////////
 
			//int bufferSize;
			//if ((column->data->numberOfBytes) * 128 * table->L1_registers < BUFFER_SIZE)
			//	bufferSize = (column->data->numberOfBytes) * 128 * table->L1_registers;
			//else
			//	bufferSize = BUFFER_SIZE;
		int bufferSize = fileData->totalValuesPerSegment * column->data->numberOfBytes;
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
				//std::cout<<
			int index = j % fileData->totalValuesPerSegment;
			for (int i = 0; i < argumentsNumber; i++) {
				int comp = VoidMemoryHandler::COMPARE((uint8_t*)buffer + index * column->data->numberOfBytes, values[i], column->data->dataType);
				if (comp & comparator && BitwiseHandler::checkBit((uint8_t*)metadata + 512, j)) {
					std::cout << "found match at offset " << (int)((uint8_t*)metadata)[512 + 32 + j] << " " << j << std::endl;
					foundValues.push_back((int)((uint8_t*)metadata)[512 + 32 + j]);
				}
			}
				std::cout <<((int*)buffer)[index] << std::endl;
				//free(value);
				//free(offset);
				//if(j)
			if (j != 0 && j % fileData->totalValuesPerSegment == 0) {
				SetFilePointer(fileHandle, fileData->segmentPointer, 0, 0);
				res = ReadFile(
					fileHandle,
					buffer,
					fileData->bufferSize,
					&numberOfBytesRead,
					NULL
				);
				(*fileData)++;
			}
		}
		free(buffer);
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

		ClearTombstones(fileName, deleteValues, L1_BLOOM_FILTER_SIZE, table->L1_registers * 128, true);
		
		free(fileName);
		};
	table->columns.IterateWithCallback(deleteFromRegistry);

	ClearTombstones(table->name, deleteValues, 0);
}


//->search offsets is a optional parameter that will decide if the tombstones`s order is represented by some offsets or by their disk order
//->number of values is used in case the offsets are determined by another order than the disk order and it is needed to know how much space too allocate for metadata
void BufferManager::ClearTombstones(char* fileName, std::vector<int>& deleteValues, int tombstonesOffset, int numberOfValues, bool searchOffsets){
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
	SetFilePointer(fileHandle, L1_BLOOM_FILTER_SIZE, 0, NULL);

	int size = numberOfValues + 32;
	void* metadata = malloc(size);
	DWORD readBytes = 0;
	SetFilePointer(fileHandle, tombstonesOffset, 0, NULL);
	ReadFile(
		fileHandle,
		metadata,
		size,
		&readBytes,
		NULL
	);
	std::cout << "read " << readBytes << " bytes out of 32 from " << fileName << std::endl;

	std::cout << "set tombstones to 0 at ";
	for (auto it = deleteValues.begin(); it != deleteValues.end(); it++) {
		if (searchOffsets)
			for (int i = 0; i < numberOfValues; i++) {
				if (*it == (int)((char*)metadata)[32 + i]) {
					std::cout << *it << " ";
					BitwiseHandler::clearBit((uint8_t*)metadata, i);
				}
			}
		else {
			BitwiseHandler::clearBit((uint8_t*)metadata, *it);
			std::cout << *it << " ";
		}
			
		//it = deleteValues.erase(it);
	}
	std::cout << "" << std::endl;

	readBytes = 0;
	SetFilePointer(fileHandle, L1_BLOOM_FILTER_SIZE, 0, NULL);
	WriteFile(
		fileHandle,
		metadata,
		32,
		&readBytes,
		NULL
	);
	CloseHandle(fileHandle);
	free(metadata);
}



int BufferManager::GetL2Size(long long registerNumber, int valueSize, int metadata){
	int valuesPerBuffer = BUFFER_SIZE / valueSize;
	int valuesPerRegsiter = 128 * 3;
	int register_size = int(valuesPerRegsiter / valuesPerBuffer) * BUFFER_SIZE + ceil(double(valuesPerRegsiter % valuesPerBuffer * valueSize) / double(SEGMENT_SIZE)) * SEGMENT_SIZE + metadata;
	return register_size;
}


//return -1 if there is no register free
long long BufferManager::GetL2FreeRegister(long long registerNumber, int valueSize, HANDLE fileHandle, int metadata){
	if (!registerNumber) return -1;
	int numberOfTombstones = ceil(double(registerNumber) / 4096);
	int registerSize = GetL2Size(registerNumber, valueSize,metadata);
	long long offset = 0;
	void* tombstones = malloc(512);
	DWORD readBytes = 0;

	for (int i = 0; i < numberOfTombstones; i++) {
		SetFilePointer(fileHandle, offset, 0, NULL);
		ReadFile(
			fileHandle,
			tombstones,
			512,
			&readBytes,
			NULL
		);
		for(int j = 0; j < 512 * 8; j++)
			if (!BitwiseHandler::checkBit((uint8_t*)tombstones, j)) {
				BitwiseHandler::setBit((uint8_t*)tombstones, j);
				int a = *(int*)tombstones;
				SetFilePointer(fileHandle, offset, 0, NULL);
				WriteFile(
					fileHandle,
					tombstones,
					512,
					&readBytes,
					NULL
				);
				free(tombstones);
				return i * 512 * 8 + j;
			}
		offset += 4096 * registerSize + 512;
	}
	free(tombstones);
	return -1;
}

long long BufferManager::GetL2Offset(long long registerNumber, int valueSize, int metadata){
	int register_size = GetL2Size(registerNumber, valueSize,metadata);
	long long numberRegisterTombstones = ceil(double(registerNumber) / 4096);
	long long offset = numberRegisterTombstones * SEGMENT_SIZE + register_size * registerNumber;
	return offset;
}

void BufferManager::StoreLevel2(Table* table) {

	HANDLE L1_register_handle = CreateFileA(
		(LPCSTR)(table->name),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	char* fileName2 = (char*)calloc(sizeof(table->name) + 2, 1);
	fileName2[0] = '2';
	fileName2[1] = '_';
	std::memcpy(fileName2 + 2, table->name, sizeof(table->name));
	//bool increaseL2Registers = false;

	HANDLE L2_register_handle = CreateFileA(
		(LPCSTR)(fileName2),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (L1_register_handle == INVALID_HANDLE_VALUE || L2_register_handle == INVALID_HANDLE_VALUE)	std::cerr << "CreateFileA failed, error: " << GetLastError() << std::endl;

	long long registerIndex = GetL2FreeRegister(table->L2_registers, table->rowSize, L2_register_handle, SEGMENT_SIZE);
	long long offset;
	bool increasedL2Registers = false;
	if (registerIndex < 0) {
		offset = GetL2Offset(table->L2_registers, table->rowSize, SEGMENT_SIZE);
		increasedL2Registers = true;
	}
		
	else
		offset = GetL2Offset(registerIndex, table->rowSize, SEGMENT_SIZE);


	SetFilePointer(L2_register_handle, offset, 0, NULL);

	//create new REGISTER TOMBSTONES if the rest are full
	if (table->L2_registers % 4096 == 0) {
		uint8_t* tombstones = (uint8_t*)malloc(512);
		for (int i = 0; i < 512; i++)
			tombstones[i] = 0;
		//set to 1 the first bit -> current register
		BitwiseHandler::setBit(tombstones, 0);
		DWORD wroteBytes = 0;
		WriteFile(
			L2_register_handle,
			tombstones,
			512,
			&wroteBytes,
			NULL
		);
		offset += 512;
		std::cout << "wrote " << wroteBytes << " bytes out of 512 -- RegisterTombstones" << std::endl;
		free(tombstones);
	}

	//write the value tombstones
	DWORD wroteBytes;
	void* tombstones = malloc(16 * 3);
	for (int i = 16 * 2; i < 16 * 3; i++)
		((uint8_t*)tombstones)[i] = 255;
	SetFilePointer(L1_register_handle, 0, 0, NULL);
	ReadFile(
		L1_register_handle,
		tombstones,
		16,
		&wroteBytes,
		NULL
	);


	SetFilePointer(L2_register_handle, offset, 0, NULL);
	WriteFile(
		L2_register_handle,
		tombstones,
		16 * 3,
		&wroteBytes,
		NULL
	);
	offset += 512;


	//write the values
	int totalValuesPerBuffer = BUFFER_SIZE / table->rowSize;
	int l1_offset = SEGMENT_SIZE;
	int valuesPerBuffer = 0;
	int wroteValues = 0;
	int valuesToFill = 0;
	if (valuesPerBuffer > 128 * 2)
		valuesPerBuffer = 128 * 2;
	else
		valuesPerBuffer = totalValuesPerBuffer;
	void* buffer = malloc(valuesPerBuffer * table->rowSize);

	//read values from l1 and write them in l2
	while (wroteValues < 128 * 2) {
		SetFilePointer(L1_register_handle, l1_offset, 0, NULL);
		SetFilePointer(L2_register_handle, offset, 0, NULL);
		int valuesToWrite = valuesPerBuffer;
		if (2 * 128 - wroteValues < valuesPerBuffer)
			valuesToWrite = 2 * 128 - wroteValues;
		ReadFile(
			L1_register_handle,
			buffer,
			valuesToWrite * table->rowSize,
			&wroteBytes,
			NULL
		);
		WriteFile(
			L2_register_handle,
			buffer,
			valuesToWrite * table->rowSize,
			&wroteBytes,
			NULL
		);
		wroteValues += valuesToWrite;
		if (wroteValues < 128 * 2) {
			offset += BUFFER_SIZE;
			l1_offset += BUFFER_SIZE;
		}
		else {
			offset += valuesToWrite * table->rowSize;
			valuesToFill = totalValuesPerBuffer - valuesToWrite;
			if (valuesToFill > 128) valuesToFill = 128;
		}
	}

	wroteValues = 0;
	WriteFile(
		L2_register_handle,
		table->values,
		valuesToFill * table->rowSize,
		&wroteBytes,
		NULL
	);
	wroteValues += valuesToFill;
	offset += BUFFER_SIZE;
	SetFilePointer(L2_register_handle, offset, 0, NULL);

	while (wroteValues < 128) {
		int valuesToWrite = valuesPerBuffer;
		if (128 - wroteValues < valuesPerBuffer)
			valuesToWrite = 128 - wroteValues;
		WriteFile(
			L2_register_handle,
			(uint8_t*)table->values + wroteValues * table->rowSize,
			valuesToWrite * table->rowSize,
			&wroteBytes,
			NULL
		);
		wroteValues += valuesToWrite;
		if (wroteValues < 128) {
			offset += BUFFER_SIZE;
		}
	}

	CloseHandle(L2_register_handle);
	CloseHandle(L1_register_handle);
	free(buffer);
	free(tombstones);
	free(fileName2);


	auto storeColumn = [table](Column* column) {
		size_t tableNameLen = std::strlen(table->name);
		size_t columnNameLen = std::strlen(column->name);
		char* fileName2 = (char*)calloc(tableNameLen + columnNameLen + 4, sizeof(char)); // +1 for null terminator
		if (!fileName2) {
			std::cerr << "Memory allocation failed!" << std::endl;
			return -1;  // Handle allocation failure
		}

		fileName2[0] = '2';
		fileName2[1] = '_';
		std::memcpy(fileName2 + 2, table->name, tableNameLen);
		fileName2[tableNameLen + 2] = '&';
		std::memcpy(fileName2 + tableNameLen + 3, column->name, columnNameLen);
		fileName2[tableNameLen + columnNameLen + 3] = '\0';

		HANDLE L2_register_handle = CreateFileA(
			(LPCSTR)(fileName2),
			GENERIC_ALL,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		char* fileName = (char*)calloc(tableNameLen + columnNameLen + 2, sizeof(char)); // +1 for null terminator
		if (!fileName) {
			std::cerr << "Memory allocation failed!" << std::endl;
			return -1;  // Handle allocation failure
		}

		std::memcpy(fileName, table->name, tableNameLen);
		fileName[tableNameLen] = '&';
		std::memcpy(fileName + tableNameLen + 1, column->name, columnNameLen);
		fileName[tableNameLen + columnNameLen + 1] = '\0';

		HANDLE L1_register_handle = CreateFileA(
			(LPCSTR)(fileName),
			GENERIC_ALL,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);


		long long registerIndex = GetL2FreeRegister(table->L2_registers, column->data->numberOfBytes, L2_register_handle, L2_METADATA_SIZE);
		long long offset;
		if (registerIndex < 0)
			offset = GetL2Offset(table->L2_registers, column->data->numberOfBytes, L2_METADATA_SIZE);
		else
			offset = GetL2Offset(registerIndex, column->data->numberOfBytes, L2_METADATA_SIZE);
		long long metadataOffest = offset;


		SetFilePointer(L2_register_handle, offset, 0, NULL);

		//create new REGISTER TOMBSTONES if the rest are full
		if (table->L2_registers % 4096 == 0) {
			uint8_t* tombstones = (uint8_t*)malloc(512);
			for (int i = 0; i < 512; i++)
				tombstones[i] = 0;
			//set to 1 the first bit -> current register
			BitwiseHandler::setBit(tombstones, 0);
			DWORD wroteBytes = 0;
			WriteFile(
				L2_register_handle,
				tombstones,
				512,
				&wroteBytes,
				NULL
			);
			offset += 512;
			metadataOffest += 512;
			std::cout << "wrote " << wroteBytes << " bytes out of 512 -- RegisterTombstones" << std::endl;
			free(tombstones);
		}
		offset += L2_METADATA_SIZE;

		SetFilePointer(L2_register_handle, offset, 0, NULL);
		SetFilePointer(L1_register_handle, 0, 0, NULL);

		void* metadata = calloc(L2_METADATA_SIZE, 1);
		void* L1_metadata = malloc(L1_METADATA_SIZE);
		//+128 for the offsets
		void* l2_data = malloc(column->data->numberOfBytes * 128 + 128);
			
		DWORD bytes;
		ReadFile(
			L1_register_handle,
			L1_metadata,
			L1_METADATA_SIZE,
			&bytes,
			NULL
		);

		//write the tombstones
		//std::memcpy((char*)metadata + 720, (char*)L1_metadata + 512, 32);
		//for (int i = 0; i < 16; i++)
		//	((char*)metadata)[720 + 32 + i] = 255;


		int totalValuesPerBuffer = BUFFER_SIZE / column->data->numberOfBytes;
		int L1_1_offset = SEGMENT_SIZE * 2;
		int L1_2_offset = 0;
		int valuesPerBuffer = 0;
		int wroteValues = 0;
		int L1_index_1 = 0;
		int L1_index_2 = 0;
		int L2_index = 0;
		int currentValues_L1_1 = 0;
		int currentValues_L1_2 = 0;
		int currentIndex_L1_1 = 0;
		int currentIndex_L1_2 = 0;
		int currentIndexL2 = 0;
		int buffer_index = 0;
		int valuesToFill = 0;
		if (totalValuesPerBuffer > 128)
			valuesPerBuffer = 128;
		else
			valuesPerBuffer = totalValuesPerBuffer;
		void* L1_1_buffer = malloc(valuesPerBuffer * column->data->numberOfBytes);
		void* L1_2_buffer = malloc(valuesPerBuffer * column->data->numberOfBytes);
		void* L2_buffer = malloc(totalValuesPerBuffer * column->data->numberOfBytes);
		column->data->GetAllValuesWithBloom(l2_data, metadata, 720, (char*)l2_data + column->data->numberOfBytes * 128);


		//calculate the offset and number of values to read from the first buffer of l1 -> second register
		int valuesToRead;
		int segment = floor(128 / totalValuesPerBuffer);
		int writtenValues;
		if (valuesPerBuffer == 128) writtenValues = valuesPerBuffer;
		else writtenValues = 128 % valuesPerBuffer;
		L1_2_offset = BUFFER_SIZE * (segment) + writtenValues * (column->data->numberOfBytes) + 2 * 512;
		valuesToRead = totalValuesPerBuffer - writtenValues;
		if (valuesToRead > 128) valuesToRead = 128;

		currentValues_L1_2 = valuesToRead;
		currentValues_L1_1 = valuesPerBuffer;

		SetFilePointer(L1_register_handle, L1_1_offset, 0, NULL);
		ReadFile(
			L1_register_handle,
			L1_1_buffer,
			valuesPerBuffer * column->data->numberOfBytes,
			&bytes,
			NULL
		);


		SetFilePointer(L1_register_handle, L1_2_offset, 0, NULL);
		ReadFile(
			L1_register_handle,
			L1_2_buffer,
			valuesToRead * column->data->numberOfBytes,
			&bytes,
			NULL
		);





		while (wroteValues < 128 * 3) {

			//compare current values betwen L1_1, L1_2 & L2
			int equalOrLess = EQUALS | LESS;
			int compL1_1__L1_2;
			int compL1_1__L2;
			int compL1_2__L1_1;
			int compL1_2__L2;


			if (L1_index_1 < 128 && L1_index_2 < 128) {
				compL1_1__L1_2 = VoidMemoryHandler::COMPARE((char*)L1_1_buffer + currentIndex_L1_1 * column->data->numberOfBytes, (char*)L1_2_buffer + currentIndex_L1_2 * column->data->numberOfBytes, column->data->dataType);
			}
			if (L1_index_1 < 128 && L2_index < 128) {
				compL1_1__L2 = VoidMemoryHandler::COMPARE((char*)L1_1_buffer + currentIndex_L1_1 * column->data->numberOfBytes, (char*)l2_data + L2_index * column->data->numberOfBytes, column->data->dataType);
			}
			if (L1_index_2 < 128 && L1_index_1 < 128) {
				compL1_2__L1_1 = VoidMemoryHandler::COMPARE((char*)L1_2_buffer + currentIndex_L1_2 * column->data->numberOfBytes, (char*)L1_1_buffer + currentIndex_L1_1 * column->data->numberOfBytes, column->data->dataType);
			}
			if (L1_index_2 < 128 && L2_index < 128) {
				compL1_2__L2 = VoidMemoryHandler::COMPARE((char*)L1_2_buffer + currentIndex_L1_2 * column->data->numberOfBytes, (char*)l2_data + L2_index * column->data->numberOfBytes, column->data->dataType);
			}


			if (L1_index_1 >= 128) {
				compL1_1__L1_2 = BIGGER;
				compL1_1__L2 = BIGGER;
				compL1_2__L1_1 = LESS;
			}
			if (L1_index_2 >= 128) {
				compL1_2__L1_1 = BIGGER;
				compL1_2__L2 = BIGGER;
				compL1_1__L1_2 = LESS;
			}
			if (L2_index >= 128) {
				compL1_1__L2 = LESS;
				compL1_2__L2 = LESS;
			}


			if (compL1_1__L1_2 & equalOrLess && compL1_1__L2 & equalOrLess) {
				std::cout << "L1_1 " << *(int*)((char*)L1_1_buffer + currentIndex_L1_1 * column->data->numberOfBytes) << " -- " << (int)(*((uint8_t*)L1_metadata + L1_BLOOM_FILTER_SIZE + 32 + L1_index_1)) << std::endl;
				std::memcpy((char*)L2_buffer + currentIndexL2 * column->data->numberOfBytes, (char*)L1_1_buffer + currentIndex_L1_1 * column->data->numberOfBytes, column->data->numberOfBytes);
				uint16_t offset = *((uint8_t*)L1_metadata + L1_BLOOM_FILTER_SIZE + 32 + L1_index_1);
				std::memcpy((char*)metadata + L2_BLOOM_FILTER_SIZE + 48 + wroteValues * 2, &offset, 2);
				bool tombstone = BitwiseHandler::checkBit((uint8_t*)L1_metadata + L1_BLOOM_FILTER_SIZE, L1_index_1);
				if (tombstone) {
					BitwiseHandler::setBit((uint8_t*)metadata + 720, wroteValues);
				}
				else {
					BitwiseHandler::clearBit((uint8_t*)metadata + 720, wroteValues);
				}
				BloomFilter::AddValue(metadata, (char*)L1_1_buffer + currentIndex_L1_1 * column->data->numberOfBytes, column->data->numberOfBytes, 720);
				currentIndexL2++;
				currentIndex_L1_1++;
				wroteValues++;
				L1_index_1++;
			}
			else if (compL1_2__L1_1 & equalOrLess && compL1_2__L2 & equalOrLess) {
				std::cout << "L1_2 " << *(int*)((char*)L1_2_buffer + currentIndex_L1_2 * column->data->numberOfBytes) << " -- " << (int)(*((uint8_t*)L1_metadata + L1_BLOOM_FILTER_SIZE + 32 + L1_index_2 + 128)) << std::endl;
				std::memcpy((char*)L2_buffer + currentIndexL2 * column->data->numberOfBytes, (char*)L1_2_buffer + currentIndex_L1_2 * column->data->numberOfBytes, column->data->numberOfBytes);
				uint16_t offset = *((uint8_t*)L1_metadata + L1_BLOOM_FILTER_SIZE + 32 + L1_index_2 + 128);
				std::memcpy((char*)metadata + L2_BLOOM_FILTER_SIZE + 48 + wroteValues * 2, &offset, 2);
				bool tombstone = BitwiseHandler::checkBit((uint8_t*)L1_metadata + L1_BLOOM_FILTER_SIZE, L1_index_2 + 128);
				if (tombstone) {
					BitwiseHandler::setBit((uint8_t*)metadata + 720, wroteValues);
				}
				else {
					BitwiseHandler::clearBit((uint8_t*)metadata + 720, wroteValues);
				}
				BloomFilter::AddValue(metadata, (char*)L1_2_buffer + currentIndex_L1_2 * column->data->numberOfBytes, column->data->numberOfBytes, 720);
				currentIndexL2++;
				currentIndex_L1_2++;
				wroteValues++;
				L1_index_2++;
			}
			else {
				std::cout << "L2 " << *(int*)((char*)l2_data + L2_index * column->data->numberOfBytes) << " -- " << (int)(*((uint8_t*)l2_data + column->data->numberOfBytes * 128 + L2_index) + 128 + 128) << std::endl;
				std::memcpy((char*)L2_buffer + currentIndexL2 * column->data->numberOfBytes, (char*)l2_data + L2_index * column->data->numberOfBytes, column->data->numberOfBytes);
				uint16_t offset = *((uint8_t*)l2_data + column->data->numberOfBytes * 128 + L2_index) + 128 + 128;
				std::memcpy((char*)metadata + L2_BLOOM_FILTER_SIZE + 48 + wroteValues * 2, &offset, 2);
				BitwiseHandler::setBit((uint8_t*)metadata + 720, wroteValues);
				currentIndexL2++;
				wroteValues++;
				L2_index++;
			}


			if (currentIndex_L1_1 > currentValues_L1_1 && L1_index_1 <= 128) {
				int remainingValues = 128 - L1_index_1;
				if (remainingValues > valuesPerBuffer)
					remainingValues = valuesPerBuffer;
				//in case the offsets points to the middle of a buffer, now it will point to the beggining of the next one
				L1_1_offset = (L1_1_offset - L1_METADATA_SIZE) / BUFFER_SIZE * BUFFER_SIZE + L1_METADATA_SIZE;
				currentValues_L1_1 = remainingValues;
				currentIndex_L1_1 = 0;
				SetFilePointer(L1_register_handle, L1_1_offset, 0, NULL);
				ReadFile(
					L1_register_handle,
					L1_1_buffer,
					remainingValues * column->data->numberOfBytes,
					&bytes,
					NULL
				);
			}
			if (currentIndex_L1_2 > currentValues_L1_2 && L1_index_2 <= 128) {
				int remainingValues = 128 - L1_index_2;
				if (remainingValues > valuesPerBuffer)
					remainingValues = valuesPerBuffer;
				//in case the offsets points to the middle of a buffer, now it will point to the beggining of the next one
				L1_2_offset = (L1_2_offset - L1_METADATA_SIZE) / BUFFER_SIZE * BUFFER_SIZE + L1_METADATA_SIZE;
				currentValues_L1_2 = remainingValues;
				currentIndex_L1_2 = 0;
				SetFilePointer(L1_register_handle, L1_2_offset, 0, NULL);
				ReadFile(
					L1_register_handle,
					L1_2_buffer,
					remainingValues * column->data->numberOfBytes,
					&bytes,
					NULL
				);
			}
			//write the L2_buffer in disk
			if (currentIndexL2 == totalValuesPerBuffer || wroteValues == 3 * 128) {
				int valuesToWrite = 0;
				if (totalValuesPerBuffer > 3 * 128)
					valuesToWrite = 3 * 128;
				else
					valuesToWrite = totalValuesPerBuffer;
				SetFilePointer(L2_register_handle, offset, 0, NULL);
				WriteFile(
					L2_register_handle,
					L2_buffer,
					valuesToWrite * column->data->numberOfBytes,
					&bytes,
					NULL
				);
				offset += BUFFER_SIZE;
				currentIndexL2 = 0;
			}


		}
		SetFilePointer(L2_register_handle, metadataOffest, 0, NULL);
		WriteFile(
			L2_register_handle,
			metadata,
			L2_METADATA_SIZE,
			&bytes,
			NULL
		);



		CloseHandle(L2_register_handle);
		CloseHandle(L1_register_handle);
		free(fileName2);
		free(metadata);
		free(L1_metadata);
		free(l2_data);
		free(L1_1_buffer);
		free(L1_2_buffer);
		free(L2_buffer);

		};
	table->columns.IterateWithCallback(storeColumn);
	if(increasedL2Registers)
		table->L2_registers++;
};




void BufferManager::SearchLevel2(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>& foundRegisters, std::vector<std::vector<int>>& foundOffsets, int comparator){

	size_t tableNameLen = std::strlen(table->name);
	size_t columnNameLen = std::strlen(column->name);
	char* fileName2 = (char*)calloc(tableNameLen + columnNameLen + 4, sizeof(char)); // +1 for null terminator
	if (!fileName2) {
		std::cerr << "Memory allocation failed!" << std::endl;
		return;  // Handle allocation failure
	}

	fileName2[0] = '2';
	fileName2[1] = '_';
	std::memcpy(fileName2 + 2, table->name, tableNameLen);
	fileName2[tableNameLen + 2] = '&';
	std::memcpy(fileName2 + tableNameLen + 3, column->name, columnNameLen);
	fileName2[tableNameLen + columnNameLen + 3] = '\0';

	HANDLE L2_register_handle = CreateFileA(
		(LPCSTR)(fileName2),
		GENERIC_ALL,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);


	int numberOfRegisterTombstones = ceil(float(table->L2_registers) / 4096);
	int L2Offset = 0;
	//offset inside the selected register
	int currentRegisterOffset = 0;
	void* registerTombstones = malloc(512);
	void* metadata = malloc(L2_METADATA_SIZE);
	void* buffer = malloc(BUFFER_SIZE);
	DWORD bytes;
	
	for (int i = 0; i < numberOfRegisterTombstones; i++) {
		SetFilePointer(L2_register_handle, L2Offset, 0, NULL);
		ReadFile(
			L2_register_handle,
			registerTombstones,
			512,
			&bytes,
			NULL
		);
		L2Offset += 512;
		int a = *(int*)registerTombstones;
		for (int j = 0; j < 4096; j++) {
			if (BitwiseHandler::checkBit((uint8_t*)registerTombstones, j)) {
				SetFilePointer(L2_register_handle, L2Offset, 0, NULL);
				ReadFile(
					L2_register_handle,
					metadata,
					L2_METADATA_SIZE,
					&bytes,
					NULL
				);
				bool preCheck = false;
				bool checkInOrder = true;
				bool orderChoosed = false;
				currentRegisterOffset = L2Offset + 512 * 3;
				if (comparator & EQUALS) {

					for (int k = 0; k < argumentsNumber; k++)
						if (BloomFilter::CheckValue(metadata, values[k], column->data->numberOfBytes, L2_BLOOM_FILTER_SIZE)) 
							preCheck = true;
					
				}
				if (comparator & LESS && !orderChoosed) {
					SetFilePointer(L2_register_handle, currentRegisterOffset, 0, NULL);
					ReadFile(
						L2_register_handle,
						buffer,
						BUFFER_SIZE,
						&bytes,
						NULL
					);
					for (int k = 0; k < argumentsNumber; k++) {
						int res = VoidMemoryHandler::COMPARE(buffer, values[k], column->data->dataType);
						if (res & LESS) {
							preCheck = true;
							orderChoosed = true;
							checkInOrder = true;
						}

					}
						
						
				}
				if (comparator & BIGGER && !orderChoosed) {
					//remaining values in the last buffer
					int totalValuesPerBuffer = floor(BUFFER_SIZE / column->data->numberOfBytes);
					if (totalValuesPerBuffer > 3 * 128) totalValuesPerBuffer = 3 * 128;
					int remainingValues = 3 * 128 % totalValuesPerBuffer;
					if (remainingValues == 0) remainingValues = totalValuesPerBuffer;

					int numberOfBuffers = ceil(float(3 * 128) / totalValuesPerBuffer);
					currentRegisterOffset += (numberOfBuffers - 1) * BUFFER_SIZE;

					SetFilePointer(L2_register_handle, currentRegisterOffset, 0, NULL);
					ReadFile(
						L2_register_handle,
						buffer,
						remainingValues * column->data->numberOfBytes,
						&bytes,
						NULL
					);

					for (int k = 0; k < argumentsNumber; k++) {
						int res = VoidMemoryHandler::COMPARE((uint8_t*)buffer + (remainingValues - 1) * column->data->numberOfBytes, values[k], column->data->dataType);
						if (res & BIGGER) {
							preCheck = true;
							checkInOrder = false;
							orderChoosed = true;
						}
							
					}
				}



				if (preCheck) {
					if (checkInOrder) {
						SetFilePointer(L2_register_handle, currentRegisterOffset, 0, NULL);
						ReadFile(
							L2_register_handle,
							buffer,
							BUFFER_SIZE,
							&bytes,
							NULL
						);

						int totalValuesPerBuffer = floor(BUFFER_SIZE / column->data->numberOfBytes);
						if (totalValuesPerBuffer > 3 * 128) totalValuesPerBuffer = 3 * 128;

						for (int value = 0; value < 3 * 128; value++) {
							if (BitwiseHandler::checkBit((uint8_t*)metadata + 720, value)) {
								for (int param = 0; param < argumentsNumber; param++) {
									int res = VoidMemoryHandler::COMPARE((uint8_t*)buffer + (value % totalValuesPerBuffer) * column->data->numberOfBytes, values[param], column->data->dataType);
									//if the comparator has both bigger and less, just continue
									if (!(comparator & res) && (comparator & (LESS | BIGGER)) != (LESS | BIGGER)) return;

									if (comparator & res) {
										int offset = *(uint16_t*)((uint8_t*)metadata + 720 + 48 + value * 2);
										int registerIndex = i * 4096 + j;
										if (foundRegisters.size() == 0 || foundRegisters.back() != registerIndex) {
											foundRegisters.push_back(registerIndex);
											foundOffsets.push_back(std::vector<int>());
										}
										foundOffsets.back().push_back(offset);
									}
								}
							}
							if (value != 0 && value % totalValuesPerBuffer == 0) {
								currentRegisterOffset += BUFFER_SIZE;
								SetFilePointer(L2_register_handle, currentRegisterOffset, 0, NULL);
								ReadFile(
									L2_register_handle,
									buffer,
									BUFFER_SIZE,
									&bytes,
									NULL
								);
							}
						}
					}
					else {
						int totalValuesPerBuffer = floor(BUFFER_SIZE / column->data->numberOfBytes);
						if (totalValuesPerBuffer > 3 * 128) totalValuesPerBuffer = 3 * 128;
						int remainingValues = 3 * 128 % totalValuesPerBuffer;
						if (remainingValues == 0) remainingValues = totalValuesPerBuffer;

						int numberOfBuffers = ceil(float(3 * 128) / totalValuesPerBuffer);
						currentRegisterOffset += (numberOfBuffers - 1) * BUFFER_SIZE;

						SetFilePointer(L2_register_handle, currentRegisterOffset, 0, NULL);
						ReadFile(
							L2_register_handle,
							buffer,
							remainingValues* column->data->numberOfBytes,
							&bytes,
							NULL
						);

						for (int value = 3 * 128; value > 0; value++) {

						}


					}
				}
			}
		}
	}
}