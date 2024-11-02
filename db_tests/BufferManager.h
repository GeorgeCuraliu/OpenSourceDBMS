#pragma once

#include <windows.h>
#include <fileapi.h>

//#include "Table.h"
class Table;

class BufferManager {
public:
	static void StoreLevel1(Table* table);
	static void SearchLevel1(Table* table, char* columnName, void* values[], int argumentsNumber);
};
