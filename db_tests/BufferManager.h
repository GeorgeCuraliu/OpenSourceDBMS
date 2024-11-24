#pragma once

#include <windows.h>
#include <fileapi.h>
#include <vector>

//#include "Table.h"
class Table;
class Column;

class BufferManager {
private:
	static void ClearTombstones(char* fileName, std::vector<int>& deleteValues, int offset);
public:
	static void StoreLevel1(Table* table);
	static void SearchLevel1(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>& foundValues, int comparator);
	static void DeleteValuesLevel1(Table* table, std::vector<int>& deleteValues);
};
