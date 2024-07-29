#pragma once

// #include "AliasSKSE.h"


namespace Settings {
#define configPath R"(Data/SKSE/Plugins/ContainerDistributionFramework/)"
	enum ChangeType {
		ADD,
		REMOVE,
		REPLACE
	};

	bool ReadSettings();
}