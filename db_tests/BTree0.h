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
			if (VoidMemoryHandler::COMPARE(value, temp->getValue(numberOfBytes), dataType) | (EQUALS | LESS)) {
				std::cout << (int)temp->getOffset(numberOfBytes) << " left " << std::endl;
				if (temp->left == nullptr) {
					temp->left = newNode;
					break;
				}
				else temp = temp->left;
			}
			else {
				std::cout << (int)temp->getOffset(numberOfBytes) << " right " << std::endl;
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
			std::cout << "found match at offset " << current->getOffset(numberOfBytes) << " " << *(int*)current->getValue(numberOfBytes) << std::endl;
			foundValues.push_back(current->getOffset(numberOfBytes));
			};

		BTNode* temp = root;
		while (temp) {
			int comparationResult = VoidMemoryHandler::COMPARE(temp->getValue(numberOfBytes), value, dataType);

			//just add the offset if it matches the equals comparator
			if (comparator == EQUALS && comparationResult == EQUALS) {
				foundValues.push_back((int)temp->getOffset(numberOfBytes));
				std::cout << "found match at -equals- offset " << (int)temp->getOffset(numberOfBytes) << " " << *(int*)temp->getValue(numberOfBytes) << std::endl;
				temp = temp->left;
			}
			//iterate the entire subtree that matches the description
			if (comparator & comparationResult) {
				if (comparator & LESS) {
					IterrateWithCallback(temp->left, GetOffsetsFromSubtree);
					temp = temp->right;
				}
				else if (comparator & BIGGER) {
					IterrateWithCallback(temp->right, GetOffsetsFromSubtree);
					temp = temp->left;
				}
			}

			

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
			BloomFilter::AddValue(bloomData, current->getValue(size - 1), size - 1, bloomSize);//-1 for offset byte
			i++;
			};
		IterrateWithCallback(root, addToBuffer);
	}

	void FlushData(BTNode* current = nullptr) {
		if (current == nullptr) current = root;
		if (current == nullptr) return;
		if (current->left) FlushData(current->left);
		if (current->right) FlushData(current->right);
		
		if (current == root && current != nullptr) {
			delete root;
			root = nullptr;
		}
		else if(current == nullptr) {
			delete current;
		}
		
	}
};