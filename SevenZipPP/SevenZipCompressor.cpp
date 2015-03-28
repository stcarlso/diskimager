#include "stdafx.h"
#include "SevenZipCompressor.h"
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveUpdateCallback.h"
#include "OutStreamWrapper.h"
#include "PropVariant.h"
#include "ArchiveMemUpdateCallback.h"

namespace SevenZip {

	using namespace intl;

	const TString SearchPatternAllFiles = _T("*");

	SevenZipCompressor::SevenZipCompressor(const SevenZipLibrary& library,
		const TString& archivePath) : m_library(library), m_archivePath(archivePath) {}

	SevenZipCompressor::~SevenZipCompressor() {}

	void SevenZipCompressor::SetCompressionLevel(const CompressionLevelEnum& level) {
		m_compressionLevel = level;
	}

	void SevenZipCompressor::CompressDirectory(const TString& directory, bool includeSubdirs) {
		FindAndCompressFiles(directory, SearchPatternAllFiles, FileSys::GetPath(directory),
			includeSubdirs);
	}

	void SevenZipCompressor::CompressFiles(const TString& directory,
			const TString& searchFilter, bool includeSubdirs) {
		FindAndCompressFiles(directory, searchFilter, directory, includeSubdirs);
	}

	void SevenZipCompressor::CompressAllFiles(const TString& directory, bool includeSubdirs) {
		FindAndCompressFiles(
			directory,
			SearchPatternAllFiles,
			directory,
			includeSubdirs);
	}

	void SevenZipCompressor::CompressFile(const TString& filePath) {
		std::vector< FilePathInfo > files = FileSys::GetFile(filePath);

		if (files.empty()) {
			throw SevenZipException(StrFmt(_T("File \"%s\" does not exist"), filePath.c_str()));
		}

		CompressFilesToArchive(TString(), files);
	}

	// Compress data from a stream object as a single file with the specified name
	void SevenZipCompressor::CompressStream(const TString& name, IStream *stream,
			const ULONGLONG size) {
		CComPtr< IOutArchive > archiver;
		m_library.CreateObject(CLSID_CFormat7z, IID_IOutArchive,
			reinterpret_cast<void**>(&archiver));

		SetCompressionProperties(archiver);

		CComPtr< OutStreamWrapper > outFile = new OutStreamWrapper(OpenArchiveStream());
		CComPtr< ArchiveMemUpdateCallback > callback = new ArchiveMemUpdateCallback(name,
			stream, size);

		HRESULT hr = archiver->UpdateItems(outFile, 1U, callback);
		if (hr != S_OK) // returning S_FALSE also indicates error
		{
			throw SevenZipException(GetCOMErrMsg(_T("Create archive from stream"), hr));
		}
	}

	CComPtr< IStream > SevenZipCompressor::OpenArchiveStream() {
		CComPtr< IStream > fileStream = FileSys::OpenFileToWrite(m_archivePath);
		if (fileStream == NULL) {
			throw SevenZipException(StrFmt(_T("Could not create archive \"%s\""),
				m_archivePath.c_str()));
		}
		return fileStream;
	}

	void SevenZipCompressor::FindAndCompressFiles(const TString& directory,
			const TString& searchPattern, const TString& pathPrefix, bool recursion) {
		if (!FileSys::DirectoryExists(directory)) {
			throw SevenZipException(StrFmt(_T("Directory \"%s\" does not exist"),
				directory.c_str()));
		}

		if (FileSys::IsDirectoryEmptyRecursive(directory)) {
			throw SevenZipException(StrFmt(_T("Directory \"%s\" is empty"),
				directory.c_str()));
		}

		std::vector< FilePathInfo > files = FileSys::GetFilesInDirectory(directory,
			searchPattern, recursion);
		CompressFilesToArchive(pathPrefix, files);
	}

	void SevenZipCompressor::CompressFilesToArchive(const TString& pathPrefix,
			const std::vector< FilePathInfo >& filePaths) {
		CComPtr< IOutArchive > archiver;
		m_library.CreateObject(CLSID_CFormat7z, IID_IOutArchive,
			reinterpret_cast<void**>(&archiver));

		SetCompressionProperties(archiver);

		CComPtr< OutStreamWrapper > outFile = new OutStreamWrapper(OpenArchiveStream());
		CComPtr< ArchiveUpdateCallback > callback = new ArchiveUpdateCallback(pathPrefix,
			filePaths);

		HRESULT hr = archiver->UpdateItems(outFile, (UInt32)filePaths.size(), callback);
		if (hr != S_OK) // returning S_FALSE also indicates error
		{
			throw SevenZipException(GetCOMErrMsg(_T("Create archive"), hr));
		}
	}

	void SevenZipCompressor::SetCompressionProperties(IUnknown* outArchive) {
		const size_t numProps = 1;
		const wchar_t* names[numProps] = { L"x" };
		CPropVariant values[numProps] = { static_cast<UInt32>(m_compressionLevel.GetValue()) };

		CComPtr< ISetProperties > setter;
		outArchive->QueryInterface(IID_ISetProperties, reinterpret_cast<void**>(&setter));
		if (setter == NULL) {
			throw SevenZipException(_T("Archive does not support setting compression properties"));
		}

		HRESULT hr = setter->SetProperties(names, values, numProps);
		if (hr != S_OK) {
			throw SevenZipException(GetCOMErrMsg(_T("Setting compression properties"), hr));
		}
	}

}
