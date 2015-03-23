/*
 * Win64DiskImagerGUI.cpp - A more modern and slimmer disk image utility that also allows
 * on the fly compression of images to save space
 */

#include "stdafx.h"
#include "Win64DiskImager.h"
#include "Win64DiskImagerGUI.h"
#include "Worker.h"
#include "FileSrc.h"
#include "RawDiskSrc.h"
#include "disk.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Load that icon!
CWin64DiskImagerGUI::CWin64DiskImagerGUI(CWnd* pParent) : CDialog(IDD_MAIN, pParent),
	m_processing(FALSE), m_waiting(FALSE), m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME)) { }

CWin64DiskImagerGUI::~CWin64DiskImagerGUI() { }

// All important method to avoid debug assertion fails
void CWin64DiskImagerGUI::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyClass)
	DDX_Control(pDX, IDC_COMPRESSION, m_cDoCompress);
	DDX_Control(pDX, IDC_DISKIMG, m_cImageSelect);
	DDX_Control(pDX, IDC_DISKSELECT, m_cDiskSelect);
	DDX_Control(pDX, IDC_DOMD5, m_cDoMD5);
	DDX_Control(pDX, IDC_MD5OUT, m_cMD5Sum);
	DDX_Control(pDX, IDC_PROGRESS, m_cProgress);
	DDX_Control(pDX, IDC_READ, m_cRead);
	DDX_Control(pDX, IDC_VERIFY, m_cVerify);
	DDX_Control(pDX, IDC_WRITE, m_cWrite);
	//}}AFX_DATA_MAP
}

// AFX/MFC message map
BEGIN_MESSAGE_MAP(CWin64DiskImagerGUI, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_READ, &CWin64DiskImagerGUI::OnRead)
	ON_BN_CLICKED(IDC_WRITE, &CWin64DiskImagerGUI::OnWrite)
	ON_BN_CLICKED(IDC_VERIFY, &CWin64DiskImagerGUI::OnVerify)
	ON_BN_CLICKED(IDC_COMPRESSION, &CWin64DiskImagerGUI::UpdateCompress)
	ON_CBN_DROPDOWN(IDC_DISKSELECT, &CWin64DiskImagerGUI::UpdateDisks)
	ON_MESSAGE(WM_DROPFILES, &CWin64DiskImagerGUI::OnDropFiles)
	ON_MESSAGE(WM_END, &CWin64DiskImagerGUI::OnEnd)
	ON_MESSAGE(WM_UPDATEDISKS, &CWin64DiskImagerGUI::OnUpdateDisks)
	ON_MESSAGE(WM_FILE_ERROR, &CWin64DiskImagerGUI::OnErrorFile)
	ON_MESSAGE(WM_PROGRESS, &CWin64DiskImagerGUI::OnProgressReport)
	ON_MESSAGE(WM_MD5_READY, &CWin64DiskImagerGUI::OnMD5Ready)
	ON_MESSAGE(WM_VER_MISMATCH, &CWin64DiskImagerGUI::OnVerifyMismatch)
	ON_MESSAGE(WM_VER_OK, &CWin64DiskImagerGUI::OnVerifyPass)
END_MESSAGE_MAP()

// Changes the progress bar in the window and title bar
void CWin64DiskImagerGUI::AdjustProgress(DWORD progress) {
	const HWND wnd = GetSafeHwnd();
	m_cProgress.SetPos(progress);
	if (m_taskBar != NULL) {
		// Zero progress will cause progress to be hidden
		if (progress > 0 && progress < 65536) {
			m_taskBar->SetProgressState(wnd, TBPF_NORMAL);
			m_taskBar->SetProgressValue(wnd, (ULONGLONG)progress, 65535ULL);
		} else
			m_taskBar->SetProgressState(wnd, TBPF_NOPROGRESS);
	}
}

