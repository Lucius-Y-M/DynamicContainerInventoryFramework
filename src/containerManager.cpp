#pragma once


#include "containerCache.h"
#include "containerManager.h"
#include "utility.h"

namespace {
	static void AddLeveledListToContainer(RE::TESLeveledList* list, RE::TESObjectREFR* a_container) {
		RE::BSScrapArray<RE::CALCED_OBJECT> result{};
		Utility::ResolveLeveledList(list, &result);
		if (result.size() < 1) return;

		for (auto& obj : result) {
			auto* thingToAdd = static_cast<RE::TESBoundObject*>(obj.form);
			if (!thingToAdd) continue;
			a_container->AddObjectToContainer(thingToAdd, nullptr, obj.count, nullptr);
		}
	}
}

namespace ContainerManager {
	bool ContainerManager::IsRuleValid(SwapRule* a_rule, RE::TESObjectREFR* a_ref) {
		auto* refLoc = a_ref->GetCurrentLocation();
		auto* refWorldspace = a_ref->GetWorldspace();

		bool hasWorldspaceLocation = a_rule->validWorldspaces.empty() ? true : false;
		if (!hasWorldspaceLocation && refWorldspace) {
			if (std::find(a_rule->validWorldspaces.begin(), a_rule->validWorldspaces.end(), refWorldspace) != a_rule->validWorldspaces.end()) {
				hasWorldspaceLocation = true;
			}
		}

		//Parent Location check. If the current location is NOT a match, this finds its parents.
		bool hasParentLocation = a_rule->validLocations.empty() ? true : false;

		if (!hasParentLocation && refLoc) {
			if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), refLoc) != a_rule->validLocations.end()) {
				hasParentLocation = true;
			}

			//Check parents.
			auto settingsParents = this->parentLocations.find(refLoc) != this->parentLocations.end() ? this->parentLocations[refLoc] : std::vector<RE::BGSLocation*>();
			RE::BGSLocation* parent = refLoc->parentLoc;
			for (auto it = settingsParents.begin(); it != settingsParents.end() && !hasParentLocation && parent; ++it) {
				if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), parent) != a_rule->validLocations.end()) {
					hasParentLocation = true;
				}
				parent = parent->parentLoc;
			}
		}
		else if (!hasParentLocation && !refLoc) {
			if (this->worldspaceMarker.find(refWorldspace) != this->worldspaceMarker.end()) {
				for (auto marker : this->worldspaceMarker[refWorldspace]) {
					if (marker->GetPosition().GetDistance(a_ref->GetPosition()) > this->fMaxLookupRadius) continue;
					auto* markerLoc = marker->GetCurrentLocation();
					if (!markerLoc) continue;
					if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), markerLoc) != a_rule->validLocations.end()) {
						hasParentLocation = true;
						break;
					}

					//Check parents.
					auto settingsParents = this->parentLocations.find(markerLoc) != this->parentLocations.end() ? this->parentLocations[markerLoc] : std::vector<RE::BGSLocation*>();
					RE::BGSLocation* parent = markerLoc->parentLoc;
					for (auto it = settingsParents.begin(); it != settingsParents.end() && !hasParentLocation && parent; ++it) {
						if (std::find(a_rule->validLocations.begin(), a_rule->validLocations.end(), parent) != a_rule->validLocations.end()) {
							hasParentLocation = true;
							break;
						}
						parent = parent->parentLoc;
					}
				}
			}
		}
		

		//Location keyword search. If these do not exist, check the parents..
		bool hasLocationKeywordMatch = a_rule->locationKeywords.empty() ? true : false;
		if (!hasLocationKeywordMatch && refLoc) {
			for (auto& locationKeyword : a_rule->locationKeywords) {
				if (refLoc->HasKeywordString(locationKeyword)) {
					hasLocationKeywordMatch = true;
				}
			}

			//Check parents.
			if (!hasLocationKeywordMatch) {
				auto refParentLocs = this->parentLocations.find(refLoc) != this->parentLocations.end() ?
					this->parentLocations[refLoc] : std::vector<RE::BGSLocation*>();

				for (auto it = refParentLocs.begin(); it != refParentLocs.end() && hasLocationKeywordMatch; ++it) {
					for (auto& locationKeyword : a_rule->locationKeywords) {
						if ((*it)->HasKeywordString(locationKeyword)) {
							hasLocationKeywordMatch = true;
						}
					}
				}
			}
		}

		//Reference check.
		bool hasReferenceMatch = a_rule->references.empty() ? true : false;
		if (!hasReferenceMatch) {
			std::stringstream stream;
			stream << std::hex << a_ref->formID;
			if (std::find(a_rule->references.begin(), a_rule->references.end(), a_ref->formID) != a_rule->references.end()) {
				hasReferenceMatch = true;
			}
		}

		//Container check.
		bool hasContainerMatch = a_rule->container.empty() ? true : false;
		auto* refBaseContainer = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (!hasContainerMatch && refBaseContainer) {
			for (auto& container : a_rule->container) {
				if (refBaseContainer == container) {
					hasContainerMatch = true;
				}
			}
		}
		return (hasReferenceMatch && hasLocationKeywordMatch && hasParentLocation && hasContainerMatch && hasWorldspaceLocation);
	}

	void ContainerManager::CreateSwapRule(SwapRule a_rule) {
		enum type {
			kAdd,
			kRemove,
			kReplace
		};
		type ruleType = kAdd;
		bool removeByKeywirds = false;

		if (a_rule.removeKeywords.empty()) {
			if (!a_rule.oldForm) {
				this->addRules.push_back(a_rule);
			}
			else if (a_rule.newForm.empty()) {
				this->removeRules.push_back(a_rule);
				ruleType = kRemove;
			}
			else {
				this->replaceRules.push_back(a_rule);
				ruleType = kReplace;
			}
		}
		else {
			removeByKeywirds = true;
			if (!a_rule.newForm.empty()) {
				this->replaceRules.push_back(a_rule);
				ruleType = kReplace;
			}
			else {
				this->removeRules.push_back(a_rule);
				ruleType = kRemove;
			}
		}

		logger::info("Registered new rule of type: {}.", ruleType == kAdd ? "Add" : ruleType == kRemove ? "Remove" : "Replace");
		logger::info("    Rule name: {}", a_rule.ruleName);
		if (ruleType == kRemove && removeByKeywirds) {
			logger::info("    Forms with these keywords will be removed:");
			for (auto& keyword : a_rule.removeKeywords) {
				logger::info("        >{}", keyword);
			}
		}
		else if (ruleType == kRemove) {
			logger::info("    This form will be removed: {}", strcmp(a_rule.oldForm->GetName(), "") == 0 ?
				clib_util::editorID::get_editorID(a_rule.oldForm).empty() ? std::to_string(a_rule.oldForm->GetLocalFormID()) :
				clib_util::editorID::get_editorID(a_rule.oldForm)
				: a_rule.oldForm->GetName());
			logger::info("    Count: {}", a_rule.count > 0 ? std::to_string(a_rule.count) : "All");
		}
		else if (ruleType == kReplace && removeByKeywirds) {
			logger::info("    Forms with these keywords will be removed:");
			for (auto& keyword : a_rule.removeKeywords) {
				logger::info("        >{}", keyword);
			}

			logger::info("    And replaced by any of these:");
			for (auto* form : a_rule.newForm) {
				logger::info("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}
		}
		else if (ruleType == kReplace) {
			logger::info("    This form will be removed: {}", strcmp(a_rule.oldForm->GetName(), "") == 0 ?
				clib_util::editorID::get_editorID(a_rule.oldForm).empty() ? std::to_string(a_rule.oldForm->GetLocalFormID()) :
				clib_util::editorID::get_editorID(a_rule.oldForm)
				: a_rule.oldForm->GetName());

			logger::info("    And replaced by any of these:");
			for (auto* form : a_rule.newForm) {
				logger::info("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}
		}
		else {
			logger::info("    Any of the following forms may be added, with a count of {}:", a_rule.count > 1 ? a_rule.count : 1);
			for (auto* form : a_rule.newForm) {
				logger::info("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}
		}

		if (a_rule.bypassSafeEdits) {
			logger::info("");
			logger::info("    This rule can distribute to safe containers.");
		}

		if (a_rule.allowVendors) {
			logger::info("");
			logger::info("    This rule can distribute to vendor containers.");
		}

		if (!a_rule.container.empty()) {
			logger::info("");
			logger::info("    This rule will only apply to these containers:");
			for (auto* it : a_rule.container) {
				logger::info("        >{}", strcmp(it->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(it).empty() ? std::to_string(it->GetLocalFormID()) :
					clib_util::editorID::get_editorID(it)
					: it->GetName());
			}
		}
		if (!a_rule.validLocations.empty()) {
			logger::info("");
			logger::info("    This rule will only apply to these locations:");
			for (auto* it : a_rule.validLocations) {
				logger::info("        >{}", strcmp(it->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(it).empty() ? std::to_string(it->GetLocalFormID()) :
					clib_util::editorID::get_editorID(it)
					: it->GetName());
			}
		}
		if (!a_rule.validWorldspaces.empty()) {
			logger::info("");
			logger::info("    This rule will only apply to these worldspaces:");
			for (auto* it : a_rule.validWorldspaces) {
				logger::info("        >{}", strcmp(it->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(it).empty() ? std::to_string(it->GetLocalFormID()) :
					clib_util::editorID::get_editorID(it)
					: it->GetName());
			}
		}
		if (!a_rule.locationKeywords.empty()) {
			logger::info("");
			logger::info("    This rule will only apply to locations with these keywords:");
			for (auto& it : a_rule.locationKeywords) {
				logger::info("        >{}", it);
			}
		}
		if (!a_rule.references.empty()) {
			logger::info("");
			logger::info("    This rule will only apply to these references:");
			for (auto reference : a_rule.references) {
				logger::info("        >{}", reference);
			}
		}
		logger::info("-------------------------------------------------------------------------------------");
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
		bool merchantContainer = a_ref->GetFactionOwner() ? a_ref->GetFactionOwner()->IsVendor() : false;

		bool isContainerUnsafe = false;
		auto* containerBase = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (containerBase && !(containerBase->data.flags & RE::CONT_DATA::Flag::kRespawn)) {
			isContainerUnsafe = true;
		}

		bool hasParentCell = a_ref->parentCell ? true : false;
		auto* parentEncounterZone = hasParentCell ? a_ref->parentCell->extraList.GetEncounterZone() : nullptr;
		if (parentEncounterZone && parentEncounterZone->data.flags & RE::ENCOUNTER_ZONE_DATA::Flag::kNeverResets) {
			isContainerUnsafe = true;
		}

		auto* containerItems = ContainerCache::CachedData::GetSingleton()->GetContainerItems(containerBase);
		for (auto& rule : this->replaceRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;

				a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				for (; itemCount > 0;) {
					size_t rng = clib_util::RNG().Generate<size_t>(0, rule.newForm.size() - 1);
					RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
					if (thingToAdd->As<RE::TESLeveledList>()) {
						AddLeveledListToContainer(thingToAdd->As<RE::TESLeveledList>(), a_ref);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, 1, nullptr);
					}
					--itemCount;
				}
			}
			else {
				uint32_t itemCount = 0;
				for (auto* item : *containerItems) {
					bool missingKeyword = false;
					for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
						if (item->HasKeywordByEditorID(*it)) continue;
						missingKeyword = true;
					}
					if (missingKeyword) continue;

					auto oldItemCount = a_ref->GetInventoryCounts()[rule.oldForm];
					if (oldItemCount < 1) continue;
					itemCount += oldItemCount;
					a_ref->RemoveItem(item, oldItemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}

				for (uint32_t i = 0; i < itemCount; ++i) {
					size_t rng = clib_util::RNG().Generate<size_t>(0, rule.newForm.size() - 1);
					RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
					auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
					if (leveledThing) {
						AddLeveledListToContainer(leveledThing, a_ref);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
					}
				}
			}
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			int32_t ruleCount = rule.count;

			if (rule.removeKeywords.empty()) {
				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;
				if (!IsRuleValid(&rule, a_ref)) continue;

				if (ruleCount > itemCount || ruleCount < 1) {
					ruleCount = itemCount;
				}
				a_ref->RemoveItem(rule.oldForm, ruleCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			}
			else {
				auto* container = a_ref->GetContainer();
				if (!container) continue;

				for (auto* item : *containerItems) {
					bool missingKeyword = false;
					for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
						if (item->HasKeywordByEditorID(*it)) continue;
						missingKeyword = true;
					}
					if (missingKeyword) continue;

					auto oldItemCount = a_ref->GetInventoryCounts()[item];
					if (oldItemCount < 1) continue;
					a_ref->RemoveItem(item, oldItemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}
			}
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;
			int32_t ruleCount = rule.count;
			if (rule.count < 1) {
				ruleCount = 1;
			}

			for (; ruleCount > 0;) {
				size_t rng = clib_util::RNG().Generate<size_t>(0, rule.newForm.size() - 1);
				RE::TESBoundObject* thingToAdd = rule.newForm.at(rng);
				auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();

				if (leveledThing) {
					AddLeveledListToContainer(leveledThing, a_ref);
				}
				else {
					a_ref->AddObjectToContainer(thingToAdd, nullptr, 1, nullptr);
				}
				--ruleCount;
			}
		} //Add rule reading end.
	}

	void ContainerManager::InitializeData() {
		auto& locationArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSLocation>();
		for (auto* location : locationArray) {
			std::vector<RE::BGSLocation*> parents;
			auto* parentLocation = location->parentLoc;
			if (!parentLocation) continue;

			parents.push_back(parentLocation);
			//logger::info("Location: {} - Parents:", clib_util::editorID::get_editorID(location));
			//logger::info("    >{}", clib_util::editorID::get_editorID(parentLocation));
			Utility::GetParentChain(parentLocation, &parents);
			this->parentLocations[location] = parents;
		}

		auto& worldspaceArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>();
		for (auto* worldspace : worldspaceArray) {
			auto* persistentCell = worldspace->persistentCell;
			if (!persistentCell) continue;
			std::vector<RE::TESObjectREFR*> references{};

			persistentCell->ForEachReference([&](RE::TESObjectREFR* a_marker) {
				auto* markerLoc = a_marker->GetCurrentLocation();
				if (!markerLoc) return RE::BSContainer::ForEachResult::kContinue;
				if (!a_marker->extraList.GetByType(RE::ExtraDataType::kMapMarker)) return RE::BSContainer::ForEachResult::kContinue;
				references.push_back(a_marker);
				return RE::BSContainer::ForEachResult::kContinue;
			});

			this->worldspaceMarker[worldspace] = references;
		}
	}

	void ContainerManager::ContainerManager::SetMaxLookupRange(float a_range) {
		this->fMaxLookupRadius = a_range;
	}
}