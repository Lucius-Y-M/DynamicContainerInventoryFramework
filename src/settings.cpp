#pragma once


#include "containerManager.h"
#include "settings.h"
#include "utility.h"

namespace {
	struct SwapData {
		bool unableToOpen = false;
		bool hasError = false;
		bool hasBatData = false;
		bool missingName = false;
		bool noChanges = false;
		std::string name;
		std::string changesError;
		std::string conditionsError;
		std::string conditionsBadBypassError;
		std::string conditionsVendorsError;
		std::string conditionsPluginTypeError;
		std::vector<std::string> badStringField;
		std::vector<std::string> missingForm;
		std::vector<std::string> objectNotArray;
		std::vector<std::string> badStringFormat;
	};

	void ReadReport(SwapData a_report) {
		if (!a_report.hasError) return;

		logger::info("{} has errors:", a_report.name);
		if (a_report.hasBatData) {
			logger::info("Swap has bad data.");
			logger::info("");
		}

		if (a_report.missingName) {
			logger::info("Missing friendlyName, or field not a string.");
			logger::info("");
		}

		if (a_report.noChanges) {
			logger::info("    Missing changes field, or field is invalid.");
			logger::info("");
		}

		if (!a_report.badStringField.empty()) {
			logger::info("The following fields are not strings when they should be:");
			for (auto it : a_report.badStringField) {
				logger::info("    >{}", it);
			}
			logger::info("");
		}

		if (!a_report.objectNotArray.empty()) {
			logger::info("The following fields are not arrays when they should be:");
			for (auto it : a_report.objectNotArray) {
				logger::info("    >{}", it);
			}
			logger::info("");
		}

		if (!a_report.badStringFormat.empty()) {
			logger::info("The following fields are not formatted as they should be:");
			for (auto it : a_report.badStringFormat) {
				logger::info("    >{}", it);
			}
			logger::info("");
		}

		if (!a_report.changesError.empty()) {
			logger::info("There are no changes specified for this rule.");
			logger::info("");
		}

		if (!a_report.conditionsError.empty()) {
			logger::info("The condition field is unreadable, but present.");
			logger::info("");
		}

		if (!a_report.conditionsBadBypassError.empty()) {
			logger::info("The unsafe container bypass field is unreadable, but present.");
			logger::info("");
		}

		if (!a_report.conditionsVendorsError.empty()) {
			logger::info("The allow vendors field is unreadable, but present.");
			logger::info("");
		}

		if (!a_report.conditionsPluginTypeError.empty()) {
			logger::info("The plugins field is unreadable, but present.");
			logger::info("");
		}

		if (!a_report.missingForm.empty()) {
			logger::info("The following fields specify a form in a plugin that is loaded, but the form doesn't exist:");
			for (auto it : a_report.missingForm) {
				logger::info("    >{}", it);
			}
			logger::info("");
		}

		if (a_report.unableToOpen) {
			logger::info("Unable to open the config. Make sure the JSON is valid (no missing commas, brackets are closed...)");
			logger::info("");
		}
	}
}




#include "../dist/jsoncpp.cpp"


