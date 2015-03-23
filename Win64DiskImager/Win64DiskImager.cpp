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
