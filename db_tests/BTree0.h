#pragma once

#include <iostream>
#include <functional>
#include <vector>

#include "VoidMemoryHandler.h"
#include "BloomFilter.h"

class BTNode {
public:
	BTNode* left, * right;
	void* value;
	BTNode(void* data, uint8_t offset, int dataSize) :left(nullptr), right(nullptr) {
		if (!data) std::cout << "data is invalid" << std::endl;
		std::cout << "offset " << (int)offset << std::endl;
		value = malloc(dataSize + 1);
		if (!value) return;
		std::memcpy(value, data, dataSize);
		std::memcpy((uint8_t*)value + dataSize, &offset, 1);
	}
	void* getValue(int valueSize) {
		void* data = malloc(valueSize);
		if (!data)return 0;
		std::memcpy(data, value, valueSize);
		return data;
	}
	uint8_t getOffset(int valueSize) {
		uint8_t offset;
		std::memcpy(&offset, (uint8_t*)value + valueSize, 1); // Read the last byte as the offset
		return offset;
	}
	~BTNode() {
		free(value);
	}
};



class BTree {
public:
	uint8_t numberOfBytes;
	BTNode* root;
	char* dataType;
	BTree(uint8_t numberOfBytes, char* dataType) :numberOfBytes(numberOfBytes), root(nullptr), dataType(dataType) {}
	void AddValue(void* value, char* dataType, uint8_t offset) {
		if (!value) std::cout << "value is invalid" << std::endl;
		std::cout << "received value " << *(int*)value << " and index " << (int)offset << std::endl;
		BTNode* newNode = new BTNode(value, offset, numberOfBytes);
		if (!root) {
			std::cout << "new root" << std::endl;
			root = newNode;
			return;
		}

		BTNode* temp = root;
		while (temp) {
			void* val = temp->getValue(numberOfBytes);
			int comp = VoidMemoryHandler::COMPARE(value, val, dataType);
			free(val);
			if (comp & (EQUALS | LESS)) {
				//std::cout << (int)temp->getOffset(numberOfBytes) << " left " << std::endl;
				if (temp->left == nullptr) {
					temp->left = newNode;
					break;
				}
				else temp = temp->left;
			}
			else {
				//std::cout << (int)temp->getOffset(numberOfBytes) << " right " << std::endl;
				if (temp->right == nullptr) {
					temp->right = newNode;
					break;
				}
				else temp = temp->right;
			}
		}

	}
	void FindValues(void* value, std::vector<int>& foundValues, int comparator) {

		auto GetOffsetsFromSubtree = [this, &foundValues](BTNode* current) {
			std::cout << "found match at offset " << (int)current->getOffset(numberOfBytes) << " " << *(int*)current->getValue(numberOfBytes) << std::endl;
			foundValues.push_back(current->getOffset(numberOfBytes));
			};

		BTNode* temp = root;
		while (temp) {
			void* val = temp->getValue(numberOfBytes);
			int comparationResult = VoidMemoryHandler::COMPARE(val, value, dataType);
			free(val);
			bool currentValue = false;

			//just add the offset if it matches the equals comparator
			if (comparator & EQUALS && comparationResult == EQUALS) {
				foundValues.push_back((int)temp->getOffset(numberOfBytes));
				std::cout << "found match at -equals- offset " << (int)temp->getOffset(numberOfBytes) << " " << *(int*)temp->getValue(numberOfBytes) << std::endl;
				currentValue = true;
			}
			//iterate the entire subtree that matches the description
			if (comparator & comparationResult && comparator != EQUALS) {
				if (!currentValue)
					foundValues.push_back((int)temp->getOffset(numberOfBytes));
				if (comparator & LESS) {
					IterrateWithCallback(temp->left, GetOffsetsFromSubtree);
					temp = temp->right;
				}
				else if (comparator & BIGGER) {
					IterrateWithCallback(temp->right, GetOffsetsFromSubtree);
					temp = temp->left;
				}
			}
			else if (comparationResult & LESS) {
				temp = temp->right;
			}
			else {
				temp = temp->left;
			}

			

		}
	}

