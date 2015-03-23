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
#include "Win64DiskImager.h"
#include "ActionParams.h"
#include "Worker.h"
#include "disk.h"
#include "md5.h"

// Handles progress updates
class ProgressHandler {
private:
	// Handle to application where progress notifications go
	const HWND m_app;
	// Last update time
	ULONGLONG m_lastUpdate;
	// Where we were on last update
	ULONGLONG m_lastSector;
	// Sector size
	ULONGLONG m_sectorSize;
	// Sector count
	ULONGLONG m_sectorCount;

public:
	// Based off of action parameter (yeah, envy and overuse...)
	ProgressHandler(CActionParams *params) : m_app(params->m_app), m_lastSector(0ULL) {
		CRawDiskSrc *dest = params->m_dest;
		m_lastUpdate = GetTickCount64();
		if (dest != NULL) {
			// Store sector count and size
			m_sectorCount = dest->GetSectorCount();
			m_sectorSize = dest->GetSectorSize();
		}
	}
	// Copy constructor
	ProgressHandler(const ProgressHandler &other) : m_app(other.m_app),
		m_lastUpdate(other.m_lastUpdate), m_lastSector(other.m_lastSector),
		m_sectorSize(other.m_sectorSize), m_sectorCount(other.m_sectorCount) {}
	virtual ~ProgressHandler() {}

	// Updates the application with the information so far
	BOOL Update(const ULONGLONG sector);
};

// Posts the file error message to the application
static inline void PostFileError(const HWND hApp) {
	PostMessage(hApp, WM_FILE_ERROR, GetLastError(), 0);
}

// Posts the completed MD5 hash as a string to the application
static inline void PostMD5(const HWND hApp, MD5 &md5) {
	md5.Final();
	// ASCII to wide char
	SendMessage(hApp, WM_MD5_READY, 0, (LPARAM)(LPCTSTR)CString(md5.digestChars));
}

// Posts a progress complete message (fills the pbar but does not enable buttons)
static inline void PostProgressComplete(const HWND hApp) {
	PostMessage(hApp, WM_PROGRESS, 65536, 0);
}

// Posts a progress update to the application, with progress from 0..65535 and rate in 100 kB/s
static inline void PostProgress(const HWND hApp, const DWORD progress, const DWORD rate) {
	PostMessage(hApp, WM_PROGRESS, progress, rate);
}

// Checks to see if a progress update is needed
#define KB_100 102400ULL
BOOL ProgressHandler::Update(const ULONGLONG sector) {
	// Do we need it?
	const ULONGLONG now = GetTickCount64(), dt = now - m_lastUpdate;
	if (dt >= UPDATE_RATE) {
		const ULONGLONG denom = dt * KB_100, copied = (sector - m_lastSector) * m_sectorSize;
		// Calculate progress / 65536, rate in 100 kB/s
		PostProgress(m_app, (DWORD)(((sector << 16) + (m_sectorCount >> 1)) / m_sectorCount),
			(DWORD)((copied * 1000ULL + (denom >> 1)) / denom));
		m_lastUpdate = now;
		m_lastSector = sector;
		return TRUE;
	}
	return FALSE;
}

// Posts the completion message and signals any waiting threads
static void Finish(const HWND hApp) {
	PostMessage(hApp, WM_END, 0, 0);
}

// Preallocates disk space for the file, returning true if successful
static BOOL Preallocate(const HANDLE hFile, const ULONGLONG size) {
	
}

// Allocates a buffer
#define ALLOC_BUFFER(_size) ((BYTE *)CoTaskMemAlloc(_size))

// Copies the entire data stream from src to dst, sending progress and error checking
static BOOL SectorCopy(CActionParams *params, CDataSrc *dst, CDataSrc *src) {
	// Parameters
	BOOL status = TRUE, doMD5 = params->m_md5;
	const HWND hApp = params->m_app;
	volatile BOOL *cancel = params->m_cancel;
	ProgressHandler prog(params);
	// Get target streams
	CRawDiskSrc *disk = params->m_dest;
	const ULONGLONG sectorSize = disk->GetSectorSize(),
		sectorCount = calcSizeInSectors(src->Size(), sectorSize);
	MD5 md5;
	// Allocate chunks
	size_t count, bufferSize = (size_t)(CHUNK_SIZE * sectorSize);
	BYTE *data = ALLOC_BUFFER(bufferSize);
	if (data != NULL) {
		if (doMD5)
			md5.Init();
		// Read it in sector by sector
		for (ULONGLONG sector = 0ULL; sector < sectorCount && status; sector += CHUNK_SIZE) {
			ULONGLONG size = sectorCount - sector;
			// Count # of sectors to copy this round
			if (size > CHUNK_SIZE) size = CHUNK_SIZE;
			count = (size_t)(size * sectorSize);
			if (!src->ReadData(sector * sectorSize, (DWORD)count, data) ||
					!dst->WriteData(sector * sectorSize, (DWORD)count, data)) {
				// Error in I/O
				PostFileError(hApp);
				status = FALSE;
			} else {
				// Update progress report
				prog.Update(sector);
				if (doMD5)
					// MD5 calculations
					md5.Update((unsigned char *)data, count);
			}
			if (cancel && *(cancel))
				// Cancel operation
				status = FALSE;
		}
		// All done
		if (doMD5 && status)
			PostMD5(hApp, md5);
		if (status)
			PostProgressComplete(hApp);
		// Do not leak it!
		CoTaskMemFree(data);
	} else
		status = FALSE;
	return status;
}

