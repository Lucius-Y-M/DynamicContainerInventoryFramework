#pragma once

#include "containerManager.h"
#include "merchantFactionCache.h"
#include "utility.h"

namespace {
	static void AddLeveledListToContainer(RE::TESLeveledList* list, RE::TESObjectREFR* a_container, uint32_t a_count) {
		RE::BSScrapArray<RE::CALCED_OBJECT> result{};
		Utility::ResolveLeveledList(list, &result, a_count);
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
		//Reference check.
		bool hasReferenceMatch = a_rule->references.empty() ? true : false;
		if (!hasReferenceMatch) {
			std::stringstream stream;
			stream << std::hex << a_ref->formID;
			if (std::find(a_rule->references.begin(), a_rule->references.end(), a_ref->formID) != a_rule->references.end()) {
				hasReferenceMatch = true;
			}
		}
		if (!hasReferenceMatch) return false;

		//Container check.
		bool hasContainerMatch = a_rule->container.empty() ? true : false;
		auto* refBaseContainer = a_ref->GetBaseObject()->As<RE::TESObjectCONT>();
		if (!hasContainerMatch && refBaseContainer) {
			for (auto it = a_rule->container.begin(); it != a_rule->container.end() && !hasContainerMatch; ++it) {
				if (refBaseContainer == *it) hasContainerMatch = true;
			}
		}
		if (!hasContainerMatch) return false;

		bool hasAVMatch = a_rule->requiredAVs.empty() ? true : false;
		if (!hasAVMatch) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			bool hasFullMatch = false;

			for (auto it = a_rule->requiredAVs.begin(); it != a_rule->requiredAVs.end() && !hasAVMatch; ++it) {
				auto& valueSet = *it;
				if (player->AsActorValueOwner()->GetActorValue(valueSet.first) >= valueSet.second.first) {
					hasFullMatch = true;
				}
				if (hasFullMatch && valueSet.second.second > -1.0f) {
					if (player->AsActorValueOwner()->GetActorValue(valueSet.first) > valueSet.second.second) {
						hasFullMatch = false;
					}
				}
				hasAVMatch = hasFullMatch;
			}
		}
		if (!hasAVMatch) return false;





		/* added: quest check  */

		bool hasQuestStageMatch = a_rule->requiredQuestStages.empty() ? true : false;
		if (!hasQuestStageMatch) {
			for (auto & it : a_rule->requiredQuestStages) {

				auto & [wantedStage, comparator] = it.second;				

				if (Comparison::CompareResult(it.first->currentStage, comparator, wantedStage)) {					

					hasQuestStageMatch = true;
					break;
				}
			}			
		}
		if (hasQuestStageMatch == false) return false;

		/* end of add */















		//We can get lazy here, since this is the last condition.
		bool hasGlobalMatch = a_rule->requiredGlobalValues.empty() ? true : false;
		if (!hasGlobalMatch) {
			for (auto it = a_rule->requiredGlobalValues.begin(); it != a_rule->requiredGlobalValues.end() && !hasGlobalMatch; ++it) {
				if ((*it).first->value == (*it).second) hasGlobalMatch = true;
			}
		}
		if (!hasGlobalMatch) return false;

		auto* refLoc = a_ref->GetCurrentLocation();
		auto* refWorldspace = a_ref->GetWorldspace();

		bool hasWorldspaceLocation = a_rule->validWorldspaces.empty() ? true : false;
		if (!hasWorldspaceLocation && refWorldspace) {
			if (std::find(a_rule->validWorldspaces.begin(), a_rule->validWorldspaces.end(), refWorldspace) != a_rule->validWorldspaces.end()) {
				hasWorldspaceLocation = true;
			}
		}

		//The following are SLOW because they have a lot to check.
		//Thus, they are moved to the end of the validation to give the first a chance to fail early.
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
		if (!hasWorldspaceLocation) return false;

