#pragma once

#include "ObjIdl.h"

// Size of the disk stream internal buffer
#define DS_BUFFER_SZ ((size_t)(1024 * 1024))

// An IStream interface that allows seven-zip to "Read" information as it comes in from disk
// and "Write" it to this application
class CDiskStream : public IStream {
protected:
	// Whether the read handle should be destroyed first (if false, DestroyPipe will destroy
	// write handle first)
	BOOL m_readFirst;
	// Read handle to the named pipe (used only in this application!)
	HANDLE m_pipeRead;
	// Write handle to the named pipe
	HANDLE m_pipeWrite;
	// COM reference counter
	ULONG m_refCount;

public:
	// Default constructor
	CDiskStream(BOOL destroyReadFirst = TRUE);
	virtual ~CDiskStream();

	// Create the named pipe, TRUE if successful
	BOOL CreatePipe();
	// Destroy the pipe
	void DestroyPipe();
	// Writer calls this when there is no more to give
	void EndOfFile();

	// COM methods
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		_COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// ISequentialStream methods

// Read data when available from the application
	virtual HRESULT STDMETHODCALLTYPE Read(_Out_writes_bytes_to_(cb, *pcbRead) void *pv,
		_In_ ULONG cb, _Out_opt_ ULONG *pcbRead);
	// Write data to the application and wait until it is consumed
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void *pv,
		_In_ ULONG cb, _Out_opt_ ULONG *pcbWritten);

	// IStream methods (not used?)
	virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
		_Out_opt_ ULARGE_INTEGER *plibNewPosition);
	virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	virtual HRESULT STDMETHODCALLTYPE CopyTo(_In_  IStream *pstm, ULARGE_INTEGER cb,
		_Out_opt_ ULARGE_INTEGER *pcbRead, _Out_opt_ ULARGE_INTEGER *pcbWritten);
	virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	virtual HRESULT STDMETHODCALLTYPE Revert(void);
	virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
		DWORD dwLockType);
	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
		DWORD dwLockType);
	virtual HRESULT STDMETHODCALLTYPE Stat(__RPC__out STATSTG *pstatstg, DWORD grfStatFlag);
	virtual HRESULT STDMETHODCALLTYPE Clone(__RPC__deref_out_opt IStream **ppstm);
};

// Compresses a 7z file in the background
UINT CompressWorker(LPVOID param);
// Decompresses a 7z file in the background
UINT DecompressWorker(LPVOID param);
