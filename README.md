# diskimager

A direct disk copy and backup utility for Windows. This program was built with Visual Studio
2013 Community Edition.

## Purpose

Win32DiskImager (http://sourceforge.net/projects/win32diskimager/) uses QT, which adds a bunch
of DLL dependencies and makes it look like it was written in the 1990s. This is a rewrite using
MFC to add features such as taskbar progress, and 7-Zip compression/decompression on the fly.

## Compiling

Open Win64DiskImager.sln in Visual Studio 2013. Compiling should be fairly straightforward.
