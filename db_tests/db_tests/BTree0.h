#pragma once

#include <iostream>
#include <functional>

#include "VoidMemoryHandler.h"

class BTNode {
public:
	BTNode* left, * right;
	void* value;
	BTNode(void* data, uint8_t offset, int dataSize) :left(nullptr), right(nullptr) {
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
		if (!root) {
			std::cout << "new root" << std::endl;
			root = new BTNode(value, offset, numberOfBytes);
			return;
		}

		BTNode* temp = root;
		while (temp) {
			if (VoidMemoryHandler::COMPARE(value, temp->getValue(numberOfBytes), dataType) <= 0) {
				std::cout << (int)temp->getOffset(numberOfBytes) << " left " << std::endl;
				if (temp->left == nullptr) {
					temp->left = new BTNode(value, offset, numberOfBytes);
					break;
				}
				else temp = temp->left;
			}
			else {
				std::cout << (int)temp->getOffset(numberOfBytes) << " right " << std::endl;
				if (temp->right == nullptr) {
					temp->right = new BTNode(value, offset, numberOfBytes);
					break;
				}
				else temp = temp->right;
			}
		}

	}
	uint8_t FindValue(void* value) {
		BTNode* temp = root;
		while (temp) {
			int comparationResult = VoidMemoryHandler::COMPARE(value, temp->getValue(numberOfBytes), dataType);
			std::cout << comparationResult << std::endl;
			if (comparationResult == 0) {
				std::cout << "found bucket at offset " << (int)temp->getOffset(numberOfBytes) << std::endl;
				return temp->getOffset(numberOfBytes);
			}
			else if (comparationResult == -1) temp = temp->left;
			else if (comparationResult == 1) temp = temp->right;
		}
		return -1;
	}

	void IterrateWithCallback(BTNode* current, std::function<void(void*)> callBack) {
		if (!current) return;

		IterrateWithCallback(current->left, callBack);
		callBack(current->value);
		IterrateWithCallback(current->right, callBack);
	}

	void GetAllValues(void* buffer) {
		int i = 0;
		auto addToBuffer = [&i, buffer, size = numberOfBytes](void* values) {
			std::memcpy((char*)buffer + i * size, values, size);
		};
		IterrateWithCallback(root, addToBuffer);
	}

	void FlushData(BTNode* current = 0) {
		if (!current) current = root;
		if (current->left) FlushData(current->left);
		if (current->right) FlushData(current->right);
		if (current = root) {
			root = nullptr;
		}
		delete current;
	}
};