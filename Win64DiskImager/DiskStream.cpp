#include "stdafx.h"
#include "DiskStream.h"

#define PIPE_NAME _T("\\\\.\\pipe\\win64diskimager")

// Constructor must specify R first or W first
CDiskStream::CDiskStream(BOOL destroyRead) : m_refCount(0UL), m_readFirst(destroyRead),
	m_pipeRead(INVALID_HANDLE_VALUE), m_pipeWrite(INVALID_HANDLE_VALUE) {}

CDiskStream::~CDiskStream() { }

// Create the pipe
BOOL CDiskStream::CreatePipe() {
	m_pipeWrite = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE, 1,
		DS_BUFFER_SZ, 0, 0, NULL);
	m_pipeRead = CreateFile(PIPE_NAME, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	return m_pipeRead != INVALID_HANDLE_VALUE && m_pipeWrite != INVALID_HANDLE_VALUE;
}

// Destroy the pipe
void CDiskStream::DestroyPipe() {
	HANDLE pipeWrite = m_pipeWrite, pipeRead = m_pipeRead;
	if (m_readFirst && pipeRead != INVALID_HANDLE_VALUE) {
		// Close it up, and make sure we avoid double close
		CloseHandle(pipeRead);
		pipeRead = INVALID_HANDLE_VALUE;
		m_pipeRead = INVALID_HANDLE_VALUE;
	}
	if (pipeWrite != INVALID_HANDLE_VALUE) {
		// Close write pipe
		CloseHandle(pipeWrite);
		m_pipeWrite = INVALID_HANDLE_VALUE;
	}
	if (pipeRead != INVALID_HANDLE_VALUE) {
		// Close it up for real
		CloseHandle(pipeRead);
		m_pipeRead = INVALID_HANDLE_VALUE;
	}
}

// Close the write handle which will send an e-o-f to the reader
void CDiskStream::EndOfFile() {
	DestroyPipe();
}

ULONG STDMETHODCALLTYPE CDiskStream::AddRef(void) {
	return (ULONG)InterlockedIncrement(&m_refCount);
}

HRESULT STDMETHODCALLTYPE CDiskStream::QueryInterface(REFIID iid, void __RPC_FAR *__RPC_FAR *ppvObject) {
	if (iid == __uuidof(IUnknown)) {
		*ppvObject = reinterpret_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}
	if (iid == IID_IStream) {
		*ppvObject = static_cast<IStream*>(this);
		AddRef();
		return S_OK;
	}
	if (iid == IID_ISequentialStream) {
		*ppvObject = static_cast<ISequentialStream*>(this);
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CDiskStream::Release(void) {
	ULONG res = (ULONG)InterlockedDecrement(&m_refCount);
	if (res == 0UL)
		delete this;
	return res;
}

// Read data when available from the application
HRESULT STDMETHODCALLTYPE CDiskStream::Read(void *pv, ULONG cb, ULONG *pcbRead) {
	DWORD read, want = (DWORD)cb, have = 0;
	BYTE *ptr = (BYTE *)pv;
	// This is fairly simple, read until EOF reached (0 bytes read), or error
	while (have < want && ReadFile(m_pipeRead, ptr + have, want - have, &read, NULL) && read > 0)
		have += read;
	*pcbRead = (ULONG)have;
	return (have < want) ? S_FALSE : S_OK;
}

// Write data to the application and wait until it is consumed
HRESULT STDMETHODCALLTYPE CDiskStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten) {
	DWORD written, want = (DWORD)cb, have = 0;
	BYTE *ptr = (BYTE *)pv;
	// This is fairly simple, write until all bytes sent or error occurs
	while (have < want && WriteFile(m_pipeWrite, ptr + have, want - have, &written, NULL) &&
			written > 0)
		have += written;
	*pcbWritten = (ULONG)have;
	return (have < want) ? STG_E_WRITEFAULT : S_OK;
}

// Implement?
HRESULT STDMETHODCALLTYPE CDiskStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
		ULARGE_INTEGER *plibNewPosition) {
	return E_NOTIMPL;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::SetSize(ULARGE_INTEGER libNewSize) {
	return E_NOTIMPL;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) {
	return E_NOTIMPL;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::Commit(DWORD grfCommitFlags) {
	return E_NOTIMPL;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::Revert(void) {
	return E_NOTIMPL;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
		DWORD dwLockType) {
	return E_NOTIMPL;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
		DWORD dwLockType) {
	return E_NOTIMPL;
}

// Default stats (just a file)
HRESULT STDMETHODCALLTYPE CDiskStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag) {
	pstatstg->cbSize.QuadPart = 0ULL;
	return S_OK;
}

// Not used
HRESULT STDMETHODCALLTYPE CDiskStream::Clone(IStream **ppstm) {
	return E_NOTIMPL;
}
