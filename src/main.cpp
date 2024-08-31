#include <spdlog/sinks/basic_file_sink.h>
#include "containerManager.h"
#include "hooks.h"
#include "iniReader.h"
#include "merchantFactionCache.h"
#include "settings.h"

void InitializeLog([[maybe_unused]] spdlog::level::level_enum a_level = spdlog::level::info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= std::format("{}.log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	const auto level = a_level;

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
    switch (a_message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        MerchantCache::MerchantCache::GetSingleton()->BuildCache();
        ContainerManager::ContainerManager::GetSingleton()->InitializeData();
        Hooks::Install();
        Settings::ReadSettings();
        INISettings::BuildINI();
        break;
    default:
        break;
    }
}





extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{


    InitializeLog();

    _loggerInfo("Starting up {}.", Plugin::NAME);
    _loggerInfo("Plugin Version: {}.", Plugin::VERSION);

    

    _loggerInfo("Container Distribution Framework, Original by SeaSparrow (Shekhinaga)");
    _loggerInfo("-- You are using its 1.6.640 port by LuciusP24");
    _loggerInfo("-- There is a 1.5.97 port available too, made by Fuzzles");



    const auto ver = a_skse->RuntimeVersion();
    if (ver < SKSE::RUNTIME_SSE_1_6_317) {
        _loggerError("!! CRITICAL:");
        _loggerError("========== Your game's runtime version is too low, and it seems you may be on 1.5.97.");
        _loggerError("========== For 1.5.97 support, please use Fuzzles's 1.5 backport.");

        return false;
    }
    else if (ver > SKSE::RUNTIME_SSE_1_6_678) {

        _loggerError("!! CRITICAL:");
        _loggerError("========== Your game's runtime version is too high, as this backport only supports 1.6.629, 1.6.640, 1.6.659 GOG");
        _loggerError("========== (and possibly 1.6.318, 1.6.353 etc.)");
        _loggerError("========== For 1.6.1130+ support, please use SeaSparrow's original mod directly. You do NOT need this backport version of mine.");

        return false;
    }



    _loggerInfo("-------------------------------------------------------------------------------------");
    SKSE::Init(a_skse);

    

    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);

    return true;
}





extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse();

    v.CompatibleVersions({
        SKSE::RUNTIME_SSE_1_6_317,
        SKSE::RUNTIME_SSE_1_6_318,
        SKSE::RUNTIME_SSE_1_6_323,
        SKSE::RUNTIME_SSE_1_6_342,
        SKSE::RUNTIME_SSE_1_6_353,
        SKSE::RUNTIME_SSE_1_6_629,
        SKSE::RUNTIME_SSE_1_6_640,
        SKSE::RUNTIME_SSE_1_6_659,
        SKSE::RUNTIME_SSE_1_6_678
    });

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* skseInterface, SKSE::PluginInfo* pluginInfo)
{

    auto ver = skseInterface->RuntimeVersion();
    if (ver < SKSE::RUNTIME_SSE_1_6_317) {
        _loggerError("!! CRITICAL:");
        _loggerError("========== Your game's runtime version is too low, and it seems you may be on 1.5.97.");
        _loggerError("========== For 1.5.97 support, please use Fuzzles's 1.5 backport.");

        return false;
    }

    else if (ver > SKSE::RUNTIME_SSE_1_6_678) {

        _loggerError("!! CRITICAL:");
        _loggerError("========== Your game's runtime version is too high, as this backport only supports 1.6.629, 1.6.640, 1.6.659 GOG");
        _loggerError("========== (and possibly 1.6.318, 1.6.353 etc.)");
        _loggerError("========== For 1.6.1130+ support, please use SeaSparrow's original mod directly. You do NOT need this backport version of mine.");

        return false;
    }

	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}



// extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse) {
//     SetupLog();
//     _loggerInfo("Starting up {}.", Version::NAME);
//     _loggerInfo("Plugin Version: {}.", Version::VERSION);

//     _loggerInfo("Container Distribution Framework, Original by SeaSparrow (Shekhinaga)");
//     _loggerInfo("-- You are using its 1.6.640 port by LuciusP24");
//     _loggerInfo("-- There is a 1.5.97 port available too, made by Fuzzles");

//     _loggerInfo("-------------------------------------------------------------------------------------");
//     SKSE::Init(a_skse);

//     auto messaging = SKSE::GetMessagingInterface();
//     messaging->RegisterListener(MessageHandler);
//     return true;
// }