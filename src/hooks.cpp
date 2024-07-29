#pragma once


#include "hooks.h"
#include "containerManager.h"

namespace Hooks {
	bool TESObjectReference_Initialize::Install()
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(19507) };
		stl::write_thunk_call<TESObjectReference_Initialize>(target.address() + 0x78C);
		return true;
	}

	void TESObjectReference_Initialize::thunk(RE::TESObjectREFR* a_container, bool a_keepChanges)
	{
		func(a_container, a_keepChanges);

		if (a_container->GetBaseObject() && a_container->GetBaseObject()->As<RE::TESObjectCONT>()) {
			
			ContainerManager::ContainerManager::GetSingleton()->HandleContainer(a_container);
		}
	}

	bool TESObjectReference_Reset::Install()
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(19802) };
		stl::write_thunk_call<TESObjectReference_Reset>(target.address() + 0x12B);
		return true;
	}

	void TESObjectReference_Reset::thunk(RE::TESObjectREFR* a_container, bool a_keepChanges)
	{
		func(a_container, a_keepChanges);

		if (a_container->GetBaseObject() && a_container->GetBaseObject()->As<RE::TESObjectCONT>()) {

			ContainerManager::ContainerManager::GetSingleton()->HandleContainer(a_container);
		}
	}

	bool Install()
	{
		return TESObjectReference_Initialize::Install() && TESObjectReference_Reset::Install();
	}
}
