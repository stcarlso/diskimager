#include "stdafx.h"
#include "ActionParams.h"
#include "Win64DiskImagerGUI.h"

// Create action parameters based on action
CActionParams::CActionParams(const UINT act, CWin64DiskImagerGUI *gui) : m_action(act),
	m_app(gui->GetSafeHwnd()), m_md5(gui->ShouldDoMD5()), m_dest(NULL), m_file(NULL),
	m_cancel(gui->RefToCancel()) {}

CActionParams::~CActionParams() {
	Release();
}

void CActionParams::Release() {
	if (m_file != NULL) {
		m_file->Close();
		delete m_file;
		m_file = NULL;
	}
	if (m_dest != NULL) {
		m_dest->Close();
		delete m_dest;
		m_dest = NULL;
	}
}
