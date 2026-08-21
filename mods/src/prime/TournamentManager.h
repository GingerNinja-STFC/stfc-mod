#pragma once

#include "errormsg.h"

#include <il2cpp/il2cpp_helper.h>

#include "MonoSingleton.h"

// Mirror of the game's TournamentManager (namespace Digit.Prime.Tournaments,
// Assembly-CSharp). TournamentManager is a MonoSingleton that owns event
// ("tournament") reward claiming. The *WithPopup methods show the full
// GenericRewardsScreenViewController animation; the *WithFlyBy methods show
// the reduced flyby animation; the raw Claim* methods claim with no UI at all.
struct TournamentManager : MonoSingleton<TournamentManager> {
  friend struct MonoSingleton<TournamentManager>;

public:
  // Raw claim (no popup, no animation). The callback may be nullptr.
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

  // Raw claim-all (no popup, no animation).
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

  // Reduced flyby animation claim.
  void ClaimRewardsWithFlyBy(void* tournament, void* flyByRewards)
  {
    static auto method =
        get_class_helper().GetMethod<void(TournamentManager*, void*, void*)>("ClaimRewardsWithFlyBy");
    static auto warn = true;
    if (method) {
      method(this, tournament, flyByRewards);
    } else if (warn) {
      warn = false;
      ErrorMsg::MissingMethod("TournamentManager", "ClaimRewardsWithFlyBy");
    }
  }

  // Reduced flyby animation claim-all.
  void ClaimAllRewardsWithFlyBy(void* tournaments, void* eventModel, void* flyByRewards)
  {
    static auto method =
        get_class_helper().GetMethod<void(TournamentManager*, void*, void*, void*)>("ClaimAllRewardsWithFlyBy");
    static auto warn = true;
    if (method) {
      method(this, tournaments, eventModel, flyByRewards);
    } else if (warn) {
      warn = false;
      ErrorMsg::MissingMethod("TournamentManager", "ClaimAllRewardsWithFlyBy");
    }
  }

  static bool IsClaimableEvent(void* tournament, bool excludeMetaEvents = false)
  {
    static auto method =
        get_class_helper().GetMethod<bool(void*, bool)>("IsClaimableEvent");
    static auto warn = true;
    if (method) {
      return method(tournament, excludeMetaEvents);
    } else if (warn) {
      warn = false;
      ErrorMsg::MissingStaticMethod("TournamentManager", "IsClaimableEvent");
    }
    return false;
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Tournaments", "TournamentManager");
    return class_helper;
  }
};
