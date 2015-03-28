#include "stdafx.h"
#include "Win64DiskImager.h"
#include "Worker.h"
#include "ProgressHandler.h"

// Posts a progress update to the application, with progress from 0..65535 and rate in 100 kB/s
static inline void PostProgress(const HWND hApp, const DWORD progress, const DWORD rate) {
	PostMessage(hApp, WM_PROGRESS, progress, rate);
}

// Checks to see if a progress update is needed
#define KB_100 102400ULL
BOOL CProgressHandler::Update(const ULONGLONG sector) {
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
