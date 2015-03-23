#pragma once

// The number of sectors to write at a time
#define CHUNK_SIZE 1024ULL
// The number of milliseconds to wait between each rate update
#define UPDATE_RATE 1000ULL

// Reads from disk to file
UINT ReadWorker(LPVOID param);
// Verifies disk and file
UINT VerifyWorker(LPVOID param);
// Writes file to disk
UINT WriteWorker(LPVOID param);
