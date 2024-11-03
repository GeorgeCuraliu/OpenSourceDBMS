#pragma once

constexpr int SHORT_INT_SIZE = 4;
constexpr int SEGMENT_SIZE = 512;
constexpr int BUFFER_SIZE = 4096;

//comparators
constexpr int EQUALS = 0x1;
constexpr int LESS = 0x2;
constexpr int BIGGER = 0x4;
constexpr int NOT = 0x8;

//operations
constexpr int AND = 0x1;
constexpr int OR = 0x2;
