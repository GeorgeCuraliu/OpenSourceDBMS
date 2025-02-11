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
	//stores the register where were found matches
	std::vector<int> L2_results_registers;
	//L2_result_offsets stores just the offset of matches found in a register
	std::vector<std::vector<int>> L2_results_offsets;

	QueryData() :next(nullptr), L0_results(std::vector<int>()), L1_results(std::vector<int>()), L2_results_registers(std::vector<int>()) {};
	

};

class Query {
private:

	LinkedList<QueryData> *querriesResults;
	Table* table;

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
		if (a.size() == 0) b.clear();
		if (b.size() == 0) a.clear();
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
	Query(Table* table):querriesResults(new LinkedList<QueryData>()), table(table){}

	//comparator -> LESS, EQUALS, BIGGER, NOT
	Query* FindByComparator(char* columnName, void* values[], int argumentsNumber, int comparator) {
		auto columnParser = [this ,columnName, &argumentsNumber, values, comparator](Column* currentColumn) {
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
				if (table->L2_registers != 0) {
					BufferManager::SearchLevel2(table, currentColumn, values, argumentsNumber, foundData->L2_results_registers, foundData->L2_results_offsets, comparator);
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
				//OperationAND(currentData->L2_results, cursor->L2_results);

				if (currentData->L2_results_registers.size() != 0) {
					std::vector<std::vector<int>>::iterator it_0 = currentData->L2_results_offsets.begin();
					for (std::vector<int>::iterator it_r0 = currentData->L2_results_registers.begin(); it_r0 != currentData->L2_results_registers.end();) {
						bool check = false;
						std::vector<std::vector<int>>::iterator it_1 = cursor->L2_results_offsets.begin();
						for (std::vector<int>::iterator it_r1 = cursor->L2_results_registers.begin(); it_r1 != cursor->L2_results_registers.end() && !check; it_r1++, it_1++) {
							if (*it_r1 == *it_r0) {
								check = true;
								OperationAND(*it_0, *it_1);
							}
						}
						if (!check) {
							it_r0 = currentData->L2_results_registers.erase(it_r0);
							it_0 = currentData->L2_results_offsets.erase(it_0);
						}
						else {
							it_r0++;
							it_0++;
						}
					}
				}

				QueryData* oldCursor = cursor;
				cursor = cursor->next;
				delete oldCursor;
			}
		}
		else if (operation == OR) {
			QueryData* cursor = querriesResults->head->next;
			while (cursor != nullptr) {
				OperationOR(currentData->L0_results, cursor->L0_results);
				OperationOR(currentData->L1_results, cursor->L1_results);
				//OperationOR(currentData->L2_results, cursor->L2_results);

				if (currentData->L2_results_registers.size() != 0) {
					std::vector<std::vector<int>>::iterator it_0 = currentData->L2_results_offsets.begin();
					for (std::vector<int>::iterator it_r0 = currentData->L2_results_registers.begin(); it_r0 != currentData->L2_results_registers.end(); it_r0++, it_0++) {
						bool check = false;
						std::vector<std::vector<int>>::iterator it_1 = cursor->L2_results_offsets.begin();
						std::vector<int>::iterator it_r1 = cursor->L2_results_registers.begin();
						for (; it_r1 != cursor->L2_results_registers.end() && !check; it_r1++, it_1++) {
							if (*it_r1 == *it_r0) {
								check = true;
								OperationOR(*it_0, *it_1);
								/*	it_r1 = currentData->L2_results_registers.erase(it_r1);
									it_1 = currentData->L2_results_offsets.erase(it_1);*/
							}
						}
						if (!check) {
							/*it_r0 = currentData->L2_results_registers.erase(it_r0);
							it_0 = currentData->L2_results_offsets.erase(it_0);*/
							currentData->L2_results_offsets.push_back(*it_1);
							currentData->L2_results_registers.push_back(*it_r1);

							cursor->L2_results_offsets.erase(it_1);
							cursor->L2_results_registers.erase(it_r1);
						}
						else {
							/*it_r0++;
							it_0++;*/

						}
					}

				}

				QueryData* oldCursor = cursor;
				cursor = cursor->next;
				delete oldCursor;
			}
			//all QueryData object were merged into this one, therefore there is no next QueryData
			currentData->next = nullptr;
			return this;
		}
	}
	//Delete() assumes that querriesResult has just one querryResult
	Query* Delete() {

		if (querriesResults->head->L0_results.size()) {
			auto deleteFromL0 = [this](Column* column) {
				column->data->DeleteValues(querriesResults->head->L0_results);
				};
			table->columns.IterateWithCallback(deleteFromL0);
			table->DeleteRow(querriesResults->head->L0_results);
		}
		
		if(querriesResults->head->L1_results.size())
		BufferManager::DeleteValuesLevel1(table, querriesResults->head->L1_results);

		querriesResults->DeleteNode(querriesResults->head);
		
		return this;
	}

};