// Compares hIn and hOut, sending progress and error checking
static BOOL SectorCompare(CActionParams *params) {
	// Parameters
	BOOL status = TRUE, doMD5 = params->m_md5;
	const HWND hApp = params->m_app;
	volatile BOOL *cancel = params->m_cancel;
	ProgressHandler prog(params);
	// Get target streams
	CDataSrc *left = params->m_file;
	CRawDiskSrc *right = params->m_dest;
	const ULONGLONG sectorSize = right->GetSectorSize(), sectorCount = right->GetSectorCount();
	MD5 md5;
	// Allocate chunks
	size_t count, bufferSize = (size_t)(CHUNK_SIZE * sectorSize);
	BYTE *dLeft = ALLOC_BUFFER(bufferSize), *dRight = ALLOC_BUFFER(bufferSize);
	if (dLeft != NULL && dRight != NULL) {
		if (doMD5)
			md5.Init();
		// Read it in sector by sector
		for (ULONGLONG sector = 0ULL; sector < sectorCount && status; sector += CHUNK_SIZE) {
			ULONGLONG size = sectorCount - sector;
			// Count # of sectors to copy this round
			if (size > CHUNK_SIZE) size = CHUNK_SIZE;
			count = (size_t)(size * sectorSize);
			if (!left->ReadData(sector * sectorSize, (DWORD)count, dLeft) ||
					!right->ReadData(sector * sectorSize, (DWORD)count, dRight)) {
				// Error in I/O
				PostFileError(hApp);
				status = FALSE;
			} else if (memcmp(dLeft, dRight, count) != 0) {
				// Verify fail
				PostMessage(hApp, WM_VER_MISMATCH, 0, 0);
				status = FALSE;
			} else {
				// Update progress report
				prog.Update(sector);
				if (doMD5)
					// MD5 calculations, we use LHS data
					md5.Update((unsigned char *)dLeft, count);
			}
			if (cancel && *(cancel))
				// Cancel operation
				status = FALSE;
		}
		// All done
		if (doMD5 && status)
			PostMD5(hApp, md5);
		if (status)
			PostProgressComplete(hApp);
	} else
		status = FALSE;
	// Do not leak it!
	if (dLeft != NULL)
		CoTaskMemFree(dLeft);
	if (dRight != NULL)
		CoTaskMemFree(dRight);
	return status;
}

// Read disk to file
UINT ReadWorker(LPVOID param) {
	CActionParams *params = (CActionParams *)param;
	// Get information
	const HWND hApp = params->m_app;
	CDataSrc *file = params->m_file;
	CRawDiskSrc *dest = params->m_dest;
	// If disk and file are open
	if (file != NULL && dest != NULL) {
		if (file->Preallocate(dest->Size()))
			SectorCopy(params, file, dest);
		else
			PostFileError(hApp);
	}
	Finish(hApp);
	// Do not leak it
	delete params;
	return 0U;
}

// Write file to disk
UINT WriteWorker(LPVOID param) {
	CActionParams *params = (CActionParams *)param;
	// Get information
	CDataSrc *file = params->m_file;
	CRawDiskSrc *dest = params->m_dest;
	// If disk and file are open
	if (file != NULL && dest != NULL)
		SectorCopy(params, dest, file);
	Finish(params->m_app);
	// Do not leak it
	delete params;
	return 0U;
}

// Verify file and disk
UINT VerifyWorker(LPVOID param) {
	CActionParams *params = (CActionParams *)param;
	// Get information
	const HWND hApp = params->m_app;
	CDataSrc *file = params->m_file;
	// If disk and file are open
	if (file != NULL && params->m_dest != NULL && SectorCompare(params)) {
		ULARGE_INTEGER pass;
		pass.QuadPart = file->Size();
		// Verify passed!
		PostMessage(hApp, WM_VER_OK, (WPARAM)pass.HighPart, (LPARAM)pass.LowPart);
	}
	Finish(hApp);
	// Do not leak it
	delete params;
	return 0U;
}
