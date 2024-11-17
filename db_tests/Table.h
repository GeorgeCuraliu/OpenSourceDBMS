#pragma once

#include "LinkedList.h"

class Column;


class Table {
public:

	LinkedList<Column> columns;
	uint16_t numberOfValues;
	uint16_t numberOfColumns;
	int rowSize;
	void* values;
	uint8_t* freeMemory;//will be used to check if a place in values is free or not
	char* name;
	uint8_t L1_registers;
	uint8_t L2_registers;

	Table(char* name, uint8_t L1_registers, uint8_t L2_registers);

	Column* AddColumn(char* name, char* type, uint8_t numberOfBytes);

	//will allocate memory for the values pointer after the diagram was loaded and check the size of a row
	void ConfirmDiagram();
	void AddRow(void* newData[]);
	void DeleteRow(std::vector<int>& offsets);
	void FlushData();
	void DisplayRow(int offset);
	void DisplayColumns();
	void DisplayAllRows();

	~Table();

};