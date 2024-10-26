#include <windows.h>
#include <fileapi.h>
#include <iostream>
#include <string>


int main0(int argv, char* argc[]) {
	HANDLE basicHandle;
	wchar_t buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring path(buffer);
	size_t pos = path.find_last_of(L"\\/");
	std::wstring fileName = path.substr(0, pos) + L"\\test.txt";
	basicHandle = CreateFile(
		fileName.c_str(),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (basicHandle == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to create the file. Error: " << GetLastError() << std::endl;
		return 1;
	}

	char *dataToRead = new char[20];
	unsigned long numberOfBytesRead = 0;

	OVERLAPPED overlapped = { 0 };
	overlapped.Offset = 0;  // Start writing from the beginning of the file
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // Event to signal when I/O is complete

	/*WriteFile(
		basicHandle,
		(LPCVOID)&dataToRead,
		(DWORD)50,
		(LPDWORD)numberOfBytesRead,
		&overlapped
	);*/

	const char* dataToWrite = "322";
	DWORD bytesToWrite = strlen(dataToWrite);
	DWORD bytesWritten;

	bool result = WriteFile(
		basicHandle,
		(LPCVOID)dataToWrite,	
		bytesToWrite,                  
		&bytesWritten,
		NULL
	);

	if (!result) {
		std::cerr << "Failed to write to the file. Error: " << GetLastError() << std::endl;
		CloseHandle(basicHandle);
		return 1;
	}

	std::cout << "Successfully wrote " << bytesWritten << " bytes to the file." << std::endl;

	result = ReadFile(
		basicHandle,
		LPVOID(dataToRead),
		bytesToWrite,
		&numberOfBytesRead,
		NULL
	);

	for (int i = 0; i < 20; i++) {
		std::cout << dataToRead[i] << std::endl;
		dataToRead[i] = *"2";
	}

	result = WriteFile(
		basicHandle,
		(LPCVOID)dataToRead,
		bytesToWrite,
		&bytesWritten,
		NULL
	);

	CloseHandle(basicHandle);

	std::cout << bytesToWrite << std::endl;

	std::cin.get();
	return 0;
}