# AI state machines and graphical authoring

Link `engine_ai` and include `engine/state_machine.hpp`. Immutable
`StateMachineDefinition` assets describe states, named behavior bindings and
ordered conditional transitions. Each agent owns a `StateMachine` instance and
supplies its game context through `StateMachineHost`; neither definitions nor
the runtime own scenes, actors, renderers, browser input or callback registries.

## Runtime integration

```cpp
engine::StateMachineData data;
data.initial = 1;
data.states = {{1, "Idle", "idle", {80, 80}},
               {2, "Follow", "follow", {380, 80}}};
data.transitions = {{1, 1, 2, "target-visible", 0},
                    {2, 2, 1, "target-lost", .5}};
auto definition = engine::StateMachineDefinition::create(std::move(data));
engine::StateMachine machine(definition);
```

Implement `StateMachineHost::enter`, `update`, `exit` and `condition` in the
application. Bind `AiState::behavior` to game actions and condition keys to
perception, memory or navigation outcomes. Keys are names, never executable code.
Use generation-checked `ActorId`/`ComponentId` references for game targets; resolve
them through their owning scene. Definitions contain no target handles.

An application actor can inherit both `Actor` and `StateMachineHost`:

```cpp
void fixedTick(double dt) override {
    updatePerception(dt);              // Game code; DecisionClock can bound queries.
    machine.step(*this, dt);           // This actor implements StateMachineHost.
    navigator.step(motor(), dt);       // Before child CharacterMotorComponent ticks.
}
void enter(const engine::AiState& state) override {
    if (state.behavior == "follow")
        navigator.navigateSource(navigationSource, lastSeenPosition);
}
void exit(const engine::AiState&) noexcept override {
    if (auto* motor = getComponent<engine::CharacterMotorComponent>())
        navigator.cancel(*motor);
}
bool condition(std::string_view key) override {
    return key == "target-visible" && targetVisible;
}
```

Call `setTickEnabled(true)` on the actor. Stop the machine with its host on
disable/detach/despawn, target/control handoff or before destroying the game
context. Guard component lookups during deferred destruction: an already
pending-removal motor may no longer resolve. Cancel persistent movement explicitly
where needed. One producer owns motor commands; do not reissue navigation every
frame merely because a state remains active.

### Execution and lifetime contract for polled flat graphs

- Instances start Idle. First `step(host, dt)` enters the initial state, advances
  its timer and calls update once. Eligible transitions are evaluated afterward.
- `minimumSeconds` measures simulation time in the current state. Transitions
  are checked in stored vector order; the first true condition wins. A transition
  runs old exit, resets the timer, then new enter. The destination's update runs
  on the next step, not once for every transition in the same step.
- Defaults are one transition and 1024 condition evaluations per step. Transition
  limits are 1–64; condition limits are 1–65536. Cycles/self-transitions are valid
  but defer at the transition bound. Exhausting the condition allowance fails with
  cleanup and throws. These are work limits, not a frame-time guarantee.
- Accept finite `dt` in `(0, .25]`. Pause by not stepping. Authoring and graph
  presentation never advance a game instance. Agents own their independent timers.
- `stop(host)` exits at most once and remains Stopped on later steps.
  `restart(host, definition, initialId)` validates the replacement before exiting
  the old state, resets the instance and enters the requested state next step.
- Enter/update/condition failures exit the active (possibly partially entered)
  state once, mark Failed and rethrow. Later steps stay inert until explicit
  restart. Exit must be `noexcept`. Callback reentry is rejected; do not call
  step/stop/restart recursively from callbacks, including exit.
- Hosts are synchronous, non-retained references. Keep the same owning game
  context while running and explicitly stop before it disappears. The runtime
  introduces no worker, subscription, scheduler or implicit scene phase.

Definitions validate before publication: 1–256 states, up to 1024 transitions,
unique state/transition IDs, existing entry and endpoints, finite positions within
±10000, finite delays 0–86400 seconds and 1–256 byte labels/bindings without control
characters (bindings also exclude spaces). Unreachable states and cycles are
allowed authoring choices. Bindings are validated by the consuming application;
the generic definition cannot infer which game callbacks exist.

## Graphical engine editor

Open `?editor=1`, then **Details → AI state machines → Open AI graph editor**.
The graph workspace is an integrated modal tool, with its own asset document and
history. It uses the same C++ definition validation and runtime as game code.

- Add/select states, edit names and behavior bindings, and choose the entry state.
- Drag nodes to arrange them. A completed drag is one undo command; Escape,
  pointer cancellation, blur and page hiding cancel unfinished gestures. Arrow
  keys move a selected state; the selection dropdown also supports keyboard use.
- Select a source state, click **Connect states**, then click its destination.
  Select the connection or its dropdown entry to edit the condition and minimum
  state time, change priority or delete it. Deleting a state removes its incident
  connections and selects another entry if needed; the last state cannot be deleted.
- **Undo/Redo** affects the AI document only. Template replacement is undoable.
  **Frame graph** and zoom change presentation without editing the asset/history.
- **Save .weai** exports the definition; **Open .weai** validates a complete file
  before replacing the document and starting clean history. Invalid fields/files
  retain the last valid graph. Closing/reopening the tool retains its in-memory
  document. Save the AI file separately to retain it across page reloads.

