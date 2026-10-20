# Hierarchical AI states

`StateMachine` in optional `engine_ai` now supports one active chain of nested
states. Flat definitions use the same API and keep their transition order.
`AiState::parent` and `AiState::initialChild` are optional state IDs. A state with
children is a group and must name one direct initial child. The graph's initial
state must be a root. Entering a group recursively selects its initial children.

```cpp
engine::StateMachineData data;
data.states = {
    {0, "Alive",  "idle", {60,80}, {}, 1},
    {1, "Patrol", "move", {100,220}, 0, {}},
    {2, "Combat", "idle", {460,80}, 0, 3},
    {3, "Chase",  "move", {460,220}, 2, {}},
    {4, "Dead",   "idle", {1050,80}}
};
data.transitions = {{0,0,4,"flag",0}, {1,1,2,"always",.5}};
data.blackboard = {{"dead", engine::AiParameterType::Boolean, false}};
auto graph = engine::StateMachineDefinition::create(std::move(data));
auto registry = engine::AiBindingRegistry::builtins();
auto settings = engine::AiControllerSettings::defaults(graph, *registry);
settings.conditions.at("flag").sources["value"] = "dead";
// Attach to an actor with a configured motor using the controller API.
// Setting dead=true interrupts Patrol or any nested Combat state.
```

Definitions reject missing parents, cycles, non-root graph entry, missing/wrong
initial children and paths deeper than 16 states. Existing limits of 256 total
states, 1024 transitions and 64 blackboard fields remain. IDs including zero are
ordinary IDs; `std::nullopt` represents no parent/child.

## Runtime ordering, timing and cancellation

The scene owner supplies the host and simulation delta synchronously. No new
thread, scheduler, retained host, subscription or scene owner is introduced.

1. Initial entry runs outermost parent to innermost leaf. Entry records a state
   before its callback, so a failing enter still receives one cleanup exit.
2. Each step advances all active state timers and calls update once per active
   state, outer to inner. Updates precede transition evaluation as in flat graphs.
3. Transitions are checked from outermost active parent to leaf. Within each
   source, stored vector order is priority. An eligible parent interrupt therefore
   wins over a child edge, even if the child edge appears earlier in the file.
4. `minimumSeconds` measures the source state's active simulation time. A retained
   parent keeps its timer across child/sibling transitions; newly entered states
   start at zero. Destination updates wait until the next step.
5. All edges are external transitions. Retain only common ancestors strictly above
   both the declared source and target. Exit the rest inner to outer, then enter
   the new path outer to inner, descending through target initial children.

For `Alive / Combat / Chase → Attack`, Alive and Combat remain active. A transition
declared on Alive to Dead exits Chase, Combat and Alive. A Combat self-transition,
or a descendant's transition targeting Combat, exits/reenters Combat and its
initial subtree while retaining Alive. There are no internal/history transitions.

The existing per-step budgets still apply across the whole chain: default one
transition and 1024 conditions; configurable limits 1–64 / 1–65536. Cycles defer
at the transition limit. Depth bounds entry/exit/update work; budgets do not
establish a frame-time guarantee. Pause by not stepping.

`stop(host)` unwinds the complete active chain once and stays stopped. Restart
validates before unwinding, then enters the requested root or nested target and
its ancestors on the next step. Callback failure or condition exhaustion unwinds
every active state, marks Failed and rethrows. Exit remains noexcept, and reentry
from enter/update/condition/exit/transition callbacks is forbidden.

Game callbacks own persistent navigation/subscriptions and release them in exit.
Retained groups deliberately keep their resources across child transitions;
leaving that group cancels them. Actor controllers invoke each active state's
registered binding and clear motor commands during exits, disable/detach and Stop.
Give movement commands an explicit owner; multiple active bindings should not
compete to write the same motor. Idle group bindings are useful for organization.

## Diagnostics and actor controllers

`current()` / `currentState()` identify the leaf. `activePath()` returns the
outer-to-inner active IDs (empty before entry or after stop/failure).
`stateSeconds()` reports leaf time; the portable machine's
`stateSeconds(activeStateId)` reports an active ancestor's time and throws for an
inactive ID. Path spans are borrowed until the next step, stop, restart or instance
destruction. Stopped machines retain the last current ID and leaf timer.

`StateMachineHost::transitioned(edge, fromLeaf, toLeaf)` runs once after a complete
transition, with no synthetic records for parent entry. It is synchronous and
may throw through the normal cleanup boundary. Actor controller logs retain up
to 32 records with leaf endpoints, declared source/target, condition, transition ID
and simulation time. Every group and leaf still needs a valid behavior binding;
all share the same per-agent blackboard. No scoped blackboard shadowing is added.

## Graphical editor

In **Details → AI state machines → Open AI graph editor**, choose
**Hierarchical template** for an Alive/Patrol/Combat/Chase/Attack/Retreat/Dead graph.
The template uses ordinary `idle`/`move`, `always` and `flag` bindings.

