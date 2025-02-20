#pragma once

#include <windows.h>
#include <fileapi.h>
#include <vector>

//#include "Table.h"
class Table;
class Column;

class BufferManager {
private:
	static void ClearTombstonesLevel1(char* fileName, std::vector<int>& deleteValues, int tombstonesOffset, int numberOfValues = 0, bool searchOffsets = false);
	static long long GetL2Offset(long long registerNumber, int valueSize, int metadata);
	static long long GetL2FreeRegister(long long registerNumber, int valueSize, HANDLE fileHandle, int metadata);
	static int GetL2Size(long long registerNumber, int valueSize, int metadata);
public:
	static void StoreLevel1(Table* table);
	static void SearchLevel1(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>& foundValues, int comparator);
	static void DeleteValuesLevel1(Table* table, std::vector<int>& deleteValues);
	static void StoreLevel2(Table* table);
	static void SearchLevel2(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>& foundRegisters, std::vector<std::vector<int>>& foundOffsets, int comparator);
	static void DeleteValuesLevel2(Table* table, std::vector<int>& registers, std::vector <std::vector<int>>& foundValues);
};
