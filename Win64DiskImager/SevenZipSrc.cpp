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
#include "SevenZipSrc.h"

// Extension to apply to files
#define EXTENSION _T(".img")

// Parameters for compression
class CompressParams {
public:
	// The compressor object from 7-zip dll
	SevenZip::SevenZipCompressor *m_compr;
	// Name of the file that will be placed inside the compressed zip
	CString m_name;
	// Size of the input in bytes
	ULONGLONG m_size;
	// Disk stream used for I/O
	CDiskStream *m_stream;

	CompressParams(SevenZip::SevenZipCompressor *compr, CDiskStream *stream, ULONGLONG size,
			const CString &name) : m_compr(compr), m_size(size), m_stream(stream),
			m_name(name) {
		stream->AddRef();
	}
	virtual ~CompressParams() {
		if (m_stream != NULL)
			m_stream->Release();
		if (m_compr != NULL)
			delete m_compr;
	}
};

// Parameters for extraction
class ExtractParams {
public:
	// Archive to extract
	IInArchive *m_archive;
	// The extractor object from 7-zip dll
	SevenZip::SevenZipExtractor *m_extract;
	// Index to extract
	UINT m_index;
	// Disk stream used for I/O
	CDiskStream *m_stream;

	ExtractParams(SevenZip::SevenZipExtractor *extr, CDiskStream *stream, IInArchive *archive,
			const UINT index) : m_extract(extr), m_stream(stream), m_index(index),
			m_archive(archive) {
		stream->AddRef();
	}
	virtual ~ExtractParams() {
		if (m_stream != NULL)
			m_stream->Release();
		if (m_extract != NULL)
			delete m_extract;
	}
};

// Close the file
void CSevenZipSrc::Close() {
	const HANDLE hThread = m_quit;
	m_stream->EndOfFile();
	if (hThread != INVALID_HANDLE_VALUE) {
		// Wait for the thread to terminate (7-zip threads on ultra on slow machines can take a
		// VERY long time to quit)
		while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0);
		CloseHandle(hThread);
		m_quit = INVALID_HANDLE_VALUE;
	}
}

// Initialize by starting up the background thread
BOOL CSevenZipSrc::Init() {
	m_thread->m_bAutoDelete = TRUE;
	return m_thread->ResumeThread() != (DWORD)0xFFFFFFFF;
}

// Read data from the device, into the array
BOOL CSevenZipSrc::ReadData(DWORD count, BYTE *data) {
	ULONG read, want = (ULONG)count;
	return SUCCEEDED(m_stream->Read(data, want, &read)) && read == want;
}

// Retrieve the size of the (uncompressed) data
ULONGLONG CSevenZipSrc::Size() {
	return m_size;
}

// Write data to the device, from the array
BOOL CSevenZipSrc::WriteData(DWORD count, BYTE *data) {
	ULONG written, want = (ULONG)count;
	return SUCCEEDED(m_stream->Write(data, want, &written)) && written == want;
}

// Renames the file name to just a path spec + .img
static const CString RenameToIMG(const CString &name) {
	const size_t length = (size_t)(name.GetLength() + 5);
	// In case the file is just "file" or something similar, make sure there is room for null
	// terminator and the entire extension
	LPTSTR newName = (LPTSTR)CoTaskMemAlloc(sizeof(TCHAR) * length);
	CString ret;
	if (newName != NULL) {
		// Copy string to new buffer
		if (SUCCEEDED(StringCchCopy(newName, length, PathFindFileName(name.GetString()))) &&
				SUCCEEDED(PathCchRenameExtension(newName, length, EXTENSION)))
			ret.SetString(newName);
		CoTaskMemFree(newName);
	} else
		// Oh no!
		ret.SetString(name);
	return ret;
}

// Creates a handle to a new 7-zip file to be used for output
CSevenZipSrc * CSevenZipSrc::CreateNew(const CString &file, SevenZip::SevenZipLibrary &lib,
		const ULONGLONG size) {
	const CString imgName = RenameToIMG(file);
	// Create compressor
	SevenZip::SevenZipCompressor *compressor = new SevenZip::SevenZipCompressor(lib,
		file.GetString());
	BOOL ok = TRUE;
	// Go to the maximum
	try {
		compressor->SetCompressionLevel(SevenZip::CompressionLevel::_Enum::Ultra);
	} catch (SevenZip::SevenZipException &e) {
		(void)e;
		ok = FALSE;
	}
	if (ok) {
		// Create COM stream to share with 7-Zip
		CComPtr<CDiskStream> stream = new CDiskStream(FALSE);
		CompressParams *params = new CompressParams(compressor, stream, size, imgName);
		CWinThread *thread;
		if (stream->CreatePipe() && (thread = AfxBeginThread(CompressWorker, params, 0, 0U,
				CREATE_SUSPENDED)) != NULL)
			// Allocate disk stream (throws if OOM)
			return new CSevenZipSrc(stream, 0ULL, thread);
		// Do not leak it
		delete params;
	}
	delete compressor;
	return NULL;
}

// Opens a handle to an existing 7-zip file to be used for input
CSevenZipSrc * CSevenZipSrc::OpenExisting(const CString &file, SevenZip::SevenZipLibrary &lib) {
	// Create compressor
	SevenZip::SevenZipExtractor *extractor = new SevenZip::SevenZipExtractor(lib,
		file.GetString());
	// Create COM stream to share with 7-Zip
	UINT index;
	ULONGLONG size;
	IInArchive *archive = extractor->FindLargestFile(index, size);
	if (archive != NULL) {
		CComPtr<CDiskStream> stream = new CDiskStream(TRUE);
		ExtractParams *params = new ExtractParams(extractor, stream, archive, index);
		CWinThread *thread;
		if (stream->CreatePipe() && (thread = AfxBeginThread(DecompressWorker, params, 0, 0U,
				CREATE_SUSPENDED)) != NULL)
			// Allocate disk stream (throws if OOM)
			return new CSevenZipSrc(stream, size, thread);
		// Do not leak it
		delete params;
		archive->Release();
	}
	delete extractor;
	return NULL;
}

// Compresses a 7z file in the background
UINT CompressWorker(LPVOID param) {
	CompressParams *params = (CompressParams *)param;
	LPCTSTR fileName = params->m_name.GetBuffer();
	// Start it up
	try {
		params->m_compr->CompressStream(fileName, params->m_stream, params->m_size);
	} catch (SevenZip::SevenZipException &e) {
		(void)e;
	}
	// Do not leak it
	delete params;
	return 0U;
}

// Decompresses a 7z file in the background
UINT DecompressWorker(LPVOID param) {
	ExtractParams *params = (ExtractParams *)param;
	SevenZip::SevenZipExtractor *extract = params->m_extract;
	// Start it up
	try {
		extract->ExtractSingleFile(params->m_archive, params->m_stream, params->m_index);
	} catch (SevenZip::SevenZipException &e) {
		(void)e;
	}
	// Do not leak it
	delete params;
	return 0U;
}
