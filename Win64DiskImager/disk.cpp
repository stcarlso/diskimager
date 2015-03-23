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
 *  Copyright (C) 2009, Justin Davis <tuxdavis@gmail.com>             *
 *  Copyright (C) 2009, 2012 ImageWriter developers                   *
 *                           https://launchpad.net/~image-writer-devs *
 **********************************************************************/

#include "stdafx.h"
#include "disk.h"

// Prefix disks with this to get the volume ID
#define DISK_PREFIX _T("\\\\.\\")

// Retrieves a handle to a device (for mounting/unmounting/locking)
HANDLE getHandleOnDevice(int device, DWORD access) {
	CString devicename;
	// Load device name
	devicename.Format(_T("\\\\.\\PhysicalDrive%d"), device);
	return CreateFile(devicename, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, 0, NULL);
}

// Retrieves a handle to a volume (raw disk data)
HANDLE getHandleOnVolume(const CString &drive, DWORD access) {
	CString volumeName;
	// Load volume name
	volumeName.Format(_T("\\\\.\\%c:"), drive.GetAt(0));
	return CreateFile(volumeName, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, 0, NULL);
}

// Locks the volume to prevent other processes or Explorer from meddling while we work
BOOL getLockOnVolume(HANDLE handle) {
	DWORD bytes;
	return DeviceIoControl(handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
}

// Unlocks a locked volume
BOOL removeLockOnVolume(HANDLE handle) {
	DWORD bytes;
	return DeviceIoControl(handle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
}

// Unmounts a volume from the system
BOOL unmountVolume(HANDLE handle) {
	DWORD bytes;
	return DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
}

// Checks to see if the volume is mounted
BOOL isVolumeMounted(HANDLE handle) {
	DWORD bytes;
	return DeviceIoControl(handle, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &bytes, NULL);
}

// Retrives the geometry of a disk - the number of sectors and the sector size
BOOL getDiskGeometry(HANDLE handle, ULONGLONG &sectorCount, ULONGLONG &sectorSize) {
	DWORD bytes;
	DISK_GEOMETRY_EX diskgeometry;
	if (DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskgeometry,
			sizeof(diskgeometry), &bytes, NULL)) {
		// Calculate sector size and count
		const ULONGLONG size = (ULONGLONG)diskgeometry.Geometry.BytesPerSector;
		sectorSize = size;
		sectorCount = (ULONGLONG)diskgeometry.DiskSize.QuadPart / size;
		return TRUE;
	}
	return FALSE;
}

// Rounds the file size up to the nearest sector
ULONGLONG calcSizeInSectors(ULONGLONG bytes, ULONGLONG sectorsize) {
	return (bytes / sectorsize) + ((bytes % sectorsize) ? 1ULL : 0ULL);
}

// Given a drive letter (ending in a slash), return the label for that drive
const CString getDriveLabel(const CString &drive) {
	CString retVal;
	LPTSTR nameBuf = (LPTSTR)CoTaskMemAlloc(sizeof(TCHAR) * (MAX_PATH + 1U));
	if (nameBuf != NULL) {
		if (GetVolumeInformation(drive, nameBuf, MAX_PATH + 1U, NULL, NULL, NULL, NULL, 0))
			// Got the label
			retVal.SetString(nameBuf);
		else
			// I got nothing
			retVal.SetString(_T(""));
	}
	return retVal;
}

// Shortcut for IOCTL_STORAGE_CHECK_VERIFY2
static inline BOOL DeviceIoVerify2(HANDLE hDevice) {
	DWORD b;
	return DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY2, NULL, 0, NULL, 0, &b, NULL);
}

// Obtain a disk property
static BOOL GetDisksProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc,
		PSTORAGE_DEVICE_NUMBER devInfo) {
	STORAGE_PROPERTY_QUERY query;
	DWORD bytes;
	// specify the query type
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;
	// Query using IOCTL_STORAGE_QUERY_PROPERTY 
	if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), pDevDesc,
			pDevDesc->Size, &bytes, NULL)) {
		// Get device number
		if (DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, devInfo,
				sizeof(STORAGE_DEVICE_NUMBER), &bytes, NULL))
			return TRUE;
	} else
		// Removable device check for insertion (no mount!)
		DeviceIoVerify2(hDevice);
	return FALSE;
}

