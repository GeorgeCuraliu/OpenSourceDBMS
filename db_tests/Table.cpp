#pragma once

#include "Table.h"

#include "BufferManager.h"
#include "LinkedList.h"
#include "Column.h"
#include "Parameters.h"
#include "BitwiseHandler.h"

Table::Table(char* name, uint8_t L1_registers, uint8_t L2_registers) :
	numberOfValues(0),
	numberOfColumns(0),
	values(nullptr),
	rowSize(0),
	name(name),
	L1_registers(L1_registers),
	L2_registers(L2_registers)
{
	freeMemory = new uint8_t[16];
	for (int i = 0; i < 16; i++) freeMemory[i] = 0;
}

Column* Table::AddColumn(char* name, char* type, uint8_t numberOfBytes) {
	numberOfColumns++;
	Column* newColumn = new Column(name, type, numberOfBytes);
	columns.AddNode(newColumn);
	return newColumn;
}

void Table::ConfirmDiagram() {//will allocate memory for the values pointer after the diagram was loaded and check the size of a row
	int size = 0;
	auto getRowSize = [&size](Column* current) {
		size += current->data->numberOfBytes;
		};
	columns.IterateWithCallback(getRowSize);
	rowSize = size;
	values = malloc(size * 128);
	if (!values) std::cout << "error allocating memory for values pointer" << std::endl;
}

void Table::AddRow(void* newData[]) {

	//store the new data into values memory
	uint8_t i;//i will be also used for offset
	for (i = 0; i < 128; i++) {
		if (!BitwiseHandler::checkBit(freeMemory, i)) {
			std::cout << "found free space at " << (int)i << " " << std::endl;
			BitwiseHandler::setBit(freeMemory, i);
			numberOfValues++;

			void* data = values;//used for lamba referncing
			int s = rowSize;
			int columnNumber = 0;
			int cursor = 0;
			auto storeValues = [&i, data, &s, newData, &columnNumber, &cursor](Column* current) {
				std::memcpy(((char*)data) + i * s + cursor, (char*)(newData[columnNumber]), current->data->numberOfBytes);
				cursor += current->data->numberOfBytes;
				columnNumber++;
				};
			columns.IterateWithCallback(storeValues);

			break;
		}
	}

	if (L1_registers != 0) i += 128;

	int columnsParsed = 0;
	auto addData = [&columnsParsed, newData, &i](Column* current) {
		std::cout << "sending offset " << (int)i << std::endl;
		current->data->AddValue(newData[columnsParsed], current->data->dataType, i);
		columnsParsed++;
		};
	columns.IterateWithCallback(addData);

	if (columnsParsed == numberOfColumns) {
		std::cout << "complete row" << std::endl;
	}
	else {
		std::cout << "incomplete row" << std::endl;
	}
	std::cout << "" << std::endl;

	if (numberOfValues >= 128) {
		std::cout << "LIMIT REACHED --- FLUSHING" << std::endl;
		FlushData();
	}
}

void Table::FlushData() {
	if (L1_registers < 5) {
		BufferManager::StoreLevel1(this);
		L1_registers++;
	}
	
	for (int i = 0; i < 128 * rowSize; i++) ((char*)values)[i] = 0;
	for (int i = 0; i < 16; i++) freeMemory[i] = 0;
	numberOfValues = 0;
	
	auto flushTrees = [](Column* column) {
		column->data->FlushData();
	};
	columns.IterateWithCallback(flushTrees);

	std::cout << "Data flushed succesfully" << std::endl;
}

void Table::DisplayRow(int offset) {
	if (BitwiseHandler::checkBit(freeMemory, offset)) {
		void* row = malloc(rowSize);
		if (!row) return;
		std::memcpy((char*)row, (char*)values + rowSize * offset, rowSize);
		std::cout << "row " << offset << "---";

		int cursor = 0;//will be used as a cursor to read from row
		auto processData = [row, &cursor](Column* current) {
			if (std::strcmp(current->data->dataType, "int") == 0) {
				void* temp = malloc(SHORT_INT_SIZE);
				if (!temp) return;
				std::memcpy((char*)temp, ((char*)row) + cursor, SHORT_INT_SIZE);
				std::cout << current->name << " (INT) -> " << *(int*)temp << "   ";
				cursor += SHORT_INT_SIZE;
				free(temp);
			}
			};
		columns.IterateWithCallback(processData);
		std::cout << "" << std::endl;
	}
}

void Table::DisplayColumns() {
	auto displayColumnsValue = [](Column* current) {
		std::cout << current->name << std::endl;
		};
	columns.IterateWithCallback(displayColumnsValue);
}

void Table::DisplayAllRows() {
	for (int i = 0; i < 128; i++) {
		DisplayRow(i);
	}
}