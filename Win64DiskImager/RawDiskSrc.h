#pragma once
#include "FileSrc.h"

// A raw disk source, basically a file source with a fancy open method and sector info
class CRawDiskSrc : public CFileSrc {
private:
	// # of sectors
	ULONGLONG m_sectorCount;
	// Sector size
	ULONGLONG m_sectorSize;

protected:
	// Handle to (locked) volume
	HANDLE m_volume;

public:
	// Make one out of an existing disk
	CRawDiskSrc(const HANDLE disk, const HANDLE volume);
	// Copy constructor
	CRawDiskSrc(const CRawDiskSrc &other) : CFileSrc(other), m_sectorCount(other.m_sectorCount),
		m_sectorSize(other.m_sectorSize), m_volume(other.m_volume) {}

	// Close the device
	virtual void Close();
	// Preallocate space for the disk (does nothing!)
	virtual BOOL Preallocate(ULONGLONG size);
	// Retrieve the actual size in bytes
	virtual ULONGLONG Size();

	// Retrieve the number of sectors
	inline ULONGLONG GetSectorCount() const {
		return m_sectorCount;
	}
	// Retrieve the sector size in bytes
	inline ULONGLONG GetSectorSize() const {
		return m_sectorSize;
	}

	// Locks the device and unmounts it
	static HANDLE LockAndUnmount(const CString &disk, const DWORD access);
	// Opens a disk path and retrieves a handle to the raw data
	static HANDLE Open(const CString &disk, const DWORD access);
};
