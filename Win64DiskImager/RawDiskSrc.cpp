/**********************************************************************
 *  This program is free software; you can redistribute it and/or     *
 *  modify it under the terms of the GNU General Public License       *
 *  as published by the Free Software Foundation; either version 2    *
 *  of the License, or (at your option) any later version.            *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 *  GNU General Public License for more details.                      *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, write to the Free Software       *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,                *
 *  Boston, MA  02110-1301, USA.                                      *
 *                                                                    *
 *  ---                                                               *
 *  Copyright (C) 2014, Stephen Carlson                               *
 *                                  <https://www.github.com/stcarlso> *
 **********************************************************************/

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
	// Open volume
	if (SUCCEEDED(hVolume = getHandleOnVolume(disk, access))) {
		// Lock volume
		if (getLockOnVolume(hVolume)) {
			// Unmount volume
			if (!isVolumeMounted(hVolume) || unmountVolume(hVolume))
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