// Given a path, returns a trailing-slashed version in slash and a no-slash version in noSlash
static void slashify(const CString &str, CString &slash, CString &noSlash) {
	const int length = str.GetLength();
	if (length > 0) {
		if (StrCmp(str.Right(1), _T("\\")) == 0) {
			// Ends with slash
			slash.SetString(str);
			noSlash.SetString(str.Left(length - 1));
		} else {
			// Does not end with slash
			slash.SetString(str);
			slash.AppendChar(_T('\\'));
			noSlash.SetString(str);
		}
	} else {
		// Nothing to do!
		slash.SetString(_T(""));
		noSlash.SetString(_T(""));
	}
}

// If the drive is removable or (fixed AND on the usb bus), SD, or MMC
static BOOL acceptableDisk(UINT driveType, STORAGE_BUS_TYPE busType) {
	return (driveType == DRIVE_REMOVABLE && busType != BusTypeSata) ||
		(driveType == DRIVE_FIXED && (busType == BusTypeUsb || busType == BusTypeSd ||
		busType == BusTypeMmc));
}

// Checks the drive to see if it should be used
static inline BOOL _checkDriveType(const CString &nameNoSlash, UINT driveType, DWORD &pid) {
	const size_t arrSz = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 511U;
	PSTORAGE_DEVICE_DESCRIPTOR devDesc;
	STORAGE_DEVICE_NUMBER deviceInfo;
	DWORD bytes;
	BOOL found = FALSE;
	// This one needs no slash
	HANDLE hDevice = CreateFile(nameNoSlash, FILE_READ_ATTRIBUTES, FILE_SHARE_READ |
		FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice != INVALID_HANDLE_VALUE) {
		// Allocate the space needed
		devDesc = (PSTORAGE_DEVICE_DESCRIPTOR)CoTaskMemAlloc(arrSz);
		ASSERT(devDesc != NULL);
		devDesc->Size = arrSz;
		// If disk can be opened and is acceptable
		if (GetDisksProperty(hDevice, devDesc, &deviceInfo) && acceptableDisk(driveType,
				devDesc->BusType)) {
			// Ensure that the drive is actually accessible
			// Multi-card hubs were reporting "removable" even when empty
			if (DeviceIoVerify2(hDevice)) {
				pid = deviceInfo.DeviceNumber;
				found = TRUE;
			} else {
				// IOCTL_STORAGE_CHECK_VERIFY2 fails on some devices under XP/Vista, try the
				// other (slower) method, just in case
				HANDLE hRead = CreateFile(nameNoSlash, FILE_READ_DATA, FILE_SHARE_READ |
					FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				if (hRead != INVALID_HANDLE_VALUE) {
					// Use the verify-with-mount approach
					if (DeviceIoControl(hRead, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0,
							&bytes, NULL)) {
						pid = deviceInfo.DeviceNumber;
						found = TRUE;
					}
					CloseHandle(hRead);
				}
			}
		}
		CloseHandle(hDevice);
		CoTaskMemFree(devDesc);
	}
	return found;
}

// Obtains the drive type, returning true if it looks like an OK drive
BOOL checkDriveType(const CString &name, DWORD &pid) {
	CString nameWithSlash, nameNoSlash;
	UINT driveType;
	// Some calls require no trailing slash, some require a trailing slash...
	slashify(CString(DISK_PREFIX) + name, nameWithSlash, nameNoSlash);
	driveType = GetDriveType(nameWithSlash);
	if (driveType == DRIVE_REMOVABLE || driveType == DRIVE_FIXED)
		return _checkDriveType(nameNoSlash, driveType, pid);
	return FALSE;
}
