#pragma once

#include "SevenZipLibrary.h"
#include "CompressionFormat.h"
#include "7zip/Archive/IArchive.h"

namespace SevenZip {
	class SevenZipExtractor {
	private:

		const SevenZipLibrary& m_library;
		TString m_archivePath;
		CompressionFormatEnum m_format;

	public:

		SevenZipExtractor(const SevenZipLibrary& library, const TString& archivePath);
		virtual ~SevenZipExtractor();

		void SetCompressionFormat(const CompressionFormatEnum& format);

		virtual void ExtractArchive(const TString& directory);

		// Extract just one file from the archive to the output stream, index supplied
		virtual void ExtractSingleFile(IInArchive *archive, IStream *output, const UINT index);

		// Find the biggest file index and size, return archive, or NULL if failed
		IInArchive * SevenZipExtractor::FindLargestFile(UINT &index, ULONGLONG &size);
	private:

		void ExtractArchive(const CComPtr< IStream >& archiveStream, const TString& directory);
	};
}
