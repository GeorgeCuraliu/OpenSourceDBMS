#pragma once

#include <iostream>

class VoidMemoryHandler {
protected:
	static int COMPARE_INT(void* argument1, void* argument2) {
		//std::cout << "arguments " << *(int*)argument1 << " " << *(int*)argument2 << std::endl;
		if (*(int*)argument1 < *(int*)argument2) return -1;
		if (*(int*)argument1 == *(int*)argument2) return 0;
		if (*(int*)argument1 > *(int*)argument2) return 1;
	}
public:
	static void INT(void* memory) {

	}
	static int COMPARE(void* argument1, void* argument2, char* dataType) {
		//std::cout << "comparing for type " << dataType << " ";
		if (strcmp(dataType, "int") == 0) {
			//std::cout << "-- CONFIRMATION int -- ";
			return COMPARE_INT(argument1, argument2);
		}
	}
};