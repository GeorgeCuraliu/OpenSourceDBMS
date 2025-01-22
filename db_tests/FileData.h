#pragma once

#include "Parameters.h"
#include <math.h>

class FileData {
public:
	int totalValuesPerSegment, valuesPerSegment, bufferSize, wroteValues, segmentPointer, numberOfRegisters, valuesToWrite, valueSize, metadata;
	FileData(int valueSize, int registersNumber, int metadata): numberOfRegisters(registersNumber), valuesToWrite(0), valueSize(valueSize), metadata(metadata) {
		totalValuesPerSegment = floor(BUFFER_SIZE / double(valueSize));
		valuesPerSegment = floor(BUFFER_SIZE / double(valueSize));
		if (valuesPerSegment > 128) valuesPerSegment = 128;
		bufferSize = valueSize * valuesPerSegment;
		wroteValues = 0;
		segmentPointer = BUFFER_SIZE * (numberOfRegisters * ceil(128.f / (double)valuesPerSegment)) + metadata;
	};

	//prefix operator
	FileData& operator++() {
		segmentPointer += BUFFER_SIZE;
		wroteValues += valuesPerSegment;
		if (wroteValues + valuesPerSegment <= 128) {
			valuesToWrite = valuesPerSegment;
		}
		else {//the case for remainig values of a buffer
			valuesToWrite = 128 - wroteValues;
		}
		return *this;
	}

	//this operator works in buffer units, therefore '++' = '+= BUFFER_SIZE'
	//it will perform all needed operations for the rest of member variables
	//postfix operator
	FileData operator++(int){
		FileData temp = *this;
		segmentPointer += BUFFER_SIZE;
		wroteValues += valuesPerSegment;
		if (wroteValues + valuesPerSegment <= 128) {
			valuesToWrite = valuesPerSegment;
		}
		else {//the case for remainig values of a buffer
			valuesToWrite = 128 - wroteValues;
		}
		return temp;
	}

	//used to fill a segment, if there is remaining space from last register
	void fillEmptySpace(int &offset) {
		int segment = 128 / valuesPerSegment;
		int writtenValues;
		if (valuesPerSegment == 128) writtenValues = valuesPerSegment;
		else writtenValues = 128 % valuesPerSegment;
		offset = BUFFER_SIZE * (segment - 1) + writtenValues * (valueSize) + metadata;
		valuesToWrite = totalValuesPerSegment - writtenValues;
		if (valuesToWrite > 128) valuesToWrite = 128;
		wroteValues += valuesToWrite;
	}
};