	void DeleteValues(std::vector<int>& offsets) {
		std::vector<BTNode*>* toDelete = new std::vector<BTNode*>;

		auto checkOffsets = [offsets, this, toDelete](BTNode* current) {
			for (auto it = offsets.begin(); it != offsets.end(); it++) {
				if (*it == current->getOffset(numberOfBytes)) {
					toDelete->push_back(current);

				}
			}
			};
		IterrateWithCallback(root, checkOffsets);

		for (auto it = toDelete->begin(); it != toDelete->end(); ) {
			BTNode* pointer = root;
			BTNode* current = *it;
			BTNode* prev = root;
			void* currValue = current->getValue(numberOfBytes);
			bool flag = true;
			while (flag && pointer != nullptr) {
				void* tempValue = pointer->getValue(numberOfBytes);
				int res = VoidMemoryHandler::COMPARE(currValue, tempValue, dataType);
				if (res == LESS) {
					prev = pointer;
					pointer = pointer->left;
				}
				else if (res == BIGGER) {
					prev = pointer;
					pointer = pointer->right;
				}
				else if(res == EQUALS && pointer->getOffset(numberOfBytes) == current->getOffset(numberOfBytes)){
					flag = false;
					it = toDelete->erase(it);
					std::cout << "deleting node " << (int)current->getOffset(numberOfBytes) << std::endl;
					if (pointer->right == nullptr && pointer->left == nullptr) {//no subtrees case
						if (root == pointer) {
							free(root);
							root = nullptr;
						}
						else if (prev->left == pointer) {
							free(prev->left);
							prev->left = nullptr;
						}
						else {
							free(prev->right);
							prev->right = nullptr;
						} 
						
					}
					else if (root == pointer) {
						if (root->right) {
							//select the left subtree of the right node
							BTNode* leftValue = root->right->left;

							//find biggest value in left subtree
							if (root->left) {
								BTNode* biggestValue = nullptr;
								biggestValue = root->left;
								while (biggestValue->right)
									biggestValue = biggestValue->right;
								biggestValue->right = leftValue;
							}

							root->right->left = root->left;
							root = root->right;
							free(pointer);

						}
						else {
							root = root->left;
						}
							
					}
					else {
						if (pointer->right != nullptr) {
							BTNode* leftValue = pointer->right->left;//et the left sub tree of the right value
							pointer->right->left = pointer->left;
							if (prev->left == pointer) 
								prev->left = pointer->right;
							else 
								prev->right = pointer->right;

							//find the biggest value in the left subtree to move the leftValue left subtree to it
							if (pointer->left) {
								BTNode* biggestValue = pointer->left;
								while (biggestValue->right)
									biggestValue = biggestValue->right;
								biggestValue->right = leftValue->left;
								leftValue->left = nullptr;
							}


							free(pointer);
						}
						else {
							if (prev->left == pointer)
								prev->left = pointer->left;
							else
								prev->right = pointer->left;
							free(pointer);
						}
					}
				}
				//if the values are the same but the offset not
				else {
					prev = pointer;
					pointer = pointer->left;
				}

				free(tempValue);
			}
			if (flag)it++;//the last pointer was null, therefore couldnt find the searched value
			free(currValue);
		}
	}

	void IterrateWithCallback(BTNode* current, std::function<void(BTNode*)> callBack) {
		if (!current) return;

		IterrateWithCallback(current->left, callBack);
		callBack(current);
		IterrateWithCallback(current->right, callBack);
	}

	void GetAllValues(void* buffer) {
		int i = 0;
		int size = numberOfBytes + 1;
		auto addToBuffer = [&i, buffer, &size](BTNode* current) {
			std::memcpy((char*)buffer + i * (size), current->value, size);
			i++;
		};
		IterrateWithCallback(root, addToBuffer);
	}

	void GetAllValuesWithBloom(void *buffer, void *bloomData, int bloomSize) {
		int i = 0;
		int size = numberOfBytes + 1;
		auto addToBuffer = [&i, buffer, &size, bloomData, &bloomSize](BTNode* current) {
			std::memcpy((char*)buffer + i * (size), current->value, size);
			void* val = current->getValue(size - 1);
			BloomFilter::AddValue(bloomData, val, size - 1, bloomSize);//-1 for offset byte
			free(val);
			i++;
			};
		IterrateWithCallback(root, addToBuffer);
	}

	void FlushData(BTNode* current) {
		if (current == nullptr) return;

		FlushData(current->left);
		FlushData(current->right);

		delete current;

	}
	void FlushData() {
		FlushData(root);
		root = nullptr;
	};
};
;;;