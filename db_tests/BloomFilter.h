#include <cstddef>
#include <iostream>
#include <cstring>

#include "BitwiseHandler.h"

class BloomFilter {
private:

    static unsigned int djb2_hash(const void* data, size_t size, unsigned int n) {
        if (size == 0 || n == 0) return 0;

        unsigned long hash = 5381;
        const unsigned char* byteData = static_cast<const unsigned char*>(data);

        for (size_t i = 0; i < size; ++i) {
            hash = ((hash << 5) + hash) + byteData[i]; // hash * 33 + c
        }

        return hash % n;
    }
    static unsigned int fnv1a_hash(const void* data, size_t size, unsigned int n) {
        if (size == 0 || n == 0) return 0;

        unsigned int hash = 2166136261u;
        const unsigned char* byteData = static_cast<const unsigned char*>(data);

        for (size_t i = 0; i < size; ++i) {
            hash ^= byteData[i];        // XOR with the byte
            hash *= 16777619;           // Multiply by the FNV prime
        }

        return hash % n;
    }
    static unsigned int simple_modular_hash(const void* data, size_t size, unsigned int n) {
        if (size == 0 || n == 0) return 0;

        unsigned int hash = 0;
        const unsigned char* byteData = static_cast<const unsigned char*>(data);

        for (size_t i = 0; i < size; ++i) {
            hash += byteData[i];
        }

        return hash % n;
    }

public:
	static void AddValue(void* bloomData, void* newValue, size_t valueSize, int bloomSize) {
        BitwiseHandler::setBit((uint8_t*)bloomData, djb2_hash(newValue, valueSize, bloomSize * 8));
        BitwiseHandler::setBit((uint8_t*)bloomData, fnv1a_hash(newValue, valueSize, bloomSize * 8));
        BitwiseHandler::setBit((uint8_t*)bloomData, simple_modular_hash(newValue, valueSize, bloomSize * 8));

        if(*(int*)newValue == 8 || *(int*)newValue == 200)
        std::cout << "new data with hash " <<
            djb2_hash(newValue, valueSize, bloomSize) << " " <<
            fnv1a_hash(newValue, valueSize, bloomSize) << " " <<
            simple_modular_hash(newValue, valueSize, bloomSize) << std::endl;
	}

    static bool CheckValue(void* bloomData, void* value, size_t valueSize, int bloomSize) {
        bool bit0 = BitwiseHandler::checkBit((uint8_t*)bloomData, djb2_hash(value, valueSize, bloomSize * 8));
        bool bit1 = BitwiseHandler::checkBit((uint8_t*)bloomData, fnv1a_hash(value, valueSize, bloomSize * 8));
        bool bit2 = BitwiseHandler::checkBit((uint8_t*)bloomData, simple_modular_hash(value, valueSize, bloomSize * 8));
        std::cout << bit0 << bit1 << bit2 << std::endl;
        std::cout << "data has hash " <<
            djb2_hash(value, valueSize, bloomSize) << " " <<
            fnv1a_hash(value, valueSize, bloomSize) << " " <<
            simple_modular_hash(value, valueSize, bloomSize) << std::endl;
        if (bit0 && bit1 && bit2) return true;
        return false;
    }

};