#pragma once



#include "AliasRust.h"



////// for better printing
#define CONSOLE_PRINT RE::ConsoleLog::GetSingleton()->Print



using FID = u32; // RE::FormID = uint32_t


// using BoundObjPtr = RE::TESBoundObject *;
// using NPCPtr = RE::TESNPC *;
// using KeywordPtr = RE::BGSKeyword *;
// using SpellPtr = RE::SpellItem *;
// using MGEFPtr = RE::ActiveEffect *;
// using BookPtr = RE::TESObjectBOOK *;

// using GlobVarPtr = RE::TESGlobal *;
// using FormListPtr = RE::BGSListForm *;

using BoundObj = RE::TESBoundObject;
using NPC = RE::TESNPC;
using Keyword = RE::BGSKeyword;
using Spell = RE::SpellItem;
using Actor = RE::Actor;


using ActEfct = RE::ActiveEffect;
/* MGEF = EffectSetting */
using MGEF = RE::EffectSetting;
/* ActualMGEF = the class that is accepted by "spell->effects.push_back()"  */
using ActualMGEF = RE::Effect;


using Book = RE::TESObjectBOOK;

using GlobVar = RE::TESGlobal;
using FormList = RE::BGSListForm;


using AV = RE::ActorValue;
using AVMod = RE::ACTOR_VALUE_MODIFIER;





using CondOpCode = RE::CONDITION_ITEM_DATA::OpCode;
using CondFunction = RE::FUNCTION_DATA::FunctionID;


using ConditionLinkedList   = RE::TESCondition;
using Condition             = RE::TESConditionItem;









static constexpr inline FID PLAYER_FID = 0x14;

static inline auto GetPlayer() {
    return RE::PlayerCharacter::GetSingleton();
}

#define PLAYER GetPlayer()



/*

    REQUIRES PO3_TWEAKS

    e.g. if TPtr = KeywordPtr,
    this returns a Some(keyword: KeywordPtr) OR None
*/
template<typename T>
[[maybe_unused]]
static inline Option<T*> GetByEDID(Str edid) {
    if (T* form = RE::TESForm::LookupByEditorID<T*>(edid); form) {
        return form;
    }

    return None;

}


template<typename T>
[[maybe_unused]]
static inline Option<T*> GetByFormID(FID formID, Str pluginName) {

    // static_assert( IsSame<T*, T*>::value, "Do NOT manually override generic alias T*!" );   // prevent accidental overriding of T* alias

    if (const auto handler = RE::TESDataHandler::GetSingleton(); handler) {
        if (auto * form = handler->LookupForm(formID, pluginName); form) {

            /* note:
                auto x = form->As<Keyword>();

                here, type of x is:

                KeywordPtr (or: RE::BGSKeyword *)
            
            */
            if (T* obj = form->As<T>() ; obj) {
                return obj;
            }
        }
    }
    return None;
}



/*
    runs a console command.
    TRUE: successful (should be in MOST cases)
    FALSE: scriptFactory failed to create


    code below mostly from dTry's combat music fix
*/
static inline bool sendConsoleCommand(const Str & command) {
    const auto scriptFac = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
    if (const auto script = (scriptFac) ? scriptFac->Create() : nullptr; script) {
        const auto selectRef = RE::Console::GetSelectedRef();
        script->SetCommand(command);
        script->CompileAndRun(selectRef.get());
        delete script;
        return true;
    }
    else {
        return false;
    }
}










    /*

        The following 2 funcs are lifted directly
        from Ershin's True Directional Movement

        I still don't quite understand how / why the hash func has to be like this
        but I KNOW it works
    */

    using HashCode = usize;

    static constexpr inline HashCode ErshHash(CString data, const usize size) {
        usize hash = 5381;

        for (CString ch = data; ch < data + size; ch++) {
            hash = ((hash << 5) + hash) + (unsigned char) * ch;
        }

        return hash;
    }

    static constexpr inline HashCode operator"" _h (CString data, const usize size) {
        return ErshHash(data, size);
    }