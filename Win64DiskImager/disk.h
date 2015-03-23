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

HANDLE getHandleOnFile(const CString &location, DWORD access);
HANDLE getHandleOnDevice(int device, DWORD access);
HANDLE getHandleOnVolume(int volume, DWORD access);
const CString getDriveLabel(const CString &drive);
BOOL getLockOnVolume(HANDLE handle);
BOOL removeLockOnVolume(HANDLE handle);
BOOL unmountVolume(HANDLE handle);
BOOL isVolumeMounted(HANDLE handle);
BOOL getDiskGeometry(HANDLE handle, ULONGLONG &sectorCount, ULONGLONG &sectorSize);
ULONGLONG calcSizeInSectors(ULONGLONG bytes, ULONGLONG sectorSize);
BOOL spaceAvailable(const CString &location, ULONGLONG spaceNeeded);
BOOL checkDriveType(const CString &name, DWORD &pid);
