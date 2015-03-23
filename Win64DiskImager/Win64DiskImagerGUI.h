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

#include "Win64DiskImager.h"
#include "ActionParams.h"

class CWin64DiskImagerGUI : public CDialog {
public:
	CWin64DiskImagerGUI(CWnd* pParent = NULL);
	virtual ~CWin64DiskImagerGUI();

	// Used by other classes
	inline BOOL ShouldDoMD5() const {
		return m_cDoMD5.GetCheck() == BST_CHECKED;
	}
	inline volatile BOOL * RefToCancel() {
		return &m_waiting;
	}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	// Control mappings for DoDataExchange to avoid GetDlgItem
	CComboBox m_cDiskSelect;
	CButton m_cDoMD5;
	CButton m_cDoCompress;
	CMFCEditBrowseCtrl m_cImageSelect;
	CEdit m_cMD5Sum;
	CProgressCtrl m_cProgress;
	CButton m_cRead;
	CButton m_cVerify;
	CButton m_cWrite;
	// List of available drives
	CStringArray m_drives;
	// Window icon
	HICON m_hIcon;
	// Processing?
	volatile BOOL m_processing;
	// Taskbar pointer
	CComPtr<ITaskbarList3> m_taskBar;
	// Waiting for cancellation?
	volatile BOOL m_waiting;

	// Displays a confirmation popup with No selected by default
	inline BOOL Confirm(const CString &message) {
		return MessageBox(message, GetResourceString(IDS_WARNINGTITLE), MB_YESNO |
			MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
	}
	// Displays an error message popup on the screen with only OK to choose
	inline void Error(const CString &message) {
		MessageBox(message, GetResourceString(IDS_ERRORTITLE), MB_OK | MB_ICONSTOP);
	}
	// Displays a notification message popup on the screen with only OK to choose
	inline void Notify(const CString &message) {
		MessageBox(message, GetResourceString(IDS_COMPLETE), MB_OK | MB_ICONINFORMATION);
	}
	// Updates the status bar text
	inline void SetStatus(const CString &status) {
		CWnd *statusBar = GetDlgItem(IDC_COPYSTATUS);
		ASSERT(statusBar != NULL);
		statusBar->SetWindowText(status);
	}

	// Helper methods
	void AdjustProgress(DWORD progress);
	BOOL BeginBackground(AFX_THREADPROC process, UINT action);
	BOOL Cancel();
	void DisplayLastError(UINT messageTemplate, DWORD error, const CString &subOne);
	const CString GetSelectedDisk() const;
	BOOL HasSpaceFor(CDataSrc *dst, CDataSrc *src, LPCTSTR dest, const UINT messageID);
	BOOL OpenDisk(CActionParams *params, const CString &disk, const DWORD access);
	BOOL OpenFile(CActionParams *params, LPCTSTR file, const DWORD access);
	BOOL OpenStreams(CActionParams *params, LPCTSTR inFile, const CString &disk,
		const DWORD attrib);
	void ResetButtonText();
	void ToggleControls(BOOL enable);

	// Handlers for messages from the system
	virtual void OnClose();
	afx_msg LRESULT OnDropFiles(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEnd(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnErrorFile(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnMD5Ready(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg LRESULT OnProgressReport(WPARAM wParam, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRead();
	afx_msg LRESULT OnUpdateDisks(WPARAM wParam, LPARAM lParam);
	afx_msg void OnVerify();
	afx_msg LRESULT OnVerifyPass(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnVerifyMismatch(WPARAM wParam, LPARAM lParam);
	afx_msg void OnWrite();
	afx_msg void UpdateCompress();
	afx_msg void UpdateDisks();

	DECLARE_MESSAGE_MAP()
};
