#include "stdafx.h"
#include "RawDiskSrc.h"
#include "disk.h"

// Populates the geometry fields
CRawDiskSrc::CRawDiskSrc(const HANDLE disk, const HANDLE volume) : CFileSrc(disk),
		m_volume(volume) {
	// Open the raw disk
	if (disk != INVALID_HANDLE_VALUE)
		getDiskGeometry(disk, m_sectorCount, m_sectorSize);
	else {
		m_sectorCount = 0ULL;
		m_sectorSize = 0ULL;
	}
}

// Close the device
void CRawDiskSrc::Close() {
	CFileSrc::Close();
	if (m_volume != NULL) {
		removeLockOnVolume(m_volume);
		CloseHandle(m_volume);
	}
}

// Locks the device and unmounts it
HANDLE CRawDiskSrc::LockAndUnmount(const CString &disk, const DWORD access) {
	HANDLE hVolume;
	// Calculate volume from disk path
	int volume = (int)(disk.GetAt(0) - _T('A'));
	if (volume < 0 || volume > 25) volume = 0;
	// Open volume
	if (SUCCEEDED(hVolume = getHandleOnVolume(volume, access))) {
		// Lock volume
		if (getLockOnVolume(hVolume)) {
			// Unmount volume
			if (unmountVolume(hVolume))
				return hVolume;
			removeLockOnVolume(hVolume);
		}
		CloseHandle(hVolume);
	}
	return INVALID_HANDLE_VALUE;
}

// Opens a disk for reading and/or writing, returning the handle to be used
HANDLE CRawDiskSrc::Open(const CString &disk, const DWORD access) {
	HANDLE hRaw;
	DWORD id;
	// Disk must be a useful type
	if (checkDriveType(disk, id) && SUCCEEDED(hRaw = getHandleOnDevice(id, access)))
		return hRaw;
	return INVALID_HANDLE_VALUE;
}

// Preallocate space for the disk (does nothing!)
BOOL CRawDiskSrc::Preallocate(ULONGLONG size) {
	return FALSE;
}

// The default call does not work on disks, so we return the calculated size instead!
ULONGLONG CRawDiskSrc::Size() {
	return m_sectorCount * m_sectorSize;
}
