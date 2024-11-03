#pragma once

#include <vector>

#include "Table.h"
#include "Column.h"

#include "BufferManager.h"

class Query {
private:
	//will be used to determine how to treat the found data
	//if it is true, all the data will be added to found data vectors
	//if false && operation = AND, the found offsets have to already be in foundValues
	bool firstQuery = true;

public:
	//comparator -> LESS, EQUALS, BIGGER, NOT
	//operation -> AND, OR
	Query* FindByComparator(Table* table, char* columnName, void* values[], int argumentsNumber, int comparator, int operation) {
		auto columnParser = [columnName, table, &argumentsNumber, values](Column* currentColumn) {
			if (strcmp(currentColumn->name, columnName) == 0) {

				//check L0 register
				for (int i = 0; i < argumentsNumber; i++) {
					uint8_t offset;
					bool res = currentColumn->data->FindValue(values[i], offset);
					if (res) table->DisplayRow(offset);
				}

				//check L1 registers
				if (table->L1_registers != 0) {
					std::vector<int> foundValues;
					BufferManager::SearchLevel1(table, currentColumn, values, argumentsNumber, &foundValues);

				}
			}
			};

			
		table->columns.IterateWithCallback(columnParser);


		return this;
	}

};