// Starts a task in the background, passing it a struct of the important GUI parameters
BOOL CWin64DiskImagerGUI::BeginBackground(AFX_THREADPROC process, UINT action) {
	CActionParams *params;
	CString inFile;
	LPTSTR canonFile;
	BOOL start = FALSE;
	const CString diskName = GetSelectedDisk();
	// Pull file from the dialog and canonicalize path
	m_cImageSelect.GetWindowText(inFile);
	if (diskName.GetLength() < 1 || inFile.GetLength() < 2)
		// No disk or file
		Error(GetResourceString(IDS_ERRORNOSEL));
	else if (PathAllocCanonicalize(inFile, 0, &canonFile) == S_OK) {
		params = new CActionParams(action, this);
		// Look up the path
		const DWORD attrib = GetFileAttributes(canonFile);
		if (diskName.GetAt(0) == canonFile[0])
			// Cannot be on the source disk
			Error(GetResourceString(IDS_ERRORONDISK));
		else if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
			// Directories can never be used
			Error(GetResourceString(IDS_ERRORDIR));
		else if (OpenStreams(params, canonFile, diskName, attrib)) {
			// This opens the file so we can throw out canonFile afterwards
			m_waiting = FALSE;
			m_cMD5Sum.Clear();
			// Reset progress to zero and clear old events
			PostMessage(WM_PROGRESS, 0, 0);
			AfxBeginThread(process, params);
			// We got it!
			start = TRUE;
		} else
			// Do not leak it!
			delete params;
		LocalFree(canonFile);
	}
	return start;
}

// Cancel the operation if running (dangerous!)
BOOL CWin64DiskImagerGUI::Cancel() {
	BOOL canClose = TRUE;
	if (m_processing) {
		if (!m_waiting && Confirm(GetResourceString(IDS_CANCELWARN))) {
			// Stop processing, controls are re-enabled (and can be closed for reals)
			m_waiting = TRUE;
			SetStatus(GetResourceString(IDS_CANCELLED));
		}
		// Stop the window close
		canClose = FALSE;
	}
	return canClose;
}

// Format the message into a string
void CWin64DiskImagerGUI::DisplayLastError(UINT messageTemplate, DWORD error,
		const CString &subOne) {
	LPTSTR errorMessage = NULL;
	CString message;
	// Retrieve the message
	::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL,
		error, 0, (LPTSTR) &errorMessage, 0, NULL);
	message.Format(messageTemplate, subOne, errorMessage);
	// Display stop message
	SetStatus(GetResourceString(IDS_ERRORTITLE));
	Error(message);
	LocalFree(errorMessage);
}

// Gets the selected disk letter, or an empty string if none is selected
const CString CWin64DiskImagerGUI::GetSelectedDisk() const {
	const int index = m_cDiskSelect.GetCurSel();
	CString disk;
	// Sanity sanity sanity...
	if (index >= 0 && index < m_drives.GetCount())
		disk.SetString(m_drives.GetAt(index));
	return disk;
}

// Bubble up only if the user is OK with cancelling what is running
void CWin64DiskImagerGUI::OnClose() {
	if (Cancel())
		CDialog::OnClose();
}

// Fired when files are dragged and dropped onto the text box
LRESULT CWin64DiskImagerGUI::OnDropFiles(WPARAM wParam, LPARAM lParam) {
	HDROP drop = reinterpret_cast<HDROP>(wParam);
	LPTSTR data;
	// May only drop one file
	if (SUCCEEDED(drop) && DragQueryFile(drop, 0xFFFFFFFFU, NULL, 0) == 1) {
		// Find out how much space is needed
		const UINT len = DragQueryFile(drop, 0, NULL, 0) + 1U;
		if (len > 1U && (data = (LPTSTR)CoTaskMemAlloc(sizeof(TCHAR) * (size_t)len)) != NULL) {
			// Load data
			if (DragQueryFile(drop, 0, data, len) != 0U) {
				// Need to check file for existence and not directory
				const DWORD attribs = GetFileAttributes(data);
				if (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY))
					m_cImageSelect.SetWindowText(data);
			}
			// Free it
			CoTaskMemFree(data);
		}
	}
	// Make sure to bubble it up
	return DefWindowProc(WM_DROPFILES, wParam, lParam);
}

