#pragma once

#include "SevenZipCompressor.h"
#include "SevenZipExtractor.h"

#if 0
#ifdef _DEBUG
	#ifdef _UNICODE
		#pragma comment ( lib, "7zpp_ud.lib" )
	#else
		#pragma comment ( lib, "7zpp_ad.lib" )
	#endif
#else
	#ifdef _UNICODE
		#pragma comment ( lib, "7zpp_u.lib" )
	#else
		#pragma comment ( lib, "7zpp_a.lib" )
	#endif
#endif
#endif
