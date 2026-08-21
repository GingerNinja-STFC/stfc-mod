#pragma once

#include "errormsg.h"

#include <il2cpp/il2cpp_helper.h>

#include "MonoSingleton.h"

struct TournamentManager : MonoSingleton<TournamentManager> {
  friend struct MonoSingleton<TournamentManager>;

public:
  void ClaimRewards(void* tournament, void* callback, bool forceRefresh = true)
  {
    static auto method =
        get_class_helper().GetMethod<void(TournamentManager*, void*, void*, bool)>("ClaimRewards");
    static auto warn = true;
    if (method) {
      method(this, tournament, callback, forceRefresh);
    } else if (warn) {
      warn = false;
      ErrorMsg::MissingMethod("TournamentManager", "ClaimRewards");
    }
  }

  void ClaimAllRewards(void* tournaments, void* callback, bool forceRefresh = true)
  {
    static auto method =
        get_class_helper().GetMethod<void(TournamentManager*, void*, void*, bool)>("ClaimAllRewards");
    static auto warn = true;
    if (method) {
      method(this, tournaments, callback, forceRefresh);
    } else if (warn) {
      warn = false;
      ErrorMsg::MissingMethod("TournamentManager", "ClaimAllRewards");
    }
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Tournaments", "TournamentManager");
    return class_helper;
  }
};
