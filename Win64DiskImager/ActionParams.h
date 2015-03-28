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

#include "FileSrc.h"
#include "RawDiskSrc.h"
#include "7zpp.h"

class CWin64DiskImagerGUI;

// Actions to take
#define ACTION_READ 0U
#define ACTION_WRITE 1U
#define ACTION_VERIFY 2U

// Action information!
class CActionParams {
public:
	// 7-zip library reference
	SevenZip::SevenZipLibrary * const m_7z;
	// Action to perform: ACTION_READ, ACTION_WRITE, or ACTION_VERIFY
	const UINT m_action;
	// Handle to the application for message posting
	const HWND m_app;
	// This will be TRUE if cancel requested
	volatile BOOL *m_cancel;
	// Do compression?
	const BOOL m_compress;
	// Destination for data
	CRawDiskSrc *m_dest;
	// File data
	CDataSrc *m_file;
	// Do MD5?
	const BOOL m_md5;

	// Create an action parameters structure and initialize the volume to target
	CActionParams::CActionParams(const UINT act, CWin64DiskImagerGUI *gui);
	CActionParams::CActionParams(const CActionParams &other);
	// We own these now, so we must trash them on quit
	virtual ~CActionParams();

	// Trashes them before we quit
	void Release();
};
