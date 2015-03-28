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
#include "ProgressHandler.h"
#include "Worker.h"
#include "disk.h"
#include "md5.h"

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

// Posts the completion message and signals any waiting threads
static void Finish(const HWND hApp) {
	PostMessage(hApp, WM_END, 0, 0);
}

// Allocates a buffer
#define ALLOC_BUFFER(_size) ((BYTE *)CoTaskMemAlloc(_size))

// Copies the entire data stream from src to dst, sending progress and error checking
static BOOL SectorCopy(CActionParams *params, CDataSrc *dst, CDataSrc *src) {
	// Parameters
	BOOL status = TRUE, doMD5 = params->m_md5;
	const HWND hApp = params->m_app;
	volatile BOOL *cancel = params->m_cancel;
	CProgressHandler prog(params);
	// Get target streams
	CRawDiskSrc *disk = params->m_dest;
	const ULONGLONG sectorSize = disk->GetSectorSize(), sectorCount = calcSizeInSectors(
		src->Size(), sectorSize);
	MD5 md5;
	// Allocate chunks, use smaller chunks when compressing to 
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
			if (!src->ReadData((DWORD)count, data) || !dst->WriteData((DWORD)count, data)) {
				// Error in I/O
				PostFileError(hApp);
				status = FALSE;
			} else if (cancel && *(cancel))
				// Cancel operation
				status = FALSE;
			else {
				// Update progress report
				prog.Update(sector);
				if (doMD5)
					// MD5 calculations
					md5.Update((unsigned char *)data, count);
			}
		}
		// All done
		if (status) {
			if (doMD5)
				PostMD5(hApp, md5);
			PostProgressComplete(hApp);
		}
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
	CProgressHandler prog(params);
	// Get target streams
	CDataSrc *left = params->m_file;
	CRawDiskSrc *right = params->m_dest;
	const ULONGLONG sectorSize = right->GetSectorSize(), sectorCount = calcSizeInSectors(
		left->Size(), sectorSize);
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
			if (!left->ReadData((DWORD)count, dLeft) || !right->ReadData((DWORD)count, dRight)) {
				// Error in I/O
				PostFileError(hApp);
				status = FALSE;
			} else if (memcmp(dLeft, dRight, count) != 0) {
				// Verify fail
				PostMessage(hApp, WM_VER_MISMATCH, 0, 0);
				status = FALSE;
			} else if (cancel && *(cancel))
				// Cancel operation
				status = FALSE;
			else {
				// Update progress report
				prog.Update(sector);
				if (doMD5)
					// MD5 calculations, we use LHS data
					md5.Update((unsigned char *)dLeft, count);
			}
		}
		// All done
		if (status) {
			if (doMD5)
				PostMD5(hApp, md5);
			PostProgressComplete(hApp);
		}
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
		if (file->Init() && file->Preallocate(dest->Size()))
			SectorCopy(params, file, dest);
		else
			PostFileError(hApp);
	}
	// Do not leak it
	params->Release();
	Finish(hApp);
	delete params;
	return 0U;
}

// Write file to disk
UINT WriteWorker(LPVOID param) {
	CActionParams *params = (CActionParams *)param;
	// Get information
	const HWND hApp = params->m_app;
	CDataSrc *file = params->m_file;
	CRawDiskSrc *dest = params->m_dest;
	// If disk and file are open
	if (file != NULL && dest != NULL) {
		if (file->Init())
			SectorCopy(params, dest, file);
		else
			PostFileError(hApp);
	}
	// Do not leak it
	params->Release();
	Finish(hApp);
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
	if (file != NULL && params->m_dest != NULL) {
		if (!file->Init())
			PostFileError(hApp);
		else if (SectorCompare(params)) {
			ULARGE_INTEGER pass;
			pass.QuadPart = file->Size();
			// Verify passed!
			PostMessage(hApp, WM_VER_OK, (WPARAM)pass.HighPart, (LPARAM)pass.LowPart);
		}
	}
	// Do not leak it
	params->Release();
	Finish(hApp);
	delete params;
	return 0U;
}
