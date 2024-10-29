#include <windows.h>
#include <iostream>
#include <fileapi.h>

#include "BTree2.h"
#include "BTree.h"

int main1(int argv, char* argc[]) {

	std::wstring filePath = L"test.txt";
	HANDLE fileHandle;

	fileHandle = CreateFile(
		filePath.c_str(),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);


	DWORD fileSize = GetFileSize(fileHandle, NULL);
	if (fileSize == INVALID_FILE_SIZE) {
		std::cerr << "Failed to get file size, error: " << GetLastError() << std::endl;
	}
	else if (fileSize == 0) {
		std::cerr << "The file is empty." << std::endl;
		CloseHandle(fileHandle);
		return 1;
	}


	INT8 buffer[5] = { 0 };
	DWORD bytesRead = 0;

	bool res = ReadFile(
		fileHandle,
		buffer,
		sizeof(buffer),
		&bytesRead,
		NULL
	);

	if (!res) {
		std::cerr << "ReadFile failed, error: " << GetLastError() << std::endl;
		CloseHandle(fileHandle);  // Always close the file handle when done
		return 1;
	}

	// Output the result
	std::cout << "Bytes read: " << bytesRead << std::endl;

	for (int i = 0; i < bytesRead; i++) {
		std::cout << "Byte " << i + 1 << ": " << buffer[i] << std::endl;
	}

	CloseHandle(fileHandle);  // Close the file handle


	//BTree<int>* dataTree = new BTree<int>();
	//for (int i = 0; i < bytesRead; i++)dataTree->addNode(buffer[i] - 48); // -48 for conversion from char to int
	//dataTree->printInOrder(dataTree->root);

	BTree2<int>* tree = new BTree2<int>();
	for (int i = 0; i < bytesRead; i++)  tree->insert(buffer[i] - 48);
	tree->printInorder();













	//GET SEGMENT SIZE

	// Path to the physical drive (PhysicalDrive0 is the first drive)
	const char* drivePath = "\\\\.\\PhysicalDrive0";

	// Open a handle to the physical drive
	HANDLE hDevice = CreateFileA(drivePath,       // Drive path
		GENERIC_READ,    // Access mode
		FILE_SHARE_READ | FILE_SHARE_WRITE, // Share mode
		NULL,            // Security attributes
		OPEN_EXISTING,   // Open existing
		0,               // Flags and attributes
		NULL);           // Template file

	if (hDevice == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open the drive. Error: " << GetLastError() << std::endl;
		return 1;
	}

	// Structure to receive the drive geometry information
	DISK_GEOMETRY diskGeometry;
	DWORD bytesReturned;

	// Send control code to get the drive geometry
	BOOL success = DeviceIoControl(hDevice,               // Handle to the device
		IOCTL_DISK_GET_DRIVE_GEOMETRY, // Control code
		NULL,                 // Input buffer (not needed)
		0,                    // Input buffer size
		&diskGeometry,        // Output buffer
		sizeof(diskGeometry), // Output buffer size
		&bytesReturned,       // Number of bytes returned
		NULL);                // Overlapped (not needed)

	if (success) {
		// Output the sector size
		std::cout << "Bytes per sector: " << diskGeometry.BytesPerSector << std::endl;
	}
	else {
		std::cerr << "Failed to get drive geometry. Error: " << GetLastError() << std::endl;
	}

	// Close the handle to the drive
	CloseHandle(hDevice);
	








	return 0;
}