		//Location keyword search. If these do not exist, check the parents..
		bool hasLocationKeywordMatch = a_rule->locationKeywords.empty() ? true : false;
		if (!hasLocationKeywordMatch && refLoc) {
			for (auto& locationKeyword : a_rule->locationKeywords) {
				if (refLoc->HasKeywordString(locationKeyword)) {
					hasLocationKeywordMatch = true;


					/* added: small optimization: since one valid = condition valid, no need to check rest */

					break;

					/* end of add */
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

							/* added: small optimization: since one valid = condition valid, no need to check rest */

							break;

							/* end of add */
						}
					}
				}
			}
		}
		if (!hasLocationKeywordMatch) return false;

		return true;
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

			/* added & changed: depending on "pickAtRandom" setting, different message is displayed */

			if (a_rule.isPickAtRandom) {
				logger::info("    One of the following forms will be added, with a count of {}:", a_rule.count > 1 ? a_rule.count : 1);
			}
			else {
				logger::info("    All of the following forms will be added, with a count of {}:", a_rule.count > 1 ? a_rule.count : 1);
			}


			for (auto* form : a_rule.newForm) {
				logger::info("        >{}", strcmp(form->GetName(), "") == 0 ?
					clib_util::editorID::get_editorID(form).empty() ? std::to_string(form->GetLocalFormID()) :
					clib_util::editorID::get_editorID(form)
					: form->GetName());
			}


			/* end of add & change */

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



		/* added: quest stages */

		if (!a_rule.requiredQuestStages.empty()) {
			logger::info("");
			logger::info("    This rule will only apply if at least one of these quest conditions is fulfilled:");
			for (auto pair : a_rule.requiredQuestStages) {

				auto & [wantedStage, comparator] = pair.second;

				logger::info("        >Quest: {} ; Its Current Stage Must: {} {}",
					pair.first->GetFormEditorID(),
					Comparison::OperatorToString(pair.second.second),
					wantedStage
				);
			}

		}

		/* end of add */




		if (!a_rule.requiredGlobalValues.empty()) {
			logger::info("");
			logger::info("    This rule will only apply if these globals have these values:");
			for (auto pair : a_rule.requiredGlobalValues) {
				logger::info("        >Global: {}, Value: {}", pair.first->GetFormEditorID(), pair.second);
			}
		}
		if (!a_rule.requiredAVs.empty()) {
			logger::info("");
			logger::info("    This rule will only apply while these actor values are all valid");
			for (auto& pair : a_rule.requiredAVs) {
				std::string valueName = "";
				switch (pair.first) {
				case RE::ActorValue::kIllusion:
					valueName = "Illusion";
					break;
				case RE::ActorValue::kConjuration:
					valueName = "Conjuration";
					break;
				case RE::ActorValue::kDestruction:
					valueName = "Destruction";
					break;
				case RE::ActorValue::kRestoration:
					valueName = "Restoration";
					break;
				case RE::ActorValue::kAlteration:
					valueName = "Ateration";
					break;
				case RE::ActorValue::kEnchanting:
					valueName = "Enchanting";
					break;
				case RE::ActorValue::kSmithing:
					valueName = "Smithing";
					break;
				case RE::ActorValue::kHeavyArmor:
					valueName = "Heavy Armor";
					break;
				case RE::ActorValue::kBlock:
					valueName = "Block";
					break;
				case RE::ActorValue::kTwoHanded:
					valueName = "Two Handed";
					break;
				case RE::ActorValue::kOneHanded:
					valueName = "One Handed";
					break;
				case RE::ActorValue::kArchery:
					valueName = "Archery";
					break;
				case RE::ActorValue::kLightArmor:
					valueName = "Light Armor";
					break;
				case RE::ActorValue::kSneak:
					valueName = "Sneak";
					break;
				case RE::ActorValue::kLockpicking:
					valueName = "Lockpicking";
					break;
				case RE::ActorValue::kPickpocket:
					valueName = "Pickpocket";
					break;
				case RE::ActorValue::kSpeech:
					valueName = "Speech";
					break;
				default:
					valueName = "Alchemy";
					break;
				}

				logger::info("        >Value: {} -> Min Level: {}, Max Level: {}", valueName, 
					pair.second.first, 
					pair.second.second > -1.0f ? std::to_string(pair.second.second) : "MAX");
			}
		}
		logger::info("-------------------------------------------------------------------------------------");
	}

	void ContainerManager::HandleContainer(RE::TESObjectREFR* a_ref) {
#ifdef DEBUG
		auto then = std::chrono::high_resolution_clock::now();
		size_t rulesApplied = 0;
#endif 
		bool merchantContainer = a_ref->GetFactionOwner() ? a_ref->GetFactionOwner()->IsVendor() : false;
		if (!merchantContainer) {
			merchantContainer = MerchantCache::MerchantCache::GetSingleton()->IsMerchantContainer(a_ref);
		}

		auto* containerBase = a_ref->GetBaseObject() ? a_ref->GetBaseObject()->As<RE::TESObjectCONT>() : nullptr;
		bool isContainerUnsafe = false;
		if (containerBase && !(containerBase->data.flags & RE::CONT_DATA::Flag::kRespawn)) {
			isContainerUnsafe = true;
		}

		bool hasParentCell = a_ref->parentCell ? true : false;
		auto* parentEncounterZone = hasParentCell ? a_ref->parentCell->extraList.GetEncounterZone() : nullptr;
		if (parentEncounterZone && parentEncounterZone->data.flags & RE::ENCOUNTER_ZONE_DATA::Flag::kNeverResets) {
			isContainerUnsafe = true;
		}
		
		auto inventoryCounts = a_ref->GetInventoryCounts();
		for (auto& rule : this->replaceRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!merchantContainer && rule.onlyVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;

			if (rule.removeKeywords.empty()) {

				auto itemCount = a_ref->GetInventoryCounts()[rule.oldForm];
				if (itemCount < 1) continue;

				a_ref->RemoveItem(rule.oldForm, itemCount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);



				/* added & changed:
				
					if pickAtRandom = 
						> true  ,  distribute only one thing 
						> false ,  distribute everything (default behavior)
				*/

				if (rule.isPickAtRandom) {

					const int index = clib_util::RNG().Generate(0, (int) rule.newForm.size() - 1);

					logger::info(">> start = 0, end = {}; end result = {}", rule.newForm.size() - 1, index);

					if (index >= 0 && index < rule.newForm.size())
					{
						
						auto obj = rule.newForm[index];

						auto leveledThing = obj->As<RE::TESLeveledList>();
						if (leveledThing) {
#ifdef DEBUG
							logger::debug("Adding leveled list to container:");
#endif
							AddLeveledListToContainer(leveledThing, a_ref, rule.count);
						}
						else {
							a_ref->AddObjectToContainer(obj, nullptr, rule.count, nullptr);
						}
					}
				}
				else {
					for (auto* obj : rule.newForm) {
						auto leveledThing = obj->As<RE::TESLeveledList>();
						if (leveledThing) {
#ifdef DEBUG
							logger::debug("Adding leveled list to container:");
#endif
							AddLeveledListToContainer(leveledThing, a_ref, rule.count);
						}
						else {
							a_ref->AddObjectToContainer(obj, nullptr, rule.count, nullptr);
						}
					}

				}

				/* end of add & change */


#ifdef DEBUG

				rulesApplied++;
#endif
			}
			else {
				uint32_t itemCount = 0;
				for (auto& pair : inventoryCounts) {
					auto* item = pair.first;
					bool missingKeyword = false;
					for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
						if (item->HasKeywordByEditorID(*it)) continue;
						missingKeyword = true;
					}
					if (missingKeyword) continue;

					itemCount += pair.second;
					a_ref->RemoveItem(item, pair.second, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}

				for (auto* thingToAdd : rule.newForm) {
					auto* leveledThing = thingToAdd->As<RE::TESLeveledList>();
					if (leveledThing) {
#ifdef DEBUG
						logger::debug("Adding leveled list to container:");
#endif
						AddLeveledListToContainer(leveledThing, a_ref, itemCount);
					}
					else {
						a_ref->AddObjectToContainer(thingToAdd, nullptr, itemCount, nullptr);
					}
				}
#ifdef DEBUG
				rulesApplied++;
#endif
			}
		} //Replace Rule reading end.

		for (auto& rule : this->removeRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!merchantContainer && rule.onlyVendors) continue;
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
#ifdef DEBUG
				rulesApplied++;
#endif
			}
			else {
				for (auto& pair : inventoryCounts) {
					auto* item = pair.first;
					bool missingKeyword = false;
					for (auto it = rule.removeKeywords.begin(); it != rule.removeKeywords.end() && !missingKeyword; ++it) {
						if (item->HasKeywordByEditorID(*it)) continue;
						missingKeyword = true;
					}
					if (missingKeyword) continue;

					a_ref->RemoveItem(item, pair.second, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}
#ifdef DEBUG
				rulesApplied++;
#endif
			}
		} //Remove rule reading end.

		for (auto& rule : this->addRules) {
			if (merchantContainer && !rule.allowVendors) continue;
			if (!merchantContainer && rule.onlyVendors) continue;
			if (!rule.bypassSafeEdits && isContainerUnsafe) continue;
			if (!IsRuleValid(&rule, a_ref)) continue;
			int32_t ruleCount = rule.count;
			if (rule.count < 1) {
				ruleCount = 1;
			}


				/* added & changed:
				
					if pickAtRandom = 
						> true  ,  distribute only one thing 
						> false ,  distribute everything (default behavior)
				*/

				if (rule.isPickAtRandom) {

					const int index = clib_util::RNG().Generate(0, (int) rule.newForm.size() - 1);

					if (index >= 0 && index < rule.newForm.size())
					{
						
						auto obj = rule.newForm[index];

						auto leveledThing = obj->As<RE::TESLeveledList>();
						if (leveledThing) {
#ifdef DEBUG
							logger::debug("Adding leveled list to container:");
#endif
							AddLeveledListToContainer(leveledThing, a_ref, rule.count);
						}
						else {
							a_ref->AddObjectToContainer(obj, nullptr, rule.count, nullptr);
						}
					}
				}
				else {

					for (auto* obj : rule.newForm) {
						auto leveledThing = obj->As<RE::TESLeveledList>();
						if (leveledThing) {
		#ifdef DEBUG
							logger::debug("Adding leveled list to container:");
		#endif
							AddLeveledListToContainer(leveledThing, a_ref, ruleCount);
						}
						else {
							a_ref->AddObjectToContainer(obj, nullptr, rule.count, nullptr);
						}
					}
				}
#ifdef DEBUG
			rulesApplied++;
#endif
		} //Add rule reading end.

#ifdef DEBUG
		auto now = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - then).count();
		if (rulesApplied > 0) {
			logger::debug("Finished processing container: {}.\n    Applied {} rules in {} nanoseconds.", containerBase->GetFormEditorID(), rulesApplied, elapsed);
		}
#endif
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
		if (a_range < 2500.0f) {
			a_range = 2500.0f;
		}
		else if (a_range > 50000.0f) {
			a_range = 50000.0f;
		}

		this->fMaxLookupRadius = a_range;
	}

}