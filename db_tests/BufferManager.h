#pragma once

#include <windows.h>
#include <fileapi.h>

//#include "Table.h"
class Table;

class BufferManager {
public:
	static void StoreLevel1(Table* table);
};
