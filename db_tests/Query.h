#pragma once

#include <vector>
#include <algorithm>

#include "Table.h"
#include "Column.h"

#include "BufferManager.h"


//class that contains the search results of a specific query
class QueryData {
public:
	QueryData* next;

	std::vector<int> L0_results;
	std::vector<int> L1_results;
	std::vector<int> L2_results;

	QueryData() :next(nullptr), L0_results(std::vector<int>()), L1_results(std::vector<int>()), L2_results(std::vector<int>()) {};
	

};

class Query {
private:

	LinkedList<QueryData> *querriesResults;

	void OperationOR(std::vector<int>& a, std::vector<int>& b) {
		for (int num : a) std::cout << num << " ";
		std::cout << "" << std::endl;
		for (int num : b) std::cout << num << " ";
		std::cout << "" << std::endl;
		if (a.size() == 0 && b.size() == 0) return;
		for (auto it = b.begin(); it != b.end(); ) {
			// Check if the current element is in a
			if (std::find(a.begin(), a.end(), *it) == a.end()) {
				std::cout << "adding " << *it << std::endl;
				a.push_back(*it);//if not, add it
			}
			else {
				// Otherwise, just move to the next element
				++it;
			}
		}
	}

	//vector a will be used to store the result
	//vector b will be used just to compare values from a
	void OperationAND(std::vector<int>& a, std::vector<int>& b) {
		for (int num : a) std::cout << num << " ";
		std::cout << "" << std::endl;
		for (int num : b) std::cout << num << " ";
		std::cout << "" << std::endl;
		if (a.size() == 0 || b.size() == 0) return;
		for (auto it = a.begin(); it != a.end(); ) {
			// Check if the current element is in b
			if (std::find(b.begin(), b.end(), *it) == b.end()) {
				std::cout << "erasing" << std::endl;
				// If not found in b, erase it
				it = a.erase(it); // erase returns the next valid iterator
			}
			else {
				// Otherwise, just move to the next element
				++it;
			}
		}
	}

public:
	Query():querriesResults(new LinkedList<QueryData>()){}

	//comparator -> LESS, EQUALS, BIGGER, NOT
	Query* FindByComparator(Table* table, char* columnName, void* values[], int argumentsNumber, int comparator) {
		auto columnParser = [this ,columnName, table, &argumentsNumber, values, comparator](Column* currentColumn) {
			if (strcmp(currentColumn->name, columnName) == 0) {
				QueryData* foundData = new QueryData();

				//check L0 register
				for (int i = 0; i < argumentsNumber; i++) {
					currentColumn->data->FindValues(values[i], foundData->L0_results, comparator);
				}

				//check L1 registers
				if (table->L1_registers != 0) {
					BufferManager::SearchLevel1(table, currentColumn, values, argumentsNumber, foundData->L1_results, comparator);
				}
				std::cout << foundData->L0_results.size() << " aw" << std::endl;
				for (int i = 0; i < foundData->L0_results.size(); i++) {
					std::cout << foundData->L0_results.at(i) << std::endl;
				}
				querriesResults->AddNode(foundData);
			}
			};

			
		table->columns.IterateWithCallback(columnParser);


		return this;
	}

	//operation -> AND, OR
	Query* CompareQueries(int operation) {
		QueryData* currentData = querriesResults->head;
		if (operation == AND) {
			QueryData* cursor = querriesResults->head->next;
			while (cursor != nullptr) {
				OperationAND(currentData->L0_results, cursor->L0_results);
				OperationAND(currentData->L1_results, cursor->L1_results);
				OperationAND(currentData->L2_results, cursor->L2_results);
				cursor = cursor->next;
			}
		}
		else if (operation == OR) {
			QueryData* cursor = querriesResults->head->next;
			while (cursor != nullptr) {
				OperationOR(currentData->L0_results, cursor->L0_results);
				OperationOR(currentData->L1_results, cursor->L1_results);
				OperationOR(currentData->L2_results, cursor->L2_results);
				cursor = cursor->next;
			}
		}
		return this;
	}

};