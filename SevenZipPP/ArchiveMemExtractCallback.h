// This file is based on the following file from the LZMA SDK (http://www.7-zip.org/sdk.html):
//   ./CPP/7zip/UI/Client7z/Client7z.cpp
#pragma once

#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>

namespace SevenZip {
	namespace intl {
		class ArchiveMemExtractCallback : public IArchiveExtractCallback,
			public ICryptoGetTextPassword {
		private:
			long m_refCount;
			CComPtr< IInArchive > m_archiveHandler;
			IStream *m_output;

			TString m_relPath;
			bool m_isDir;

			bool m_hasNewFileSize;
			UInt64 m_newFileSize;

			UInt32 m_index;
		public:

			ArchiveMemExtractCallback(const CComPtr< IInArchive >& archiveHandler,
				IStream *output, const UInt32 index);
			virtual ~ArchiveMemExtractCallback();

			STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject);
			STDMETHOD_(ULONG, AddRef)();
			STDMETHOD_(ULONG, Release)();

			// IProgress
			STDMETHOD(SetTotal)(UInt64 size);
			STDMETHOD(SetCompleted)(const UInt64* completeValue);

			// IArchiveExtractCallback
			STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode);
			STDMETHOD(PrepareOperation)(Int32 askExtractMode);
			STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

			// ICryptoGetTextPassword
			STDMETHOD(CryptoGetTextPassword)(BSTR* password);

		private:
			HRESULT GetPropertyFilePath(UInt32 index);
			HRESULT GetPropertyIsDir(UInt32 index);
			HRESULT GetPropertySize(UInt32 index);
		};
	}
}