// Performs GUI tasks which occur when procedures end or are cancelled
LRESULT CWin64DiskImagerGUI::OnEnd(WPARAM wParam, LPARAM lParam) {
	// Ignore the parameters
	(void)wParam;
	(void)lParam;
	// If we are waiting to quit, do so
	m_processing = FALSE;
	m_waiting = FALSE;
	// Clear the progress bar
	AdjustProgress(0);
	// Enable UI
	ToggleControls(TRUE);
	ResetButtonText();
	return (LRESULT)0;
}

// Displays the file error popup
LRESULT CWin64DiskImagerGUI::OnErrorFile(WPARAM wParam, LPARAM lParam) {
	CString filePath;
	DisplayLastError(IDS_ERRORFILE, (DWORD)wParam, _T("disk"));
	return (LRESULT)0;
}

// Checks for sufficient space
BOOL CWin64DiskImagerGUI::HasSpaceFor(CDataSrc *dst, CDataSrc *src, LPCTSTR dest,
		const UINT messageID) {
	ULONGLONG avail;
	if (dst != NULL)
		// Destination size is used
		avail = dst->Size();
	else {
		// This will surely be enough
		const DWORD len = lstrlen(dest) + 1;
		LPTSTR volume = (LPTSTR)CoTaskMemAlloc(sizeof(TCHAR) * (size_t)len);
		ULARGE_INTEGER ds;
		// Extract disk where mounted, then GetDiskFreeSpaceEx it
		if (GetVolumePathName(dest, volume, len) && GetDiskFreeSpaceEx(volume, &ds, NULL, NULL))
			avail = ds.QuadPart;
		CoTaskMemFree(volume);
	}
	// Is there room?
	if (avail < src->Size()) {
		CString message;
		message.Format(messageID, dest);
		Error(message);
		return FALSE;
	}
	return TRUE;
}

// Initialize the application
BOOL CWin64DiskImagerGUI::OnInitDialog() {
	// Initialize dialog
	CDialog::OnInitDialog();
	// Set the large and small icon for this dialog.
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	// Set up progress
	m_cProgress.SetRange32(0, 65536);
	// Set up taskbar progress
	m_taskBar.CoCreateInstance(CLSID_TaskbarList);
	if (m_taskBar != NULL && !SUCCEEDED(m_taskBar->HrInit()))
		m_taskBar.Release();
	PostMessage(WM_PROGRESS, 0, 0);
	// Preload read/write/verify controls
	ResetButtonText();
	// Initialize file browser
	UpdateCompress();
	PostMessage(WM_UPDATEDISKS);
	return TRUE;
}

// Puts text in the MD5 checksum box!
LRESULT CWin64DiskImagerGUI::OnMD5Ready(WPARAM wParam, LPARAM lParam) {
	LPCTSTR md5 = (LPCTSTR)lParam;
	(void)wParam;
	// The argument is freed by caller since SendMessage should be used
	if (md5 != NULL)
		m_cMD5Sum.SetWindowText(md5);
	return (LRESULT)0;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CWin64DiskImagerGUI::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // device context for painting
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	} else
		CDialog::OnPaint();
}

