#pragma once
#include "DataSrc.h"

// Represents a disk file data source
class CFileSrc : public CDataSrc {
protected:
	// Handle to the file
	const HANDLE m_file;

public:
	// Create from opened file
	CFileSrc(const HANDLE file) : m_file(file) {}
	// Copy constructor
	CFileSrc(const CFileSrc &other) : m_file(other.m_file) {}

	// Close the device
	virtual void Close();
	// Opens a file name and retrieves a handle to its data
	static HANDLE Open(LPCTSTR name, const DWORD access);
	// Preallocate space for the file
	virtual BOOL Preallocate(ULONGLONG size);
	// Read data from the device, into the array
	virtual BOOL ReadData(ULONGLONG start, DWORD count, BYTE *data);
	// Retrieve the actual size in bytes
	virtual ULONGLONG Size();
	// Write data to the device, from the array
	virtual BOOL WriteData(ULONGLONG start, DWORD count, BYTE *data);
};