namespace Settings {
	void ReadConfig(Json::Value a_config, std::string a_reportName, SwapData* a_report) {
		if (!a_config.isObject()) return;
		auto& rules = a_config["rules"];
		if (!rules.isArray()) return;

		for (auto& data : rules) {
			if (!data.isObject()) {
				a_report->hasBatData = true;
				a_report->hasError = true;
				continue;
			}

			auto& friendlyName = data["friendlyName"]; //Friendly Name
			auto& conditions = data["conditions"];     //Conditions
			auto& changes = data["changes"];           //Changes

			bool conditionsAreValid = true;            //If conditions are present but invalid, this stops the rule from registering.
			std::string ruleName = friendlyName.asString(); ruleName += " >> ";  ruleName += a_reportName; //Rule name. Used for logging.

			//Initializing condition results so that they may be used in changes.
			bool bypassUnsafeContainers = false;   
			bool distributeToVendors = false;
			std::vector<std::string> validLocationKeywords{};
			std::vector<RE::BGSLocation*> validLocationIdentifiers{};
			std::vector<RE::TESWorldSpace*> validWorldspaceIdentifiers{};
			std::vector<RE::TESObjectCONT*> validContainers{};
			std::vector<RE::FormID> validReferences{};

			//Missing name check. Missing name is used in the back end for rule checking.
			if (!friendlyName || !friendlyName.isString()) {
				a_report->hasBatData = true;
				a_report->hasError = true;
				a_report->missingName = true;
				continue;
			}
			auto friendlyNameString = friendlyName.asString();

			//Missing changes check. If there are no changes, there is no need to check conditions.
			if (!changes || !changes.isArray()) {
				a_report->changesError = friendlyNameString;
				a_report->hasError = true;
				a_report->noChanges = true;
				continue;
			}

			if (conditions) {
				if (!conditions.isObject()) {
					a_report->conditionsError = friendlyNameString;
					a_report->hasError = true;
					conditionsAreValid = false;
					continue;
				}

				//Plugins check.
				//Will not read the rest of the config if at least one plugin is not present.
				auto& plugins = conditions["plugins"];
				if (plugins) {
					if (!plugins.isArray()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& plugin : plugins) {
						if (!plugin.isString()) {
							a_report->conditionsPluginTypeError = friendlyNameString;
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						if (!Utility::IsModPresent(plugin.asString())) {
							conditionsAreValid = false;
						}
					}
				} //End of plugins check

				//Bypass Check.
				auto& bypassField = conditions["bypassUnsafeContainers"];
				if (bypassField) {
					if (!bypassField.isBool()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						a_report->conditionsBadBypassError = true;
						conditionsAreValid = false;
						continue;
					}
					bypassUnsafeContainers = bypassField.asBool();
				}

				//Vendors Check.
				auto& vendorsField = conditions["allowVendors"];
				if (vendorsField) {
					if (!vendorsField.isBool()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						a_report->conditionsBadBypassError = true;
						conditionsAreValid = false;
						continue;
					}
					distributeToVendors = bypassField.asBool();
				}

				//Container check.
				auto& containerField = conditions["containers"];
				if (containerField) {
					if (!containerField.isArray()) {
						std::string name = friendlyNameString; name += " -> add";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : containerField) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> containers";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto containerID = Utility::ParseFormID(identifier.asString());
						if (containerID == 0) {
							continue;
						}

						auto* container = RE::TESForm::LookupByID<RE::TESObjectCONT>(containerID);
						if (!container) {
							std::string name = friendlyNameString; name += " -> containers -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validContainers.push_back(container);
					}
				}

				//Location check.
				auto& locations = conditions["locations"];
				if (locations) {
					if (!locations.isArray()) {
						std::string name = friendlyNameString; name += " -> locations";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}
					for (auto& identifier : locations) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> locations";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto locationID = Utility::ParseFormID(identifier.asString());
						if (locationID == 0) {
							continue;
						}

						auto* location = RE::TESForm::LookupByID<RE::BGSLocation>(locationID);
						if (!location) {
							std::string name = friendlyNameString; name += " -> locations -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validLocationIdentifiers.push_back(location);
					}
				}

				//Location check.
				//If a location is null, it will not error.
				auto& worldspaces = conditions["worldspaces"];
				if (worldspaces) {
					if (!worldspaces.isArray()) {
						std::string name = friendlyNameString; name += " -> worldspaces";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : worldspaces) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> worldspaces";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						auto worldspaceID = Utility::ParseFormID(identifier.asString());
						if (worldspaceID == 0) {
							continue;
						}

						auto* worldspace = RE::TESForm::LookupByID<RE::TESWorldSpace>(worldspaceID);
						if (!worldspace) {
							std::string name = friendlyNameString; name += " -> locations -> "; name += identifier.asString();
							a_report->missingForm.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validWorldspaceIdentifiers.push_back(worldspace);
					}
				}

				//Location keywords.
				//No verification, for KID reasons.
				auto& locationKeywords = conditions["locationKeywords"];
				if (locationKeywords) {
					if (!locationKeywords.isArray()) {
						std::string name = friendlyNameString; name += " -> locationKeywords";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						conditionsAreValid = false;
						continue;
					}

					for (auto& identifier : locationKeywords) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> locationKeywords";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}
						validLocationKeywords.push_back(identifier.asString());
					}
				}

				//Reference check.
				auto& referencesField = conditions["references"];
				if (referencesField && referencesField.isArray()) {
					for (auto& identifier : referencesField) {
						if (!identifier.isString()) {
							std::string name = friendlyNameString; name += " -> references";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						auto components = clib_util::string::split(identifier.asString(), "|"sv);
						if (components.size() != 2) {
							std::string name = friendlyNameString; name += " -> references -> "; name += identifier.asString();
							a_report->badStringFormat.push_back(name);
							a_report->hasError = true;
							conditionsAreValid = false;
							continue;
						}

						if (!Utility::IsModPresent(components.at(1))) continue;
						validReferences.push_back(Utility::ParseFormID(identifier.asString()));
					}
				}
			} //End of conditions check
			if (!conditionsAreValid) continue;

			//Changes tracking here.
			for (auto& change : changes) {
				ContainerManager::SwapRule newRule;
				newRule.bypassSafeEdits = bypassUnsafeContainers;
				newRule.allowVendors = distributeToVendors;
				newRule.validLocations = validLocationIdentifiers;
				newRule.validWorldspaces = validWorldspaceIdentifiers;
				newRule.locationKeywords = validLocationKeywords;
				newRule.container = validContainers;
				newRule.references = validReferences;
				newRule.ruleName = ruleName;
				bool changesAreValid = true;

				auto& oldId = change["remove"];
				auto& newId = change["add"];
				auto& countField = change["count"];
				auto& removeKeywords = change["removeByKeywords"];
				if (!(oldId || newId || removeKeywords)) {
					a_report->hasBatData = true;
					a_report->changesError = friendlyNameString;
					changesAreValid = false;
					continue;
				}

				if (oldId) {
					if (!oldId.isString()) {
						std::string name = friendlyNameString; name += " -> remove";
						a_report->badStringField.push_back(name);
						a_report->hasError = true;
						changesAreValid = false;
						continue;
					}
					
					auto parsedFormID = Utility::ParseFormID(oldId.asString());
					if (parsedFormID == 0) {
						continue;
					}

					auto* parsedForm = RE::TESForm::LookupByID<RE::TESBoundObject>(parsedFormID);
					if (!parsedForm) continue;
					newRule.oldForm = parsedForm;
				} //Old object check.

				if (newId) {
					if (!newId.isArray()) {
						std::string name = friendlyNameString; name += " -> add";
						a_report->objectNotArray.push_back(name);
						a_report->hasError = true;
						changesAreValid = false;
						continue;
					}

					for (auto& addition : newId) {
						if (!addition.isString()) {
							std::string name = friendlyNameString; name += " -> add";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							changesAreValid = false;
							continue;
						}

						auto parsedFormID = Utility::ParseFormID(addition.asString());
						if (parsedFormID == 0) {
							continue;
						}

						auto* parsedForm = RE::TESForm::LookupByID<RE::TESBoundObject>(parsedFormID);
						if (!parsedForm) continue;
						newRule.newForm.push_back(parsedForm);
					}
				} //New ID Check

				if (removeKeywords) {
					if (!removeKeywords.isArray()) {
						a_report->conditionsPluginTypeError = friendlyNameString;
						a_report->hasError = true;
						conditionsAreValid = false;
						changesAreValid = false;
						continue;
					}

					std::vector<std::string> validRemoveKeywords{};
					for (auto& removeKeyword : removeKeywords) {
						if (!removeKeyword.isString()) {
							std::string name = friendlyNameString; name += " -> removeByKeywords";
							a_report->badStringField.push_back(name);
							a_report->hasError = true;
							changesAreValid = false;
							continue;
						}
						validRemoveKeywords.push_back(removeKeyword.asString());
					}
					newRule.removeKeywords = validRemoveKeywords;
				}
				if (!changesAreValid) continue;

				if (countField && countField.isInt()) {
					newRule.count = countField.asInt();
				}
				else {
					newRule.count = -1;
				}

				ContainerManager::ContainerManager::GetSingleton()->CreateSwapRule(newRule);
			} //End of changes check
		} //End of data loop
	} //End of read config




	bool ReadSettings() {
		std::vector<std::string> configFiles = std::vector<std::string>();
		try {
			configFiles = clib_util::distribution::get_configs(configPath, ""sv, ".json"sv);
		}
		catch (std::exception e) { //Directory not existing, for example?
			SKSE::log::warn("WARNING - caught exception {} while attempting to fetch configs.", e.what());
			return false;
		}
		if (configFiles.empty()) return true;

		std::vector<SwapData> reports;
		for (const auto& config : configFiles) {
			try {
				std::ifstream rawJSON(config);
				Json::Reader  JSONReader;
				Json::Value   JSONFile;
				JSONReader.parse(rawJSON, JSONFile);

				SwapData  report;
				std::string configName = config.substr(config.rfind("/") + 1, config.length() - 1);
				report.name = configName;
				ReadConfig(JSONFile, configName, &report);
				reports.push_back(report);
			}
			catch (std::exception e) { //Unlikely to be thrown, unless there are some weird characters involved.
				SKSE::log::warn("WARNING - caught exception {} while reading a file.", e.what());
			}
		}

		for (auto& report : reports) {
			ReadReport(report);
		}
		return true;
	}
}