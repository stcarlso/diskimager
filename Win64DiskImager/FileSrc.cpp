#include "stdafx.h"
#include "FileSrc.h"

// Close the file
void CFileSrc::Close() {
	if (m_file != INVALID_HANDLE_VALUE && m_file != NULL)
		CloseHandle(m_file);
}

// Preallocate space for the disk (does nothing!)
BOOL CFileSrc::Preallocate(ULONGLONG size) {
	LARGE_INTEGER space;
	space.QuadPart = size;
	return SetFilePointerEx(m_file, space, NULL, FILE_BEGIN) && SetEndOfFile(m_file) &&
		SetFilePointer(m_file, 0L, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}

// Read data from the device, into the array
BOOL CFileSrc::ReadData(ULONGLONG start, DWORD count, BYTE *data) {
	// Will run OOM long before overflowing 32-bit integer for # of bytes to read
	DWORD bytes;
	LARGE_INTEGER li;
	// Move to the right offset
	li.QuadPart = start;
	SetFilePointer(m_file, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (ReadFile(m_file, data, count, &bytes, NULL)) {
		if (bytes < count)
			// Fill it with zeroes, this is important, use SecureZeroMemory to not optimize me
			SecureZeroMemory(data + bytes, count - bytes);
		return TRUE;
	}
	return FALSE;
}

// Get the file size in bytes!
ULONGLONG CFileSrc::Size() {
	LARGE_INTEGER filesize;
	if (GetFileSizeEx(m_file, &filesize))
		// Calculate the number of sectors, possibly plus one
		return (ULONGLONG)filesize.QuadPart;
	return 0ULL;
}

// Write data to the device, from the array
BOOL CFileSrc::WriteData(ULONGLONG start, DWORD count, BYTE *data) {
	DWORD bytes;
	LARGE_INTEGER li;
	li.QuadPart = start;
	SetFilePointer(m_file, li.LowPart, &li.HighPart, FILE_BEGIN);
	return WriteFile(m_file, data, count, &bytes, NULL);
}

// Opens the 
HANDLE CFileSrc::Open(LPCTSTR name, const DWORD access) {
	// Set sharing/creation flags automatically
	const BOOL write = (access & GENERIC_WRITE) != 0;
	const DWORD share = write ? 0 : FILE_SHARE_READ;
	const DWORD create = write ? CREATE_ALWAYS : OPEN_EXISTING;
	return CreateFile(name, access, share, NULL, create, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
}