- **Parent group** reparents a selected state/subtree. Descendants are excluded
  from the choices, and portable validation independently rejects cycles. The
  first child becomes an initial child; moving/deleting the initial child picks
  another remaining direct child. Empty groups become leaves. Moving the graph
  entry under another group chooses an available root entry.
- **Initial child** selects a group's direct entry. **Set initial child** on a
  child sets its parent's entry; **Set entry** on a root sets graph entry.
- The node's **+ / −** or **Expand/Collapse children** controls toggle descendants.
  Expanded groups show boundaries; edges to hidden descendants use their visible
  enclosing group. **Open group** or double-click enters that group, and breadcrumb
  buttons navigate back. The selection list shows full paths and reveals a selected
  state's ancestors. Cross-scope edges remain editable through that list.
- Dragging a group moves its subtree as one validated history command. Canceling a
  drag changes no document state. Deleting a group removes its descendants and
  incident transitions as one undoable command. Deleting the entire graph rejects.
- Collapse, focus, breadcrumbs and zoom are transient presentation choices; they
  do not alter runtime execution or graph history. Undo/Redo/files retain authored
  parent/initial-child relationships and positions.

Standalone preview highlights the active ancestors/leaf and displays the full
path. Its condition checkboxes remain manual: enable `always` to advance the
template and `flag` to test the parent interrupt. Authoring is gated during preview.

Assign the open graph to an ordinary actor with a suitable motor. In the actor
binding forms, source `flag.value` from the `dead` blackboard field. During Play,
the live debugger displays e.g. `Alive → Combat → Chase`, highlights the active
chain and the exact accepted edge, and exposes the existing live inputs. Setting
`dead` true interrupts the group and stops child movement. Source authoring/history
remain unchanged. The original patrol application has its own supported callbacks;
the hierarchical template is an actor-controller example, not new patrol policy.

## Files and remaining scope

Graphs now write `WEAI 5`, retaining parent/initial-child presence plus IDs. Readers
accept versions 1, 2, 3, 4 and 5; older files load as flat graphs. The 512 KiB graph limit
is unchanged. Controller payload `AIC 2` embeds the new graph and keeps reading
`AIC 1`; the outer custom codec remains version 1. Scene files and isolated Play
preserve hierarchy/configuration and start fresh runtime paths/timers/default
blackboard values. There is no core scene-archive format bump for this optional
component. Deploy matching browser scripts and Wasm together.

Parallel regions, history states, internal transitions,
visual action scripting and runtime checkpoints remain future work. See
[state-machine contracts](state-machines.md), [controllers](ai-controllers.md)
and selected engine docs/ai-hierarchy-validation.md.

[Typed event queues](ai-events.md) now share parent/source transition priority. Activation-scoped inputs survive retained ancestors but cancel when their state exits; unscoped occurrences use explicit FIFO/budget semantics.

## AI task sequences

AiState now carries <=16 ordered AiTaskSpec entries with registered keys, exact typed parameters, compatible blackboard sources, timeout (0,86400] default30, optional observation event and zero-payload succeededEvent/failedEvent names. AiBindingRegistry::create accepts an optional third vector of up to128 AiTaskBinding factories. Each controller owns fresh AiTask instances; start(controller,spec,activationScope) snapshots inputs, tick returns Running/Succeeded/Failed, and cancel noexcept releases work on every terminal/exit path. At most one task ticks per active state per fixed step; callbacks run only on the scene owner. Scoped terminal events defer through the existing cutoff, retry once per later update while the queue is full and cancel on exit. Retained ancestors retain tasks. Controller stop from callbacks defers teardown safely; configure/restart/source replacement cannot replace executing ownership.

Built-ins: wait(seconds), wait-event(spec.event), move-to(target Vector/speed/distance) via existing Navigator and explicit transient setNavigationSource(source) followed by restart(), face-target(target Vector,+Z facing). Motor/pose tasks require an existing motor, and competing active AI writers reject. Source revision invalidation remains authoritative; no automatic bake/ground/animation subscription. Application animation factories use scoped component handles and own/cancel subscriptions. See [task contracts](ai-tasks.md).

Graph task forms add/reorder/remove tasks, typed constants/source mapping, timeouts and result events with one Apply history edit. Actor Play live tasks show index/key, elapsed, result, reason and pending delivery, with <=32 history records. Standalone manual-guard preview explicitly leaves task execution to Actor Play/custom hosts. WEAI5 reads1..5 inside unchanged AIC2/outer codec1; objects, progress, providers/subscriptions and history stay transient. Run ai_tasks plus state_machine/ai_controller/ai/editor/editor_structure/application/scene_archive checks and scripts/editor-ai-tasks-smoke.cjs on WebGPU/WebGL2 at DPR1/2; preserve exact artifacts and report any concurrent-suite failures.
