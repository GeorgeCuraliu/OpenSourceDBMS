#pragma once

#include "Table.h"
#include "Column.h"

class Conditions {
public:
	static void equals(Table* table, char* columnName, void* values[], int argumentsNumber) {
		auto columnParser = [columnName, table, &argumentsNumber, values](Column* currentColumn) {
			if (strcmp(currentColumn->name, columnName) == 0) {
				for (int i = 0; i < argumentsNumber; i++) {
					int offset = currentColumn->data->FindValue(values[i]);
					if (offset >= 0) table->DisplayRow(offset);

				}
			}
			};
		table->columns.IterateWithCallback(columnParser);
	}
	static void less() {

	}
};