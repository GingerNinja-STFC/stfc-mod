// Fixes the stale "can promote" pip on the officer tab.
//
// The officers tab pip is PipType.Officers, counted by OfficersPipManager from
// persisted NewFlagType.InventoryItemOfficers breadcrumbs
// (OfficersPipManager.InitializePipCount counts the breadcrumb-bit entries from
// NewFlagsManager at session start; HandleBreadcrumbChanged keeps the count live
// afterwards). Those breadcrumbs mean "this officer can be promoted with your
// current shards and rank" - OfficerManager.OnOfficerPromoteSuccess and
// FleetCommanderManager.OnOfficerPromoteSuccess set/clear them via
// NewFlagsManager.SetBreadcrumb/ClearBreadcrumb based on
// Officer.HasEnoughShardsAndRankToPromote, but ONLY for the officer that was just
// promoted.
//
// When shard counts change any other way (chest claims, purchases, spending shards
// on a different officer), no other officer's breadcrumb is re-evaluated. The
// breadcrumb bit persists in NewFlagDataCache (saved to cloud/local storage), so a
// stale breadcrumb resurrects the pip at every login. Leaving the officer roster
// zeroes the displayed count, but the underlying flag survives the IsKnown-gated
// ClearNew there and comes back on the next session.
//
// This patch re-runs the game's own validation for all owned officers: every few
// seconds from OfficerManager.Update (main thread), walk the owned officers from
// InventoryDataContainer._ownedOfficers (rebuilt on every officers sync) and for
// each officer whose breadcrumb is set, re-check
// Officer.HasEnoughShardsAndRankToPromote. Stale breadcrumbs are cleared with
// NewFlagsManager.ClearBreadcrumb - a no-op for ids without a flag - which clears
// the persisted bit, marks the cache dirty for saving, and fires the game's own
// BreadcrumbChanged event so OfficersPipManager updates the pip count through its
// usual path. Only stale breadcrumbs are cleared; no new breadcrumbs are ever set,
// matching the game's own promote-time semantics.
//
// The InventoryDataContainer is captured directly from
// InventoryDataContainer.OnPlayerInventoriesMessage (which fires on every
// inventory sync, including the initial one at login) and kept alive through a
// strong GC handle. Reaching it through OfficerManager's
// CachedService<InventoryService> does not work: that cache is only resolved by
// officer UI code paths, so it stays null until the player actually opens an
// officer screen.

#include "prime/MonoSingleton.h"

#include "errormsg.h"

#include <il2cpp/il2cpp_helper.h>

#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <atomic>
#include <chrono>
#include <cstddef>

namespace
{
constexpr auto      kRefreshInterval = std::chrono::milliseconds{5000};
constexpr int32_t   kOfficerFlagType = 1; // NewFlagType.InventoryItemOfficers
constexpr ptrdiff_t kListItemsOffs   = 0x10;
constexpr ptrdiff_t kListSizeOffs    = 0x18;
constexpr ptrdiff_t kArrayLengthOffs = 0x18;
constexpr ptrdiff_t kArrayDataOffs   = 0x20;

struct OfficerManager {
  static IL2CppClassHelper& class_helper()
  {
    static auto helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Officers", "OfficerManager");
    return helper;
  }
};

struct NewFlagsManager : MonoSingleton<NewFlagsManager> {
  friend struct MonoSingleton<NewFlagsManager>;

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.NewFlags", "NewFlagsManager");
    return helper;
  }
};

using GetOfficerIdFn      = int64_t(void*, const void*);
using CanOfficerPromoteFn = bool(void*, const void*);
using ShowBreadcrumbFn    = bool(void*, int32_t, int64_t, bool*, const void*);
using ClearBreadcrumbFn   = void(void*, int32_t, int64_t, const void*);

GetOfficerIdFn*      get_officer_id      = nullptr;
CanOfficerPromoteFn* can_promote         = nullptr;
ShowBreadcrumbFn*    show_breadcrumb     = nullptr;
ClearBreadcrumbFn*   clear_breadcrumb    = nullptr;
ptrdiff_t            owned_officers_offs = 0;

std::atomic<Il2CppGCHandle> captured_container{nullptr};

void* CapturedContainer()
{
  auto handle = captured_container.load(std::memory_order_acquire);
  return handle != nullptr ? il2cpp_gchandle_get_target(handle) : nullptr;
}

