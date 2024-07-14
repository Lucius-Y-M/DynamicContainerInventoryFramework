#pragma once

#include "PCH.h"


namespace Settings {
#define configPath R"(Data/SKSE/Plugins/ContainerDistributionFramework/)"
	enum ChangeType {
		ADD,
		REMOVE,
		REPLACE
	};

	bool ReadSettings();
}