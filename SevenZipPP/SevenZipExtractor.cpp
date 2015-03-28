#include "stdafx.h"
#include "SevenZipExtractor.h"
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveOpenCallback.h"
#include "ArchiveExtractCallback.h"
#include "ArchiveMemExtractCallback.h"
#include "InStreamWrapper.h"
#include "PropVariant.h"

namespace SevenZip {

	using namespace intl;


	CComPtr< IInArchive > GetArchiveReader(const SevenZipLibrary& library, const CompressionFormatEnum& format) {
		const GUID* guid = NULL;

		switch (format) {
		case CompressionFormat::Zip:
			guid = &CLSID_CFormatZip;
			break;

		case CompressionFormat::GZip:
			guid = &CLSID_CFormatGZip;
			break;

		case CompressionFormat::BZip2:
			guid = &CLSID_CFormatBZip2;
			break;

		case CompressionFormat::Rar:
			guid = &CLSID_CFormatRar;
			break;

		case CompressionFormat::Tar:
			guid = &CLSID_CFormatTar;
			break;

		case CompressionFormat::Iso:
			guid = &CLSID_CFormatIso;
			break;

		case CompressionFormat::Cab:
			guid = &CLSID_CFormatCab;
			break;

		case CompressionFormat::Lzma:
			guid = &CLSID_CFormatLzma;
			break;

		case CompressionFormat::Lzma86:
			guid = &CLSID_CFormatLzma86;
			break;

		default:
			guid = &CLSID_CFormat7z;
			break;
		}

		CComPtr< IInArchive > archive;
		library.CreateObject(*guid, IID_IInArchive, reinterpret_cast<void**>(&archive));
		return archive;
	}


	SevenZipExtractor::SevenZipExtractor(const SevenZipLibrary& library, const TString& archivePath)
		: m_library(library)
		, m_archivePath(archivePath)
		, m_format(CompressionFormat::SevenZip) {}

	SevenZipExtractor::~SevenZipExtractor() {}

	void SevenZipExtractor::SetCompressionFormat(const CompressionFormatEnum& format) {
		m_format = format;
	}

	void SevenZipExtractor::ExtractArchive(const TString& destDirectory) {
		CComPtr< IStream > fileStream = FileSys::OpenFileToRead(m_archivePath);
		if (fileStream == NULL) {
			throw SevenZipException(StrFmt(_T("Could not open archive \"%s\""),
				m_archivePath.c_str()));
		}

		ExtractArchive(fileStream, destDirectory);
	}

	void SevenZipExtractor::ExtractArchive(const CComPtr< IStream >& archiveStream,
			const TString& destDirectory) {
		CComPtr< IInArchive > archive = GetArchiveReader(m_library, m_format);
		CComPtr< InStreamWrapper > inFile = new InStreamWrapper(archiveStream);
		CComPtr< ArchiveOpenCallback > openCallback = new ArchiveOpenCallback();

		HRESULT hr = archive->Open(inFile, 0, openCallback);
		if (hr != S_OK) {
			throw SevenZipException(GetCOMErrMsg(_T("Open archive"), hr));
		}

		CComPtr< ArchiveExtractCallback > extractCallback = new ArchiveExtractCallback(archive,
			destDirectory);

		hr = archive->Extract(NULL, -1, false, extractCallback);
		if (hr != S_OK) // returning S_FALSE also indicates error
		{
			throw SevenZipException(GetCOMErrMsg(_T("Extract archive"), hr));
		}
	}

	static UINT _FindLargestFile(IInArchive *archive, UInt32 numFiles, ULONGLONG &maxSize) {
		CPropVariant propDir, propSize;
		ULONGLONG size, max = 0ULL;
		UInt32 maxIndex = 0;
		for (UInt32 i = 0; i < numFiles; i++)
			// Find largest non-dir file
			if (SUCCEEDED(archive->GetProperty(i, kpidIsDir, &propDir)) &&
					SUCCEEDED(archive->GetProperty(i, kpidSize, &propSize)) &&
					propDir.vt == VT_BOOL && propDir.boolVal == VARIANT_FALSE) {
				// Size property could have a variety of representations
				switch (propSize.vt) {
				case VT_UI1:
					size = (ULONGLONG)propSize.bVal;
					break;
				case VT_UI2:
					size = (ULONGLONG)propSize.uiVal;
					break;
				case VT_UI4:
					size = (ULONGLONG)propSize.ulVal;
					break;
				case VT_UI8:
					size = (ULONGLONG)propSize.uhVal.QuadPart;
					break;
				default:
					size = 0ULL;
				}
				// Do we have a winner?
				if (size > max) {
					maxIndex = i;
					max = size;
				}
			}
		maxSize = max;
		return (UINT)maxIndex;
	}

	IInArchive * SevenZipExtractor::FindLargestFile(UINT &index, ULONGLONG &size) {
		// Read archive from file system
		CComPtr< IInArchive > archive = GetArchiveReader(m_library, m_format);
		CComPtr< IStream > fileStream = FileSys::OpenFileToRead(m_archivePath);
		if (fileStream != NULL) {
			// Open archive
			CComPtr< InStreamWrapper > inFile = new InStreamWrapper(fileStream);
			CComPtr< ArchiveOpenCallback > openCallback = new ArchiveOpenCallback();
			if (archive->Open(inFile, 0, openCallback) == S_OK) {
				// Search archive for largest file
				ULONGLONG maxSize = 0ULL;
				UInt32 maxIndex = 0, numFiles;
				if (archive->GetNumberOfItems(&numFiles) == S_OK) {
					// Good to go!
					index = _FindLargestFile(archive, numFiles, size);
					return archive.Detach();
				}
			}
		}
		return NULL;
	}

	void SevenZipExtractor::ExtractSingleFile(IInArchive *archive, IStream *output,
			const UINT index) {
		UInt32 indices = (UInt32)index;
		CComPtr< ArchiveMemExtractCallback > extractCallback = new ArchiveMemExtractCallback(
			archive, output, indices);
		// Extract just this file
		HRESULT hr = archive->Extract(&indices, 1, false, extractCallback);
		if (hr != S_OK) // returning S_FALSE also indicates error
		{
			throw SevenZipException(GetCOMErrMsg(_T("Extract archive"), hr));
		}
		archive->Release();
	}
}
