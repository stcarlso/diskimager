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
#include "DiskStream.h"
#include "7zpp.h"

// A data source supporting read/write to 7-zip archives. Input is limited to archives with only
// one file, anything else will fail in Open()
class CSevenZipSrc : public CDataSrc {
protected:
	// Size of the file
	ULONGLONG m_size;
	// Archive currently open
	CDiskStream *m_stream;
	// Use this to know when to quit out
	HANDLE m_quit;
	// Thread (started suspended) to do the dirty work
	CWinThread *m_thread;
	
	// Default constructor
	CSevenZipSrc(CDiskStream *stream, const ULONGLONG size, CWinThread *thread) :
			m_stream(stream), m_size(size), m_thread(thread) {
		const HANDLE srcThread = m_thread->m_hThread;
		m_stream->AddRef();
		// Copy handle to avoid double close
		if (!DuplicateHandle(GetCurrentProcess(), srcThread, GetCurrentProcess(), &m_quit, 0,
				FALSE, DUPLICATE_SAME_ACCESS))
			m_quit = srcThread;
	}
	virtual ~CSevenZipSrc() {
		m_stream->Release();
	}
public:
	// Close the file
	virtual void Close();
	// Initialize the 7-zip file (start compress/decompress thread)
	virtual BOOL Init();
	// Read data from the device, into the array
	virtual BOOL ReadData(DWORD count, BYTE *data);
	// Retrieve the size of the data
	virtual ULONGLONG Size();
	// Write data to the device, from the array
	virtual BOOL WriteData(DWORD count, BYTE *data);

	// Creates a handle to a new 7-zip file to be used for output
	static CSevenZipSrc * CreateNew(const CString &file, SevenZip::SevenZipLibrary &lib,
		const ULONGLONG size);
	// Opens a handle to an existing 7-zip file to be used for input
	static CSevenZipSrc * OpenExisting(const CString &file, SevenZip::SevenZipLibrary &lib);
};
