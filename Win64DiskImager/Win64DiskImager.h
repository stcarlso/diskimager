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

#include "resource.h"

// Message send to GUI to trigger completion (or cancel!)
#define WM_END (WM_USER + 1)
// Updates the disks in the combo box
#define WM_UPDATEDISKS (WM_USER + 2)
// Displays the file error message using wParam as an error code
#define WM_FILE_ERROR (WM_USER + 3)
// Reports the progress of the operation (wParam from 0 to 65535) and the rate (lParam in 100kB/s)
#define WM_PROGRESS (WM_USER + 4)
// MD5 post report
#define WM_MD5_READY (WM_USER + 5)
// Verify failure message, content mismatch
#define WM_VER_MISMATCH (WM_USER + 6)
// Verify sucess message
#define WM_VER_OK (WM_USER + 7)

// Application class
class CWin64DiskImagerApp : public CWinApp {
public:
	CWin64DiskImagerApp();

	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

// Holds the only instance (MFC style!?)
extern CWin64DiskImagerApp theApp;

// Gets a string resource, failing with an assertion if not found
static inline CString GetResourceString(const UINT resource) {
	CString res;
	if (!res.LoadString(resource)) ASSERT(FALSE);
	return res;
}
