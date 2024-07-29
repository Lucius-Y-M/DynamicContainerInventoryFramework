#include "../include/utility.h"

namespace Utility {
	bool IsModPresent(std::string a_modName) {
		return RE::TESDataHandler::GetSingleton()->LookupModByName(a_modName) ? true : false;
	}

	bool IsHex(std::string const& s) {
		return s.compare(0, 2, "0x") == 0
			&& s.size() > 2
			&& s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	}

	RE::FormID StringToFormID(std::string a_str) {
		RE::FormID result;
		std::istringstream ss{ a_str };
		ss >> std::hex >> result;
		return result;
	}

	RE::FormID ParseFormID(const std::string& a_identifier) {
		if (const auto splitID = clib_util::string::split(a_identifier, "|"); splitID.size() == 2) {
			const auto  formID = clib_util::string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
		}
		auto* form = RE::TESForm::LookupByEditorID(a_identifier);
		if (form) return form->formID;
		return static_cast<RE::FormID>(0);
	}

	void GetParentChain(RE::BGSLocation* a_child, std::vector<RE::BGSLocation*>* a_parentArray) {
		auto* parent = a_child->parentLoc;
		if (!parent) return;

		if (std::find(a_parentArray->begin(), a_parentArray->end(), parent) != a_parentArray->end()) {
			logger::error("IMPORTANT - Recursive parent locations. Consider fixing this.");
			logger::error("Chain:");
			for (auto* location : *a_parentArray) {
				logger::error("    {} ->", location->GetName());
			}
			return;
		}
		//logger::infonfo("    >{}", clib_util::editorID::get_editorID(parent));
		a_parentArray->push_back(parent);
		return GetParentChain(parent, a_parentArray);
	}

	void ResolveLeveledList(RE::TESLeveledList* a_levItem, RE::BSScrapArray<RE::CALCED_OBJECT>* a_result, uint32_t a_count) {
		RE::BSScrapArray<RE::CALCED_OBJECT> temp{};
		a_levItem->CalculateCurrentFormList(RE::PlayerCharacter::GetSingleton()->GetLevel(), (int16_t) a_count, temp, 0, true);

		for (auto& it : temp) {
			auto* form = it.form;
			auto* leveledForm = form->As<RE::TESLeveledList>();
			if (leveledForm) {
#ifdef DEBUG
				logger::debug("    >Sublist found, resolving that...");
#endif
				ResolveLeveledList(leveledForm, a_result, it.count);
			}
			else {
				logger::debug("    >Resolved: {} -> {}", it.form->GetFormEditorID(), it.count);
				a_result->push_back(it);
			}
		}
	}

}






