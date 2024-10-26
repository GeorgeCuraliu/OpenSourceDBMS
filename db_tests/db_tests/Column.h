#pragma once

#include "BTree0.h"

class Column {
public:
	Column* next;
	char* name;
	BTree* data;

	Column(char* name, char* dataType, uint8_t numberOfBytes) :next(nullptr), name(name), data(new BTree(numberOfBytes, dataType)) {}
};