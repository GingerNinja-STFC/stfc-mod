#pragma once

#include <il2cpp/il2cpp_helper.h>

// Mirror of the game's FlyByRewardsController (namespace Digit, Assembly-CSharp).
// This is the "reduced" reward animation controller used by the *WithFlyBy claim
// paths in TournamentManager, as opposed to the full popup used by *WithPopup.
struct FlyByRewardsController {
public:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit", "FlyByRewardsController");
    return class_helper;
  }
};
