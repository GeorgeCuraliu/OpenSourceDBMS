#pragma once

#include <windows.h>
#include <fileapi.h>
#include <vector>

//#include "Table.h"
class Table;
class Column;

class BufferManager {
public:
	static void StoreLevel1(Table* table);
	static void SearchLevel1(Table* table, Column* column, void* values[], int argumentsNumber, std::vector<int>* foundValues);
};
