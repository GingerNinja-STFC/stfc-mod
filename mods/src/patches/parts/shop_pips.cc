// Fixes stale "can afford" pips on refineries and other shop-bundle driven stores.
//
// The pips on refinery buildings (and other bundle stores) are PipType.CanAfford*
// counts managed by CanAffordBundlePipManager.HandleCanAffordBundles, driven by
// ShopEvents CanAfford/CannotAfford notifications. Those are raised from
// ShopService.CheckAndNotifyCanAffordBundles, which re-classifies a category's
// cached bundles and queues per-bundle notifications into ShopService._eventContainer
// (fired by ShopService.Tick).
//
// The game only runs that re-classification when bundles are fetched/parsed, on the
// 10s data-refresh tick, and from ResourcesChangedEventHandler (player resources).
// ShopService.InventoryChangedEventHandler (dump.cs RVA 0x18FBF48, current dump) is
// an EMPTY 4-byte stub even though RegisterEvents subscribes it to
// InventoryEvents.InventoryChangedEvent, so spending inventory items (raw refinery
// materials, artifact fragments, event items, ...) never re-evaluates affordability
// and the pips stick around even once nothing is affordable anymore.
//
// This patch hooks the empty handler and runs the exact same refresh the game performs
// on every resource change (ShopService.ResourcesChangedEventHandler body): walk all
// shop categories through the service's BundleCategoryIndex
// (ShopService._bundleDataCache) with TryGetCategory and feed each cached category
// list into ShopService.CheckAndNotifyCanAffordBundles. That path only queues
// Can/CannotAfford notifications - it does not mutate bundle state containers or
// new-flag state - so repeated runs are idempotent.
//
// Deliberately NOT used: ShopService.EvaluateCachedBundles. It first runs
// Bundle.EvaluateState (mutating state containers and triggering reactive events),
// queues BundlesStateChanged per bundle, and only calls CheckAndNotifyCanAffordBundles
// when a bundle's evaluated state actually changed. The game only ever invokes it for
// freshly fetched categories (TryParseServerModel), outdated categories (10s tick) or
// the factions category (FactionsService.RefreshShopBundlesState) - never for all
// categories in bulk. Running it for every category on every data change floods the
// notification pipeline, re-marks bundles as new (NewFlagsManager.SetNew), and never
// unconditionally re-classifies affordability, which leaves inflated/stale pip counts
// in place (observed as refinery pips stuck at "99+").

#include "errormsg.h"

#include <il2cpp/il2cpp_helper.h>

#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <chrono>
#include <cstddef>

namespace
{
constexpr auto kRefreshDebounce = std::chrono::milliseconds{1000};

using GetCategoryLengthFn = int32_t(const void*);
using TryGetCategoryFn    = bool(void*, int32_t, void**, const void*);
using CheckAndNotifyFn    = void(void*, int32_t, void*, const void*);

GetCategoryLengthFn* get_category_length    = nullptr;
TryGetCategoryFn*    try_get_category       = nullptr;
CheckAndNotifyFn*    check_and_notify       = nullptr;
ptrdiff_t            bundle_data_cache_offs = 0;
ptrdiff_t            event_container_offs   = 0;

void RefreshStorePips(void* shop_service)
{
  if (get_category_length == nullptr || try_get_category == nullptr || check_and_notify == nullptr) {
    return;
  }

  auto* bundle_index = *reinterpret_cast<void**>(reinterpret_cast<char*>(shop_service) + bundle_data_cache_offs);
  if (bundle_index == nullptr) {
    return;
  }

  auto* event_container = *reinterpret_cast<void**>(reinterpret_cast<char*>(shop_service) + event_container_offs);
  if (event_container == nullptr) {
    return;
  }

  const auto category_count = get_category_length(nullptr);
  for (int32_t category = 0; category < category_count; ++category) {
    void* category_list = nullptr;
    if (!try_get_category(bundle_index, category, &category_list, nullptr)) {
      continue;
    }

    check_and_notify(shop_service, category, category_list, nullptr);
  }
}

void ShopService_InventoryChangedEventHandler_Hook(auto original, void* _this, void* created, void* dismissed,
                                                   void* updated)
{
  original(_this, created, dismissed, updated);

  if (_this == nullptr) {
    return;
  }

  static std::chrono::steady_clock::time_point last_refresh{};
  const auto                                   now = std::chrono::steady_clock::now();
  if (last_refresh != std::chrono::steady_clock::time_point{} && now - last_refresh < kRefreshDebounce) {
    return;
  }
  last_refresh = now;

  RefreshStorePips(_this);
  spdlog::debug("[ShopPipFix] re-evaluated store pips after inventory change");
}
} // namespace

void InstallShopPipFixHooks()
{
  static auto shop_service_helper =
      il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimePlatform.Content", "ShopService");
  if (!shop_service_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.PrimePlatform.Content", "ShopService");
    return;
  }

  static auto bundle_index_helper =
      il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimePlatform.Content", "BundleCategoryIndex");
  if (!bundle_index_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.PrimePlatform.Content", "BundleCategoryIndex");
    return;
  }

  static auto shop_category_helper =
      il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.Prime.Shop", "ShopCategory");
  if (!shop_category_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Shop", "ShopCategory");
    return;
  }

  get_category_length = shop_category_helper.GetMethod<GetCategoryLengthFn>("get_Length");
  if (get_category_length == nullptr) {
    ErrorMsg::MissingStaticMethod("ShopCategory", "get_Length");
    return;
  }

  try_get_category = bundle_index_helper.GetMethod<TryGetCategoryFn>("TryGetCategory");
  if (try_get_category == nullptr) {
    ErrorMsg::MissingMethod("BundleCategoryIndex", "TryGetCategory");
    return;
  }

  check_and_notify = shop_service_helper.GetMethod<CheckAndNotifyFn>("CheckAndNotifyCanAffordBundles");
  if (check_and_notify == nullptr) {
    ErrorMsg::MissingMethod("ShopService", "CheckAndNotifyCanAffordBundles");
    return;
  }

  const auto* bundle_data_cache_field =
      il2cpp_class_get_field_from_name(shop_service_helper.get_cls(), "_bundleDataCache");
  if (bundle_data_cache_field == nullptr) {
    spdlog::error("Unable to find field 'ShopService->_bundleDataCache'");
    return;
  }

  const auto* event_container_field =
      il2cpp_class_get_field_from_name(shop_service_helper.get_cls(), "_eventContainer");
  if (event_container_field == nullptr) {
    spdlog::error("Unable to find field 'ShopService->_eventContainer'");
    return;
  }

  bundle_data_cache_offs = bundle_data_cache_field->offset;
  event_container_offs   = event_container_field->offset;

  const auto inventory_changed_handler = shop_service_helper.GetMethod("InventoryChangedEventHandler", 3);
  if (inventory_changed_handler == nullptr) {
    ErrorMsg::MissingMethod("ShopService", "InventoryChangedEventHandler");
    return;
  }

  SPUD_STATIC_DETOUR(inventory_changed_handler, ShopService_InventoryChangedEventHandler_Hook);
}
