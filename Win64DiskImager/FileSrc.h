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
