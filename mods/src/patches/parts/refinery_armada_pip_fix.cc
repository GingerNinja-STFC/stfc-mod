// Removes PipType.CrossAllianceArmadas from the "Refining" HUD drawer button's pip badge.
//
// That button's NotificationPipWidget bakes a PipTypeFilter containing ~24 CanAffordRefinery*
// pip types plus PipType.CrossAllianceArmadas (2000000038). That inclusion is intentional on
// the game's part - CrossAllianceArmadas are multi-alliance co-op armadas that target refinery
// bosses, so the type is genuinely refinery-relevant - but it means the Refining tab lights up
// with a numbered pip badge whenever any ally starts (or has running) a cross-alliance armada,
// even though there is nothing to claim/afford on the refinery shop itself. That pip is only
// ever dismissed by opening the Alliance -> Combat armadas list (the actual home for that pip
// type), not by visiting Refinery, which reads as a stuck/misplaced notification.
//
// This patch strips PipType.CrossAllianceArmadas out of that one widget's PipTypeFilter at
// Start() time so the Refining badge only reflects genuinely refinery-relevant notifications
// (CanAffordRefinery* etc). The target widget is identified purely by its baked filter contents
// (any NotificationPipWidget whose filter includes both CrossAllianceArmadas and
// CanAffordRefineryMaterials) rather than by GameObject name/hierarchy, so it keeps working
// across future localization or UI-relayout changes to that button, and can't accidentally
// touch some other, unrelated widget that happens to reference CrossAllianceArmadas alone
// (e.g. inside the armadas list itself).
//
// PipTypeFilter's backing PipType[] is a value-type array - its elements are packed 4-byte ints
// directly in the array body, not stored as 8-byte pointer slots. Addressing/overwriting an
// entry must go through il2cpp_array_addr_with_size() (NOT il2cpp_get_array_element(), which is
// only correct for reference-type arrays whose slots hold an object pointer directly).
//
// Unity dispatches MonoBehaviour.Start() through a generic reflective invoke path shared by
// every script in the process, so this hook can in principle be reached with a `_this` that is
// not actually a NotificationPipWidget. Every raw field read below is guarded by an explicit
// runtime class check plus a sane bound on the array length before touching memory.

#include "errormsg.h"

#include "prime/GameObject.h"
#include "prime/Transform.h"

#include <il2cpp/il2cpp_helper.h>

#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <cstdint>
#include <string>

namespace
{
constexpr int32_t kCanAffordRefineryMaterials = 16777216;
constexpr int32_t kCrossAllianceArmadas       = 2000000038;

ptrdiff_t    pip_type_filter_field_offs = -1;
ptrdiff_t    pip_types_field_offs       = -1;
Il2CppClass* widget_class               = nullptr;
Il2CppClass* filter_class               = nullptr;

std::string DescribeWidgetHierarchy(void* widget)
{
  static auto component_helper = il2cpp_get_class_helper("UnityEngine.CoreModule", "UnityEngine", "Component");
  if (!component_helper.isValidHelper()) {
    return "<no Component helper>";
  }

  static auto transform_prop = component_helper.GetProperty("transform");
  if (!transform_prop.isValidHelper()) {
    return "<no transform property>";
  }

  auto*       transform = reinterpret_cast<Transform*>(transform_prop.GetRaw<Il2CppObject>(widget));
  std::string path;
  for (int depth = 0; transform != nullptr && depth < 6; ++depth) {
    auto* go   = transform->gameObject;
    auto  name = go != nullptr ? go->Name() : std::string{"?"};
    path       = path.empty() ? name : (name + "/" + path);
    transform  = transform->parent;
  }

  return path.empty() ? "<no gameobject>" : path;
}

// The filter MUST be patched BEFORE the original Start() runs: Start() itself calls
// PipManager.RegisterPip + UpdatePip, which computes and pushes the badge count from the
// widget's filter. PipManager.UpdatePip only recomputes a widget's count when something
// pushes a pip-type update for a type in its filter, so patching after the original (as an
// earlier revision did) left the count computed with CrossAllianceArmadas still included -
// observed at login, where armada data syncs before the HUD widget starts and the stale
// armada count then stayed on the badge indefinitely.
bool RemoveCrossAllianceArmadasFromFilter(void* widget)
{
  if (widget == nullptr || pip_type_filter_field_offs < 0 || pip_types_field_offs < 0 || widget_class == nullptr ||
      filter_class == nullptr) {
    return false;
  }

  if (il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(widget)) != widget_class) {
    return false;
  }

  auto* filter = *reinterpret_cast<void**>(reinterpret_cast<char*>(widget) + pip_type_filter_field_offs);
  if (filter == nullptr || il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(filter)) != filter_class) {
    return false;
  }

  auto* array = *reinterpret_cast<Il2CppArray**>(reinterpret_cast<char*>(filter) + pip_types_field_offs);
  if (array == nullptr) {
    return false;
  }

  constexpr uintptr_t kMaxSanePipTypeCount = 64;
  const auto          length               = static_cast<uintptr_t>(array->max_length);
  if (length == 0 || length > kMaxSanePipTypeCount) {
    return false;
  }

  bool     has_can_afford_refinery = false;
  int32_t* cross_alliance_slot     = nullptr;
  for (uintptr_t i = 0; i < length; ++i) {
    auto* slot = reinterpret_cast<int32_t*>(il2cpp_array_addr_with_size(array, i, sizeof(int32_t)));
    if (*slot == kCanAffordRefineryMaterials) {
      has_can_afford_refinery = true;
    } else if (*slot == kCrossAllianceArmadas) {
      cross_alliance_slot = slot;
    }
  }

  if (has_can_afford_refinery && cross_alliance_slot != nullptr) {
    *cross_alliance_slot = 0; // PipType 0 is unused/unregistered, so it always contributes 0.
    return true;
  }

  return false;
}

void NotificationPipWidget_Start_Hook(auto original, void* _this)
{
  if (RemoveCrossAllianceArmadasFromFilter(_this)) {
    spdlog::info("[RefineryArmadaPipFix] removed CrossAllianceArmadas from Refinery pip widget at '{}'",
                 DescribeWidgetHierarchy(_this));
  }

  original(_this);
}
} // namespace

void InstallRefineryArmadaPipFixHooks()
{
  static auto widget_helper =
      il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Notifications", "NotificationPipWidget");
  if (!widget_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Notifications", "NotificationPipWidget");
    return;
  }

  static auto filter_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Notifications", "PipTypeFilter");
  if (!filter_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Notifications", "PipTypeFilter");
    return;
  }

  const auto* filter_field = il2cpp_class_get_field_from_name(widget_helper.get_cls(), "_pipTypeFilter");
  if (filter_field == nullptr) {
    spdlog::error("Unable to find field 'NotificationPipWidget->_pipTypeFilter'");
    return;
  }

  const auto* types_field = il2cpp_class_get_field_from_name(filter_helper.get_cls(), "_pipTypes");
  if (types_field == nullptr) {
    spdlog::error("Unable to find field 'PipTypeFilter->_pipTypes'");
    return;
  }

  pip_type_filter_field_offs = filter_field->offset;
  pip_types_field_offs       = types_field->offset;
  widget_class               = widget_helper.get_cls();
  filter_class               = filter_helper.get_cls();

  auto start_method = widget_helper.GetMethod("Start", 0);
  if (start_method == nullptr) {
    ErrorMsg::MissingMethod("NotificationPipWidget", "Start");
    return;
  }

  SPUD_STATIC_DETOUR(start_method, NotificationPipWidget_Start_Hook);
}
