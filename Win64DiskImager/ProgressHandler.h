#pragma once

#include "ActionParams.h"
#include "RawDiskSrc.h"

// Handles progress updates
class CProgressHandler {
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
	CProgressHandler(CActionParams *params) : m_app(params->m_app), m_lastSector(0ULL) {
		CRawDiskSrc *dest = params->m_dest;
		m_lastUpdate = GetTickCount64();
		if (dest != NULL) {
			// Store sector count and size
			m_sectorCount = dest->GetSectorCount();
			m_sectorSize = dest->GetSectorSize();
		}
	}
	// Copy constructor
	CProgressHandler(const CProgressHandler &other) : m_app(other.m_app),
		m_lastUpdate(other.m_lastUpdate), m_lastSector(other.m_lastSector),
		m_sectorSize(other.m_sectorSize), m_sectorCount(other.m_sectorCount) {}
	virtual ~CProgressHandler() {}

	// Updates the application with the information so far
	BOOL Update(const ULONGLONG sector);
};
