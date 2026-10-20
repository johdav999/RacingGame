# Typed AI blackboards

An immutable `StateMachineData::blackboard` schema declares up to 64 fields.
`AiParameter` and `AiValue` live in `engine/ai_value.hpp`; Boolean, Number, Vector
and Actor reuse the controller parameter types. Names are unique 1–256 byte keys
without whitespace/control characters. Numeric and vector defaults/updates must be
finite and within the declared inclusive bounds. Actor values are optional saved
actor IDs, resolved against the owning scene rather than retained pointers.

Each `AiControllerComponent` copies the graph defaults into its own live map.
Shared graphs never share mutable values. Update inputs synchronously on the scene
owner, normally in the owning actor's fixed tick before child AI and motor ticks:

```cpp
auto data = graph->data();
data.blackboard = {{"targetVisible", engine::AiParameterType::Boolean, false},
                   {"velocity", engine::AiParameterType::Vector,
                    engine::Vec3{1,0,0}, -100, 100}};
auto definition = engine::StateMachineDefinition::create(std::move(data));
auto settings = engine::AiControllerSettings::defaults(definition, *registry);
settings.states.at(moveState).sources["velocity"] = "velocity";
settings.conditions.at("flag").sources["value"] = "targetVisible";
// Attach using the existing actor controller API, then feed gameplay inputs:
controller.setBlackboard({{"targetVisible", visible},
                         {"velocity", engine::Vec3{speed,0,0}}});
```

`setBlackboard(patch)` validates the complete patch before publishing it. Unknown
fields, wrong types, nonfinite/out-of-range values and nonempty actor IDs absent
from an attached controller's scene reject without changing any values, current
state, timers, commands or transition history. Live references whose actors are
later removed/disabled resolve to null. Detached writes validate ID syntax; scene
resolution becomes available after attachment. These APIs are single-owner APIs,
not worker-safe subscriptions or an asynchronous event queue.

`AiBinding::sources` maps a parameter name to a field name. `parameters` still
contains a complete valid constant configuration. A source overrides its constant
at each enter/update/exit/condition callback, which receives a resolved value copy.
Source types must match exactly; numeric/vector field bounds must fit entirely
inside the consuming parameter bounds. Unknown source fields/parameters reject
the complete configuration. Collision-aware `move` requires Y=0 both for defaults
and live writes. Narrow fields or adapter callbacks enforce additional game rules.
`flag` and `not-flag` provide positive/negative Boolean guards.

`blackboard()` and `blackboardValue(key)` expose read-only live values; an unknown
lookup throws. Returned maps/value references are borrowed until the next patch,
configuration replacement or component destruction. `restart()` retains live gameplay inputs while resetting runtime
timers/history; `configure()` replaces the schema/configuration and resets values
to defaults. Stop/disable cancels the controller normally and retains inputs.

## Editor and debugger

Open **Details → AI state machines → Open AI graph editor**. In **Blackboard
fields**, add/remove fields and edit their type, default and bounds, then choose
**Apply fields**. This is one validated graph-history edit; Undo/Redo and `.weai`
files retain the schema. Schema editing is gated during graph preview and Play.
The standalone graph preview still uses its manual guard checkboxes; it does not
execute gameplay callbacks or actor bindings.

Choose **Use open AI graph** in **Actor AI controller**. Each parameter offers
**Constant** or a compatible **Blackboard: field** source. Apply the assignment
through the existing scene history. Removing/renaming a field in the graph does
not silently mutate previously assigned snapshots. Reassign and remap explicitly.

During Play, **Live debugger during Play** shows current values, editable typed
inputs and guard labels in the bounded transition log. Live edits affect only the
selected reconstructed Play instance and never modify source defaults/history.
The values shown alongside controls refresh even while an input is being edited;
rejected writes retain the old runtime values. Transition labels identify the
accepted graph guard, not arbitrary callback explanations or an exhaustive guard
evaluation trace.

## Persistence and example

Graph files now write `WEAI 5`; readers accept versions 1, 2, 3, 4 and 5. Version 1 has an
empty schema. Controller payloads write `AIC 2` with parameter-source mappings and
accept version 1 constants. The outer optional component codec stays version 1;
the payload version identifies its grammar. Existing 512 KiB graph / 2 MiB
controller limits are checked on save and load. Matching browser code/Wasm must
be deployed together. Scenes and Play retain schema/defaults/bindings, while live
values, timers, commands and transition history remain transient.

The application-owned patrol demo in `src/ai_demo.cpp` now uses an actor controller
with live perception flags, `target` and `lastSeenPosition`. Actor fixed ticks
publish observations; registered application callbacks own the Navigator and
patrol/follow/investigate/return policy. Target loss leads to investigation and
return while navigation failure remains a bounded blocked state. Custom registries
and their captures remain application-owned; generic bindings do not acquire
automatic perception, navigation or callback discovery.

See [controller contracts](ai-controllers.md) and
selected engine docs/ai-blackboards-validation.md.

WEAI3 also retains hierarchy. One per-agent blackboard is shared across its active chain, without scoped shadowing. See [hierarchical AI](ai-hierarchy.md).

[Typed event queues](ai-events.md) carry one-time inputs alongside persistent blackboard values. Game callbacks may copy triggering payloads into validated fields; queue delivery does not implicitly change gameplay data.

## AI task sequences

AiState now carries <=16 ordered AiTaskSpec entries with registered keys, exact typed parameters, compatible blackboard sources, timeout (0,86400] default30, optional observation event and zero-payload succeededEvent/failedEvent names. AiBindingRegistry::create accepts an optional third vector of up to128 AiTaskBinding factories. Each controller owns fresh AiTask instances; start(controller,spec,activationScope) snapshots inputs, tick returns Running/Succeeded/Failed, and cancel noexcept releases work on every terminal/exit path. At most one task ticks per active state per fixed step; callbacks run only on the scene owner. Scoped terminal events defer through the existing cutoff, retry once per later update while the queue is full and cancel on exit. Retained ancestors retain tasks. Controller stop from callbacks defers teardown safely; configure/restart/source replacement cannot replace executing ownership.

Built-ins: wait(seconds), wait-event(spec.event), move-to(target Vector/speed/distance) via existing Navigator and explicit transient setNavigationSource(source) followed by restart(), face-target(target Vector,+Z facing). Motor/pose tasks require an existing motor, and competing active AI writers reject. Source revision invalidation remains authoritative; no automatic bake/ground/animation subscription. Application animation factories use scoped component handles and own/cancel subscriptions. See [task contracts](ai-tasks.md).

Graph task forms add/reorder/remove tasks, typed constants/source mapping, timeouts and result events with one Apply history edit. Actor Play live tasks show index/key, elapsed, result, reason and pending delivery, with <=32 history records. Standalone manual-guard preview explicitly leaves task execution to Actor Play/custom hosts. WEAI5 reads1..5 inside unchanged AIC2/outer codec1; objects, progress, providers/subscriptions and history stay transient. Run ai_tasks plus state_machine/ai_controller/ai/editor/editor_structure/application/scene_archive checks and scripts/editor-ai-tasks-smoke.cjs on WebGPU/WebGL2 at DPR1/2; preserve exact artifacts and report any concurrent-suite failures.