void RefreshOfficerBreadcrumbs()
{
  static bool warned_no_flags_manager  = false;
  static bool warned_no_container      = false;
  static bool warned_no_officers       = false;
  static bool reported_first_refresh   = false;
  static bool reported_promotable_keep = false;

  auto* new_flags = NewFlagsManager::Instance();
  if (new_flags == nullptr) {
    if (!warned_no_flags_manager) {
      warned_no_flags_manager = true;
      spdlog::warn("[OfficerPipFix] NewFlagsManager instance not available yet");
    }
    return;
  }

  auto* data_container = CapturedContainer();
  if (data_container == nullptr) {
    if (!warned_no_container) {
      warned_no_container = true;
      spdlog::info("[OfficerPipFix] waiting for inventory data (container not captured yet)");
    }
    return;
  }

  auto* owned_officers = *reinterpret_cast<void**>(reinterpret_cast<char*>(data_container) + owned_officers_offs);
  if (owned_officers == nullptr) {
    if (!warned_no_officers) {
      warned_no_officers = true;
      spdlog::info("[OfficerPipFix] waiting for owned officers list");
    }
    return;
  }

  auto*      items = *reinterpret_cast<void**>(reinterpret_cast<char*>(owned_officers) + kListItemsOffs);
  const auto size  = *reinterpret_cast<int32_t*>(reinterpret_cast<char*>(owned_officers) + kListSizeOffs);
  if (items == nullptr || size <= 0) {
    return;
  }

  const auto max_length = *reinterpret_cast<int32_t*>(reinterpret_cast<char*>(items) + kArrayLengthOffs);
  auto**     officers   = reinterpret_cast<void**>(reinterpret_cast<char*>(items) + kArrayDataOffs);

  int32_t flagged = 0;
  int32_t cleared = 0;
  for (int32_t i = 0; i < size && i < max_length; ++i) {
    auto* officer = officers[i];
    if (officer == nullptr) {
      continue;
    }

    const auto officer_id     = get_officer_id(officer, nullptr);
    bool       breadcrumb_set = false;
    if (!show_breadcrumb(new_flags, kOfficerFlagType, officer_id, &breadcrumb_set, nullptr) || !breadcrumb_set) {
      continue;
    }
    ++flagged;

    if (can_promote(officer, nullptr)) {
      if (!reported_promotable_keep) {
        reported_promotable_keep = true;
        spdlog::info("[OfficerPipFix] officer {} breadcrumb kept (officer can still be promoted)", officer_id);
      }
      continue;
    }

    clear_breadcrumb(new_flags, kOfficerFlagType, officer_id, nullptr);
    ++cleared;
    spdlog::info("[OfficerPipFix] cleared stale promote breadcrumb for officer {}", officer_id);
  }

  if (!reported_first_refresh) {
    reported_first_refresh = true;
    spdlog::info("[OfficerPipFix] validated {} owned officers, {} breadcrumb flag(s), cleared {}", size, flagged,
                 cleared);
  }
}

void OfficerManager_Update_Hook(auto original, void* _this)
{
  original(_this);

  static std::chrono::steady_clock::time_point last_refresh{};
  const auto                                   now = std::chrono::steady_clock::now();
  if (last_refresh != std::chrono::steady_clock::time_point{} && now - last_refresh < kRefreshInterval) {
    return;
  }
  last_refresh = now;

  RefreshOfficerBreadcrumbs();
}

void InventoryDataContainer_OnPlayerInventoriesMessage_Hook(auto original, void* _this, void* message)
{
  original(_this, message);

  if (_this == nullptr) {
    return;
  }

  auto existing = captured_container.load(std::memory_order_acquire);
  if (existing != nullptr && il2cpp_gchandle_get_target(existing) == _this) {
    return;
  }

  auto handle = il2cpp_gchandle_new(reinterpret_cast<Il2CppObject*>(_this), false);
  if (handle == nullptr) {
    return;
  }

  auto old = captured_container.exchange(handle, std::memory_order_acq_rel);
  if (old != nullptr) {
    il2cpp_gchandle_free(old);
  }

  spdlog::info("[OfficerPipFix] inventory data container captured");
}
} // namespace

void InstallOfficerPipFixHooks()
{
  if (!OfficerManager::class_helper().isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Officers", "OfficerManager");
    return;
  }

  static auto new_flags_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.NewFlags", "NewFlagsManager");
  if (!new_flags_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.NewFlags", "NewFlagsManager");
    return;
  }

  static auto inventory_data_helper =
      il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Services", "InventoryDataContainer");
  if (!inventory_data_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.PrimeServer.Services", "InventoryDataContainer");
    return;
  }

  static auto officer_helper =
      il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Models", "Officer");
  if (!officer_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.PrimeServer.Models", "Officer");
    return;
  }

  get_officer_id = officer_helper.GetMethod<GetOfficerIdFn>("get_Id");
  if (get_officer_id == nullptr) {
    ErrorMsg::MissingMethod("Officer", "get_Id");
    return;
  }

  can_promote = officer_helper.GetMethod<CanOfficerPromoteFn>("get_HasEnoughShardsAndRankToPromote");
  if (can_promote == nullptr) {
    ErrorMsg::MissingMethod("Officer", "get_HasEnoughShardsAndRankToPromote");
    return;
  }

  show_breadcrumb = new_flags_helper.GetMethod<ShowBreadcrumbFn>("ShowBreadcrumb");
  if (show_breadcrumb == nullptr) {
    ErrorMsg::MissingMethod("NewFlagsManager", "ShowBreadcrumb");
    return;
  }

  clear_breadcrumb = new_flags_helper.GetMethod<ClearBreadcrumbFn>("ClearBreadcrumb");
  if (clear_breadcrumb == nullptr) {
    ErrorMsg::MissingMethod("NewFlagsManager", "ClearBreadcrumb");
    return;
  }

  const auto* owned_officers_field =
      il2cpp_class_get_field_from_name(inventory_data_helper.get_cls(), "_ownedOfficers");
  if (owned_officers_field == nullptr) {
    spdlog::error("Unable to find field 'InventoryDataContainer->_ownedOfficers'");
    return;
  }
  owned_officers_offs = owned_officers_field->offset;

  const auto update_method = OfficerManager::class_helper().GetMethod("Update", 0);
  if (update_method == nullptr) {
    ErrorMsg::MissingMethod("OfficerManager", "Update");
    return;
  }
  SPUD_STATIC_DETOUR(update_method, OfficerManager_Update_Hook);

  const auto inventories_parsed = inventory_data_helper.GetMethod("OnPlayerInventoriesMessage", 1);
  if (inventories_parsed == nullptr) {
    ErrorMsg::MissingMethod("InventoryDataContainer", "OnPlayerInventoriesMessage");
    return;
  }
  SPUD_STATIC_DETOUR(inventories_parsed, InventoryDataContainer_OnPlayerInventoriesMessage_Hook);
}
