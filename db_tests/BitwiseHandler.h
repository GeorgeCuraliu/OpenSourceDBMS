#pragma once

class BitwiseHandler {
public:
	static void setBit(uint8_t* memory, int bitPos) {
		memory[bitPos / 8] |= (1 << (bitPos % 8));
	}
	static void clearBit(uint8_t* memory, int bitPos) {
		memory[bitPos / 8] &= ~(1 << (bitPos % 8));
	}
	static bool checkBit(uint8_t* memory, int bitPos) {
		return memory[bitPos / 8] & (1 << (bitPos % 8));
	}
};