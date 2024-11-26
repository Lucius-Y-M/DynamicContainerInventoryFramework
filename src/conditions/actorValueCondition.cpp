#include "actorValueCondition.h"

#include "RE/misc.h"

namespace Conditions
{
	bool AVCondition::IsValid(RE::TESObjectREFR* a_container) 
	{
		(void)a_container;
		const auto player = RE::PlayerCharacter::GetSingleton();
		assert(player);
		if (player && player->GetActorValue(RE::LookupActorValueByName(value)) >= minValue) {
			return !inverted;
		}
		return inverted;
	}

	AVCondition::AVCondition(const char* a_value, float a_minValue)
	{
		this->value = a_value;
		this->minValue = a_minValue;
	}
}