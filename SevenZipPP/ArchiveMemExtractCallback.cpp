// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#include "StdAfx.h"
#include "ArchiveMemExtractCallback.h"
#include "PropVariant.h"
#include "FileSys.h"
#include "OutStreamWrapper.h"
#include <comdef.h>

namespace SevenZip {
	namespace intl {
		const TString EmptyFileAlias = _T("[Content]");

		ArchiveMemExtractCallback::ArchiveMemExtractCallback(
			const CComPtr< IInArchive >& archiveHandler, IStream *output, const UInt32 index) :
			m_refCount(0), m_archiveHandler(archiveHandler), m_output(output), m_index(index) {}

		ArchiveMemExtractCallback::~ArchiveMemExtractCallback() {}

		STDMETHODIMP ArchiveMemExtractCallback::QueryInterface(REFIID iid, void** ppvObject) {
			if (iid == __uuidof(IUnknown)) {
				*ppvObject = reinterpret_cast<IUnknown*>(this);
				AddRef();
				return S_OK;
			}

			if (iid == IID_IArchiveExtractCallback) {
				*ppvObject = static_cast<IArchiveExtractCallback*>(this);
				AddRef();
				return S_OK;
			}

			if (iid == IID_ICryptoGetTextPassword) {
				*ppvObject = static_cast<ICryptoGetTextPassword*>(this);
				AddRef();
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) ArchiveMemExtractCallback::AddRef() {
			return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
		}

		STDMETHODIMP_(ULONG) ArchiveMemExtractCallback::Release() {
			ULONG res = static_cast<ULONG>(InterlockedDecrement(&m_refCount));
			if (res == 0) {
				delete this;
			}
			return res;
		}

		STDMETHODIMP ArchiveMemExtractCallback::SetTotal(UInt64 size) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemExtractCallback::SetCompleted(const UInt64* completeValue) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemExtractCallback::GetStream(UInt32 index,
				ISequentialOutStream** outStream, Int32 askExtractMode) {
			HRESULT hr = GetPropertyIsDir(index);
			// Retrieve all the various properties for the file at this index
			if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
				// Stop here for files which should be skipped
				return S_OK;
			else if (!SUCCEEDED(hr))
				// Failed to get property
				return hr;
			else if (m_index == index && !m_isDir) {
				// Index matched
				CComPtr< OutStreamWrapper > wrapperStream = new OutStreamWrapper(m_output);
				*outStream = wrapperStream.Detach();
				return S_OK;
			}
			return E_FAIL;
		}

		STDMETHODIMP ArchiveMemExtractCallback::PrepareOperation(Int32 askExtractMode) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemExtractCallback::SetOperationResult(Int32 operationResult) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemExtractCallback::CryptoGetTextPassword(BSTR* password) {
			return E_ABORT;
		}

		HRESULT ArchiveMemExtractCallback::GetPropertyFilePath(UInt32 index) {
			CPropVariant prop;
			HRESULT hr = m_archiveHandler->GetProperty(index, kpidPath, &prop);
			if (!SUCCEEDED(hr))
				return hr;
			if (prop.vt == VT_EMPTY)
				m_relPath = EmptyFileAlias;
			else if (prop.vt != VT_BSTR)
				return E_FAIL;
			else {
				_bstr_t bstr = prop.bstrVal;
#ifdef _UNICODE
				m_relPath = bstr;
#else
				char relPath[MAX_PATH];
				int size = WideCharToMultiByte( CP_UTF8, 0, bstr, bstr.length(), relPath, MAX_PATH, NULL, NULL );
				m_relPath.assign( relPath, size );
#endif
			}
			return S_OK;
		}

		HRESULT ArchiveMemExtractCallback::GetPropertyIsDir(UInt32 index) {
			CPropVariant prop;
			HRESULT hr = m_archiveHandler->GetProperty(index, kpidIsDir, &prop);
			if (!SUCCEEDED(hr))
				return hr;
			if (prop.vt == VT_EMPTY)
				m_isDir = false;
			else if (prop.vt != VT_BOOL)
				return E_FAIL;
			else
				m_isDir = prop.boolVal != VARIANT_FALSE;
			return S_OK;
		}

		HRESULT ArchiveMemExtractCallback::GetPropertySize(UInt32 index) {
			CPropVariant prop;
			HRESULT hr = m_archiveHandler->GetProperty(index, kpidSize, &prop);
			if (!SUCCEEDED(hr))
				return hr;
			switch (prop.vt) {
			case VT_EMPTY:
				m_hasNewFileSize = false;
				return S_OK;
			case VT_UI1:
				m_newFileSize = prop.bVal;
				break;
			case VT_UI2:
				m_newFileSize = prop.uiVal;
				break;
			case VT_UI4:
				m_newFileSize = prop.ulVal;
				break;
			case VT_UI8:
				m_newFileSize = (UInt64)prop.uhVal.QuadPart;
				break;
			default:
				return E_FAIL;
			}
			m_hasNewFileSize = true;
			return S_OK;
		}
	}
}
