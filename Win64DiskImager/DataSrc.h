#pragma once

// Abstract data source class which superclasses disks, files, and compressed files
class CDataSrc {
public:
	CDataSrc() { }
	CDataSrc(const CDataSrc &other) { }
	virtual ~CDataSrc() { }

	// Close the device
	virtual void Close() { }
	// Preallocate space for this item (optional)
	virtual BOOL Preallocate(ULONGLONG size) {
		return FALSE;
	}
	// Read data from the device, into the array
	virtual BOOL ReadData(ULONGLONG start, DWORD count, BYTE *data) = 0;
	// Retrieve the size of the data
	virtual ULONGLONG Size() = 0;
	// Write data to the device, from the array
	virtual BOOL WriteData(ULONGLONG start, DWORD count, BYTE *data) = 0;
};
