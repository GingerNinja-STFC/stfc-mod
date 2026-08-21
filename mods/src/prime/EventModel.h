#pragma once

#include <il2cpp/il2cpp_helper.h>

// Mirror of the game's EventModel (namespace Digit.PrimePlatform.Models, Assembly-CSharp).
// EventModel is the data model backing a tournament/event; it is passed to the
// ClaimAllRewardsWithFlyBy/WithPopup methods on TournamentManager.
struct EventModel {
public:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Assembly-CSharp", "Digit.PrimePlatform.Models", "EventModel");
    return class_helper;
  }
};
