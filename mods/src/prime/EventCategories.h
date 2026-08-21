#pragma once

#include <cstdint>

#include <il2cpp/il2cpp_helper.h>

// Mirror of the game's EventCategories struct (Digit.PrimePlatform.Models).
// The struct holds a single int _flagValue at offset 0x0; the static
// EventCategories.Values class lists the well-known category constants.
struct EventCategories {
public:
  // Well-known category values from EventCategories.Values (see dump.cs).
  enum Value : int32_t {
    Standard            = 0,
    Dailygoals          = 1,
    Dailymilestone      = 2,
    Leaderboard         = 3,
    Stat                = 4,
    BattlePassSeason    = 5,
    BattlePassEvent     = 6,
    TreasuryProgress    = 7,
    TreasuryReward      = 8,
    ServerClashEvent    = 9,
    WebstoreEvent       = 10,
    Playerlifecycle     = 11,
    FieldTraining       = 12,
    FtCategory          = 13,
    Cutscenes           = 14,
    MinigameCategory    = 15,
    MinigameStage       = 16,
    Warchest            = 17,
    AllianceGame        = 18,
    AllianceGameTask    = 19,
    MetaEventCategory   = 20,
    MetaEventObjective  = 21,
    Invasion            = 22,
    LoopMuseum          = 23,
    LoopMuseumTask      = 24,
    PlcBpSeason         = 25,
    PlcBpEvent          = 26,
    ProgressionReward   = 27,
    Factionweeklyevents = 28,
  };

  __declspec(property(get = __get__flagValue)) int Value;

  int __get__flagValue()
  {
    static auto field = get_class_helper().GetProperty("Value");
    return *field.GetUnboxedSelf<int>(this);
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimePlatform.Models", "EventCategories");
    return class_helper;
  }
};
