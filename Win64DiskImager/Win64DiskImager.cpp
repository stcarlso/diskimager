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
#include "Win64DiskImagerGUI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CWin64DiskImagerApp, CWinApp)
END_MESSAGE_MAP()

CWin64DiskImagerApp::CWin64DiskImagerApp() { }

CWin64DiskImagerApp theApp;

// CWin64DiskImagerApp initialization
BOOL CWin64DiskImagerApp::InitInstance() {
	INITCOMMONCONTROLSEX initCtrls;
	// COM initialize
	if (CoInitialize(NULL) == S_OK) {
		// Change the registry key under which our settings are stored
		SetRegistryKey(_T("Disk Utility"));
		// Enable visual styles (why do we need a manifest for version 6?)
		initCtrls.dwSize = sizeof(initCtrls);
		initCtrls.dwICC = ICC_WIN95_CLASSES;
		InitCommonControlsEx(&initCtrls);
		// Initialize application
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CWinApp::InitInstance();
		{
			// Display the "dialog"
			CWin64DiskImagerGUI gui;
			m_pMainWnd = &gui;
			gui.DoModal();
		}
	}
	CoUninitialize();
	return FALSE;
}