### Isolated runtime preview

**Start preview** creates a separate `StateMachine` over the current immutable
definition. Named-condition checkboxes stand in for game observations. **Step
0.1 s** advances once; **Run/Pause** advances the preview's own bounded clock.
The active node, elapsed state time and enter/exit counts are shown. Stop before
authoring; definition-changing controls are disabled during preview, and portable
bridge commands enforce that gate. Closing the tool, leaving the editor or
starting scene Play stops the preview. Blur/hiding pauses automatic preview time.

Preview resolves conditions supplied by the checkboxes and does not execute game
behavior bindings. It validates graph flow through the actual C++ runtime without
moving actors or changing scene clocks, bytes, selection or scene undo history.

### Running an authored graph in the demo

The patrol template is application data in `src/ai_demo.cpp`, outside reusable
libraries. The autonomous demo now uses `StateMachine` for Patrol, Follow,
Investigate, Return and Blocked behavior; manual click-to-move explicitly hands
control away from the state machine and retains navigation recovery.

**Use in AI demo** validates bindings before retaining the graph for the current
application session. Close the graph, exit the scene editor, then open the AI demo.
The supported behavior keys are `patrol`, `follow`, `investigate`, `return` and
`blocked`. Supported condition keys are `target-visible`, `target-lost`,
`target-removed`, `memory-expired`, `arrived` and `navigation-failed`.
Renamed states, graph entry, connections, priorities and minimum times come from
the authored definition. Unknown demo bindings reject without replacing valid
configuration. Other games implement their own host and supported binding set.

## Portable authoring and files

`engine_editor` additionally links `engine_ai` and exposes
`engine/editor_ai.hpp`. `AiGraphEditor` owns immutable graph snapshots, bounded
history (default 128 edits; configurable 1–1024), undo/redo and a saved identity.
`edit(data)` validates and prepares a candidate before replacing state; identical
content is a no-op. `open(bytes)` parses before replacing document/history;
`markSaved()` tracks export state independently of scene files.

`StateMachineArchive::save/load` writes version-5 `.weai` text with header `WEAI 5` (also reads v1-v4),
stable IDs, graph/state names, bindings, layout, transition order and delays.
Loads reject unsupported versions, invalid counts, orphan identities, malformed
values, truncated/trailing input and files over 512 KiB. This format contains no
live callbacks, runtime timers, agent target handles or scene resources. Applications
can load a file and pass the resulting shared definition to their instances.

`docs/ai-controllers.md` in the selected engine checkout now attach graph snapshots and typed
registered bindings to ordinary actors through scene history. Their optional custom
codec preserves configuration in scene files and isolated Play clones; a live
Play debugger displays selected controller state/time/history. Standalone graph
files remain separate authoring assets. Concurrent states, behavior
trees, automatic game callback discovery, visual action scripting, runtime AI
checkpoints and AI graph autosave remain extensions.

See `docs/ai-state-machines-validation.md` in the selected engine checkout.

Graph assets now retain field schemas/defaults; see [typed blackboards](ai-blackboards.md). Live values belong to controller instances and are not graph runtime state.

These flat-graph rules extend to parent interrupts, nested entry, ancestor timers and complete-path cancellation; see [hierarchical AI](ai-hierarchy.md).

[Typed event queues](ai-events.md) now add event triggers, activation-scoped cancellation and payload diagnostics to the same runtime.

## AI task sequences

AiState now carries <=16 ordered AiTaskSpec entries with registered keys, exact typed parameters, compatible blackboard sources, timeout (0,86400] default30, optional observation event and zero-payload succeededEvent/failedEvent names. AiBindingRegistry::create accepts an optional third vector of up to128 AiTaskBinding factories. Each controller owns fresh AiTask instances; start(controller,spec,activationScope) snapshots inputs, tick returns Running/Succeeded/Failed, and cancel noexcept releases work on every terminal/exit path. At most one task ticks per active state per fixed step; callbacks run only on the scene owner. Scoped terminal events defer through the existing cutoff, retry once per later update while the queue is full and cancel on exit. Retained ancestors retain tasks. Controller stop from callbacks defers teardown safely; configure/restart/source replacement cannot replace executing ownership.

Built-ins: wait(seconds), wait-event(spec.event), move-to(target Vector/speed/distance) via existing Navigator and explicit transient setNavigationSource(source) followed by restart(), face-target(target Vector,+Z facing). Motor/pose tasks require an existing motor, and competing active AI writers reject. Source revision invalidation remains authoritative; no automatic bake/ground/animation subscription. Application animation factories use scoped component handles and own/cancel subscriptions. See [task contracts](ai-tasks.md).

Graph task forms add/reorder/remove tasks, typed constants/source mapping, timeouts and result events with one Apply history edit. Actor Play live tasks show index/key, elapsed, result, reason and pending delivery, with <=32 history records. Standalone manual-guard preview explicitly leaves task execution to Actor Play/custom hosts. WEAI5 reads1..5 inside unchanged AIC2/outer codec1; objects, progress, providers/subscriptions and history stay transient. Run ai_tasks plus state_machine/ai_controller/ai/editor/editor_structure/application/scene_archive checks and scripts/editor-ai-tasks-smoke.cjs on WebGPU/WebGL2 at DPR1/2; preserve exact artifacts and report any concurrent-suite failures.
