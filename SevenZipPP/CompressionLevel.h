#pragma once


#include "Enum.h"


namespace SevenZip {
	struct CompressionLevel {
		enum _Enum {
			None,
			Fast,
			Low = 3,
			Normal = 5,
			High = 7,
			Ultra = 9
		};

		typedef intl::EnumerationDefinitionNoStrings _Definition;
		typedef intl::EnumerationValue< _Enum, _Definition, Normal > _Value;
	};

	typedef CompressionLevel::_Value CompressionLevelEnum;
}
