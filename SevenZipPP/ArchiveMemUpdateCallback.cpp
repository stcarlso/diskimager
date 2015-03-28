// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp

#include "StdAfx.h"
#include "ArchiveMemUpdateCallback.h"
#include "PropVariant.h"
#include "FileSys.h"
#include "InStreamWrapper.h"

namespace SevenZip {
	namespace intl {
		ArchiveMemUpdateCallback::ArchiveMemUpdateCallback(const TString& fileName,
			IStream *inData, const ULONGLONG size) : m_refCount(0), m_fileName(fileName),
			m_data(inData), m_size(size) {}

		ArchiveMemUpdateCallback::~ArchiveMemUpdateCallback() {}

		STDMETHODIMP ArchiveMemUpdateCallback::QueryInterface(REFIID iid, void** ppvObject) {
			if (iid == __uuidof(IUnknown)) {
				*ppvObject = reinterpret_cast<IUnknown*>(this);
				AddRef();
				return S_OK;
			}

			if (iid == IID_IArchiveUpdateCallback) {
				*ppvObject = static_cast<IArchiveUpdateCallback*>(this);
				AddRef();
				return S_OK;
			}

			if (iid == IID_ICryptoGetTextPassword2) {
				*ppvObject = static_cast<ICryptoGetTextPassword2*>(this);
				AddRef();
				return S_OK;
			}

			if (iid == IID_ICompressProgressInfo) {
				*ppvObject = static_cast<ICompressProgressInfo*>(this);
				AddRef();
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) ArchiveMemUpdateCallback::AddRef() {
			return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
		}

		STDMETHODIMP_(ULONG) ArchiveMemUpdateCallback::Release() {
			ULONG res = static_cast<ULONG>(InterlockedDecrement(&m_refCount));
			if (res == 0) {
				delete this;
			}
			return res;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::SetTotal(UInt64 size) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::SetCompleted(const UInt64* completeValue) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::GetUpdateItemInfo(UInt32 index, Int32* newData,
				Int32* newProperties, UInt32* indexInArchive) {
			// Setting info for Create mode (vs. Append mode).
			if (newData != NULL) {
				*newData = 1;
			}

			if (newProperties != NULL) {
				*newProperties = 1;
			}

			if (indexInArchive != NULL) {
				*indexInArchive = static_cast<UInt32>(-1);
			}

			return S_OK;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::GetProperty(UInt32 index, PROPID propID,
				PROPVARIANT* value) {
			CPropVariant prop;
			SYSTEMTIME st;
			FILETIME ft;
			GetSystemTime(&st);
			SystemTimeToFileTime(&st, &ft);

			if (propID == kpidIsAnti) {
				prop = false;
				prop.Detach(value);
				return S_OK;
			}

			if (index >= 1) {
				return E_INVALIDARG;
			}

			switch (propID) {
			case kpidPath:		prop = m_fileName.c_str(); break;
			case kpidIsDir:		prop = false; break;
			case kpidSize:		prop = (UInt64)m_size; break;
			case kpidAttrib:	prop = (UInt32)0; break;
			case kpidCTime:		prop = ft; break;
			case kpidATime:		prop = ft; break;
			case kpidMTime:		prop = ft; break;
			}

			prop.Detach(value);
			return S_OK;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::GetStream(UInt32 index,
				ISequentialInStream** inStream) {
			if (index >= 1) {
				return E_INVALIDARG;
			}

			CComPtr< InStreamWrapper > wrapperStream = new InStreamWrapper(m_data);
			*inStream = wrapperStream.Detach();

			return S_OK;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::SetOperationResult(Int32 operationResult) {
			return S_OK;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::CryptoGetTextPassword2(Int32* passwordIsDefined,
				BSTR* password) {
			// No password for these archives
			*passwordIsDefined = 0;
			*password = SysAllocString(L"");
			return *password != 0 ? S_OK : E_OUTOFMEMORY;
		}

		STDMETHODIMP ArchiveMemUpdateCallback::SetRatioInfo(const UInt64* inSize,
				const UInt64* outSize) {
			return S_OK;
		}
	}
}
