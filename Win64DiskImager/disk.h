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

#pragma once

// Rounds the file size up to the nearest sector
ULONGLONG calcSizeInSectors(ULONGLONG bytes, ULONGLONG sectorSize);
// Checks the drive type to see if it is a removable device; if so, provides the volume ID
BOOL checkDriveType(const CString &name, DWORD &pid);
// Retrieves a handle to a device (for mounting/unmounting/locking)
HANDLE getHandleOnDevice(int device, DWORD access);
// Retrieves a handle to a volume (raw disk data)
HANDLE getHandleOnVolume(const CString &drive, DWORD access);
// Retrives the geometry of a disk - the number of sectors and the sector size
BOOL getDiskGeometry(HANDLE handle, ULONGLONG &sectorCount, ULONGLONG &sectorSize);
// Given a drive letter (ending in a slash), return the label for that drive
const CString getDriveLabel(const CString &drive);
// Locks the volume to prevent other processes or Explorer from meddling while we work
BOOL getLockOnVolume(HANDLE handle);
// Checks to see if the volume is mounted
BOOL isVolumeMounted(HANDLE handle);
// Unlocks a locked volume
BOOL removeLockOnVolume(HANDLE handle);
// Unmounts a volume from the system
BOOL unmountVolume(HANDLE handle);
