#pragma once

#include "EventModel.h"

#include <il2cpp/il2cpp_helper.h>

// Mirror of the game's TournamentGroup (namespace Digit.Prime.Tournaments, Assembly-CSharp).
// A TournamentGroup holds the list of tournaments for an event category plus the
// EventModel describing the group. Used by TournamentManager.ClaimAllTournamentsInCategoryWithPopup.
struct TournamentGroup {
public:
  __declspec(property(get = __get_Tournaments)) void* Tournaments;
  __declspec(property(get = __get_EventModel)) EventModel* EventModel;

  void* __get_Tournaments()
  {
    static auto field = get_class_helper().GetField("<Tournaments>k__BackingField");
    if (field.isValidHelper()) {
      return *(void**)((char*)this + field.offset());
    }
    return nullptr;
  }

  EventModel* __get_EventModel()
  {
    static auto field = get_class_helper().GetField("<EventModel>k__BackingField");
    if (field.isValidHelper()) {
      return *(EventModel**)((char*)this + field.offset());
    }
    return nullptr;
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Tournaments", "TournamentGroup");
    return class_helper;
  }
};