// Updates the progress bars and status bar to match the reported information
LRESULT CWin64DiskImagerGUI::OnProgressReport(WPARAM wParam, LPARAM lParam) {
	// In 100 kB/s (so X.X MB/s)
	const DWORD rate = (DWORD)lParam, rateMB = rate / 10, rateKB1 = rate % 10, progress =
		(DWORD)wParam;
	CString speed;
	if (progress >= 0 && progress <= 65536) {
		// Show them if it is done
		if (progress < 65536) {
			if (progress > 0)
				speed.Format(_T("[%d%%] "), (progress * 25 + 8192) / 16384);
			else
				speed.SetString(_T(""));
			speed.AppendFormat(_T("%d.%01d MB/s"), rateMB, rateKB1);
		} else
			// Completed!
			speed.LoadString(IDS_COMPLETE);
		AdjustProgress(progress);
		SetStatus(speed);
	}
	return (LRESULT)0;
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWin64DiskImagerGUI::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

// Read disk to file
void CWin64DiskImagerGUI::OnRead() {
	if (!m_processing) {
		// Start it up
		if (BeginBackground(ReadWorker, ACTION_READ)) {
			m_processing = TRUE;
			ToggleControls(FALSE);
			// Change Read to Cancel
			m_cRead.SetWindowText(GetResourceString(IDS_BTN_ACTIVE));
			m_cRead.EnableWindow(true);
		}
	} else
		// Cancel!
		Cancel();
}

// Update the disks on an event
LRESULT CWin64DiskImagerGUI::OnUpdateDisks(WPARAM wParam, LPARAM lParam) {
	UpdateDisks();
	return (LRESULT)0;
}

// Write file to disk
void CWin64DiskImagerGUI::OnWrite() {
	if (!m_processing) {
		// Start it up
		if (BeginBackground(WriteWorker, ACTION_WRITE)) {
			m_processing = TRUE;
			ToggleControls(FALSE);
			// Change Write to Cancel
			m_cWrite.SetWindowText(GetResourceString(IDS_BTN_ACTIVE));
			m_cWrite.EnableWindow(true);
		}
	} else
		// Cancel!
		Cancel();
}

// Read disk and verify against image
void CWin64DiskImagerGUI::OnVerify() {
	if (!m_processing) {
		// Start it up
		if (BeginBackground(VerifyWorker, ACTION_VERIFY)) {
			m_processing = TRUE;
			ToggleControls(FALSE);
			// Change Verify to Cancel
			m_cVerify.SetWindowText(GetResourceString(IDS_BTN_ACTIVE));
			m_cVerify.EnableWindow(true);
		}
	} else
		// Cancel!
		Cancel();
}

// Triggers when verify is OK, where wParam:lParam is the # of bytes processed
#define MB_1 1048576ULL
LRESULT CWin64DiskImagerGUI::OnVerifyPass(WPARAM wParam, LPARAM lParam) {
	CString message;
	ULARGE_INTEGER value;
	// Pull both parameters into value
	value.HighPart = (DWORD)wParam;
	value.LowPart = (DWORD)lParam;
	message.Format(IDS_VERIFYOK, (value.QuadPart + (MB_1 >> 1)) / MB_1);
	Notify(message);
	return (LRESULT)0;
}

// Triggers when verify detects a mismatch
LRESULT CWin64DiskImagerGUI::OnVerifyMismatch(WPARAM wParam, LPARAM lParam) {
	const CString message = GetResourceString(IDS_VERIFYFAIL);
	Error(message);
	SetStatus(message);
	return (LRESULT)0;
}

// Opens a disk stream
BOOL CWin64DiskImagerGUI::OpenDisk(CActionParams *params, const CString &disk,
		const DWORD access) {
	HANDLE hDisk, hRaw;
	// First, lock and unmount the volume
	if (SUCCEEDED(hDisk = CRawDiskSrc::LockAndUnmount(disk, access))) {
		if (SUCCEEDED(hRaw = CRawDiskSrc::Open(disk, access)))
			params->m_dest = new CRawDiskSrc(hRaw, hDisk);
		else {
			// Disk error when opening
			CloseHandle(hDisk);
			DisplayLastError(IDS_ERRORDISK, GetLastError(), disk);
		}
	} else
		// Disk error when locking/unmounting
		DisplayLastError(IDS_ERRORDISK, GetLastError(), disk);
	return params->m_dest != NULL;
}

// Opens a file
BOOL CWin64DiskImagerGUI::OpenFile(CActionParams *params, LPCTSTR file,
		const DWORD access) {
	CString message;
	HANDLE hFile;
	if (SUCCEEDED(hFile = CFileSrc::Open(file, access)))
		// Opened file
		params->m_file = new CFileSrc(hFile);
	else {
		// Display error message
		const UINT code = (access & GENERIC_WRITE) != 0 ? IDS_ERROROUTPUT : IDS_FILENOTFOUND;
		message.Format(code, file);
		Error(message);
	}
	return params->m_file != NULL;
}

// Opens the data file for I/O
BOOL CWin64DiskImagerGUI::OpenStreams(CActionParams *params, LPCTSTR inFile,
		const CString &disk, const DWORD attrib) {
	CString message;
	// Check the file, READ from disk = WRITE to file
	switch (params->m_action) {
	case ACTION_READ:
		// Prompt for overwrite if the file exists
		message.Format(IDS_OVERWRITE, inFile);
		if (attrib == INVALID_FILE_ATTRIBUTES || Confirm(message)) {
			// Attempt to open the output file for writing
			if (OpenFile(params, inFile, GENERIC_WRITE) &&
					OpenDisk(params, disk, GENERIC_READ) &&
					!HasSpaceFor(NULL, params->m_dest, inFile, IDS_ERRORSPACE))
				params->Release();
		}
		break;
	case ACTION_WRITE:
		// This could be dangerous.
		message.Format(IDS_WRITEWARN, disk);
		if (Confirm(message) && OpenFile(params, inFile, GENERIC_READ) &&
				OpenDisk(params, disk, GENERIC_WRITE) &&
				!HasSpaceFor(params->m_dest, params->m_file, disk, IDS_ERRORSPACE))
			params->Release();
		break;
	case ACTION_VERIFY:
		// Should be safe, neither disk nor file is written
		if (OpenFile(params, inFile, GENERIC_READ) && OpenDisk(params, disk, GENERIC_READ) &&
				!HasSpaceFor(params->m_dest, params->m_file, disk, IDS_VERIFYSIZE))
			params->Release();
		break;
	default:
		// Bug!
		ASSERT(FALSE);
		break;
	}
	// If we got everything open
	return params->m_file != NULL && params->m_dest != NULL;
}

// Resets the 3 buttons to their default text
void CWin64DiskImagerGUI::ResetButtonText() {
	// Read
	m_cRead.SetWindowText(GetResourceString(IDS_READ_INACTIVE));
	// Verify
	m_cVerify.SetWindowText(GetResourceString(IDS_VERIFY_INACTIVE));
	// Write
	m_cWrite.SetWindowText(GetResourceString(IDS_WRITE_INACTIVE));
}

// Toggles all the controls
void CWin64DiskImagerGUI::ToggleControls(BOOL enable) {
	m_cDiskSelect.EnableWindow(enable);
	m_cDoMD5.EnableWindow(enable);
	m_cDoCompress.EnableWindow(enable);
	m_cImageSelect.EnableWindow(enable);
	m_cRead.EnableWindow(enable);
	m_cVerify.EnableWindow(enable);
	m_cWrite.EnableWindow(enable);
}

// Updates the file chooser to allow 7z (compress) or img (uncompress)
void CWin64DiskImagerGUI::UpdateCompress() {
	CString filter, caption;
	if (m_cDoCompress.GetCheck() == BST_CHECKED) {
		// 7-Zip files
		filter.LoadString(IDS_EXT_ZIP);
		caption.LoadString(IDS_CAPTION_ZIP);
	} else {
		// Disk images
		filter.LoadString(IDS_EXT_IMG);
		caption.LoadString(IDS_CAPTION_IMG);
	}
	m_cImageSelect.EnableFileBrowseButton(filter, caption);
}

// Update the disk list in the combo box (do not even scan C:, too many deaths!)
void CWin64DiskImagerGUI::UpdateDisks() {
	DWORD avail = GetLogicalDrives(), temp;
	m_cDiskSelect.ResetContent();
	m_drives.RemoveAll();
	// Add available disks here
	for (int i = 0; i < 26; i++) {
		CString letter;
		if (avail & 1) {
			letter.Format(_T("%c:\\"), (TCHAR)(_T('A') + i));
			// Go through available letters
			if (checkDriveType(letter, temp) && i != 2) {
				m_cDiskSelect.AddString(letter);
				m_drives.Add(letter);
			}
		}
		avail >>= 1;
	}
	// Select the first item if possible
	if (m_cDiskSelect.GetCount() > 0)
		m_cDiskSelect.SetCurSel(0);
}
