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
