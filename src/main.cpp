#pragma once


#include <spdlog/sinks/basic_file_sink.h>
// #include "containerCache.h"




#include "containerManager.h"
#include "hooks.h"
#include "iniReader.h"
#include "merchantFactionCache.h"
#include "settings.h"


#define DLLEXPORT __declspec(dllexport)

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





extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse();
	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* skse, SKSE::PluginInfo* pluginInfo)
{

    if (skse->RuntimeVersion() < SKSE::RUNTIME_SSE_1_6_318) {

        logger::critical("This DLL specifically does not support 1.5.97 (or earlier).");
        logger::critical("If you are on 1.5.97, please download the backport by Fuzzles here: ");

        return false;
    }
    else if (skse->RuntimeVersion() > SKSE::RUNTIME_SSE_1_6_678) {
        logger::critical("This DLL specifically does not support runtimes from 1.6.1130 up.");
        logger::critical("If you are on 1.6.1130+, you should simply use the original by Shekhinaga: ");

        return false;
    }



	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
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




















extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse) {


    InitializeLog(
        /*
            TODO: remove before release
        */
        //    spdlog::level::debug
    );

	_loggerInfo("-- Loaded plugin {} {}", Plugin::NAME, Plugin::VERSION.string());

    _loggerInfo("-- Original by Shekhinaga, 1.6.640 backport by LuciusP24 --");
    _loggerInfo("-- An additional 1.5.97 backport is also available, by Fuzzles --");

    _loggerInfo("-------------------------------------------------------------------------------------");
    SKSE::Init(a_skse);

    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);
    return true;
}