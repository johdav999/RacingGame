# Cyberpunk racing game design

Working title: **Neon Ascension**

An arcade combat racer set in the megacities of 2189. Players begin with a
reconditioned street car or hovercraft, earn credits in dangerous city races,
and build a machine capable of challenging the corporate racing elite.
Wheeled cars and low-flying hovercraft compete on the same circuits. Weapons,
defensive systems, driving skill, and garage decisions all influence victory.

This is the proposed full game design. The first six-racer AI prototype is now
implemented; see [its delivered scope](prototype.md). The wider career, combat
and content systems below remain planned features. Numerical values are initial
tuning targets.

## Player experience

The defining moment is a contested overtake: slide through a rain-soaked market
corner, break an opponent's missile lock behind a support pillar, redirect
reactor energy into boost, and cross the line with a damaged vehicle and enough
prize money for the next upgrade.

Three principles guide the game:

- Driving creates opportunities. Racing lines, braking, drafting, and route
  choices must remain the strongest foundations for success.
- Combat changes position. A well-timed attack or defense should create a
  passing opportunity with a readable response for the other racer.
- A vehicle becomes personal. Its silhouette, handling, equipment, damage,
  and history should reflect how its owner races.

Start with a single-player career against AI and standalone events. Design
around keyboard and controller play on desktop browsers. Multiplayer and
touch controls are later scope.

## Setting and identity

Racing began as an underground courier competition through abandoned transit
routes. Corporations turned it into a broadcast sport, but neighborhood crews
still maintain the circuits and challenge the sponsored teams.

The player inherits a small garage above a failing charging station. A mechanic
helps restore the first vehicle, while a former league racer becomes the
player's spotter. Progress moves from local respect to sponsorship offers and
eventually a championship against the corporations controlling the sport.

Cyberpunk should shape the world beyond its lighting: wealthy towers above
flooded streets, improvised machines competing with expensive prototypes,
pirate broadcasts, corporate ownership of public infrastructure, and crews
repairing equipment that manufacturers have abandoned.

Story arrives through brief garage conversations, rival introductions, race
radio, and changes in the home district. Repeated races allow dialogue skips.
Sponsor choice affects liveries, side objectives, and dialogue; it never blocks
essential equipment.

## Career loop

1. Select an event and inspect its track, rewards, restrictions, and rivals.
2. Prepare a legal vehicle build and test it on the garage practice circuit.
3. Race for finishing position, championship points, and optional objectives.
4. Receive credits, reputation, and any earned unlocks.
5. Buy or tune equipment, customize the vehicle, and enter the next event.

Typical races last four to six minutes. Garage changes should take less than
two minutes when the player already knows what to buy. Save after rewards,
purchases, and championship results so a career can be played in short sessions.

## Driving and vehicle classes

Handling is immediate and expressive, with believable momentum rather than
simulation complexity. Use a chase camera with adjustable distance, camera
shake, and speed effects.

| Property | Wheeled cars | Hovercraft |
| --- | --- | --- |
| Character | Grip, weight transfer, controlled drift | Lateral glide, momentum, air braking |
| Cornering | Strong braking and fast grip recovery | Earlier setup with sustained sideways motion |
| Strength | Stable contact and efficient tight corners | Smooth travel over rough surfaces and flowing bends |
| Weakness | Rough ground disrupts traction | Side impacts displace the vehicle more easily |
| Mastery | Brake timing, drift exits, traction control | Countersteering, air brake timing, slide recovery |
| Visual identity | Armored coupes, exposed motors, wide tires | Lift pods, stabilizer fins, luminous thrusters |

Hovercraft follow the track at a controlled height. They can jump across marked
gaps but cannot fly freely, bypass checkpoints, or escape into the city skyline.
Both types use steering and an optional slide control; hovercraft do not require
extra vertical controls.

Both classes can use every championship route. Class advantages come from
handling and surface response. Alternate routes remain available to both, with
different execution demands. Each circuit combines technical and flowing
sections so one class does not receive a universal advantage.

Boost draws from a reactor battery. The battery regenerates over time, with a
modest additional gain from sustained close drafting. It cannot be farmed by
stationary sliding or repeatedly crossing a trigger.

Collisions scrub speed and can cause damage, but slight contact should not
produce uncontrollable spins. Off-track recovery returns the racer to the last
valid safe position with a time cost and no checkpoint advancement.

## Race rules and modes

The standard field is eight racers. Finishing order uses completed laps,
ordered checkpoints, and progress along the current valid track segment.
Weapons become active three seconds after the start.

| Mode | Rules | Purpose |
| --- | --- | --- |
| Circuit | Three laps with combat enabled | Main career competition |
| Sprint | One longer route with combat enabled | City traversal and spectacle |
| Time trial | Solo, weapons disabled, fixed build leaderboard category | Driving practice and mastery |
| Clean circuit | Standard circuit with offensive systems disabled | Optional driving-focused events |
| Elimination | After a grace period, last place is removed at announced intervals | Later high-pressure events |

Main career progression uses circuits and sprints. Other modes provide variety
without becoming mandatory promotion requirements.

Normal races have no separate score for damage or takedowns. Racing forward
must remain more profitable than repeatedly attacking a weak opponent.

## Cities and circuits

The full career vision contains five cities, each with three core routes.
Routes reuse recognizable landmarks while changing the actual driving
challenge. Reverse routes are optional additions and do not count toward that
target.

| City | Identity | Track features | Driving challenge |
| --- | --- | --- | --- |
| Kairox | Dense neon markets beneath residential towers | Narrow service roads, station tunnels, market plazas | Braking and precise overtakes |
| Vanta Port | Rain, shipyards, flood barriers, container terminals | Dock straights, industrial switchbacks, canal bridges | Drafting and contact management |
| Helix Heights | Corporate towers and elevated transport networks | Skyways, sweeping ramps, transparent tunnels | High-speed commitment |
| Cinder Ward | Refineries and districts rebuilt around heavy industry | Rough service lanes, cooling plants, broad factory yards | Surface choice and defensive positioning |
| Aurora Crown | Wealthy civic center above the old city | Boulevard starts, tower descents, underground final sectors | Combining every learned skill |

Each track needs at least two reliable overtaking zones, one technical section,
one place where scenery can break weapon lock, and a clearly signposted
optional route. Shortcuts trade distance against difficulty, exposure, or speed
loss. None should be both easier and faster in ordinary conditions.

Event-specific hazards follow visible, repeatable schedules announced before
entry. Machinery must warn before crossing a track. Ordinary crowds remain
behind barriers; civilian collision is not a scoring mechanic.

Road edges, braking references, and opponents must remain readable beneath
neon signage and rain. Decorative traffic occupies separated lanes.

## League progression

The following original production design is superseded for the playable six-racer
career by [the implemented championship contract](championship.md): Street,
District, Metro and Elite leagues lead to a six-round Master Championship;
points are 15/12/10/8/6/4. The city-specific eight-racer/tier-5 design below remains
future direction, not implemented content.

Each league consists of four championship races followed by a separate final.
Points for the eight places are 15, 12, 10, 8, 6, 4, 2, and 1. The top three in
the four-race standings qualify for the final. Ties use race wins, then the most
recent race result. Winning the final earns promotion.

A failed final can be retried without repeating the season. A failed season can
be replayed, and all earned money and equipment are retained. Previously
unlocked events remain available. There is no automatic demotion.

| League | Primary setting | Equipment ceiling | New emphasis |
| --- | --- | --- | --- |
| Street Circuit | Kairox | Tier 1 | Learn the vehicle, basic weapons, and defenses |
| District League | Vanta Port | Tier 2 | Tradeoffs, rival styles, and sponsorship |
| Metro League | Helix Heights | Tier 3 | Sustained speed and route planning |
| Corporate League | Cinder Ward | Tier 4 | Specialized opponents and resource management |
| Apex Championship | Aurora Crown and selected earlier cities | Tier 5 | A complete build and consistent execution |

Entry uses explicit part-tier limits and a shared fitted-power budget. The
garage displays the exact part or capacity that makes a build illegal and
offers a reversible legal preset. A single overall rating is insufficient to
police specialist weapons and defenses.

League promotion unlocks equipment for purchase; it does not require buying
every item in the preceding tier. Each starter chassis remains upgradeable
through the career.

## Economy and ownership

Credits buy vehicles, performance parts, weapons, defenses, and cosmetics.
Reputation unlocks league invitations and sponsor opportunities. Reputation
cannot be spent, and credits cannot buy promotion.

Players choose either a starter car or hovercraft free of charge. Both include
a basic pulse weapon and shield. The other starter chassis becomes an early
purchase, allowing players to try both without restarting their career.

An initial Street Circuit economy:

| Item or result | Credits |
| --- | ---: |
| Completed race stipend | 500 |
| First place bonus | 1,000 |
| Second place bonus | 700 |
| Third place bonus | 500 |
| Fourth through eighth place bonus | 350 / 250 / 150 / 100 / 0 |
| Optional sponsor objective | 0 to 250 |
| Basic performance upgrade | 1,500 to 2,500 |
| Alternative Tier 1 weapon or defense | 2,000 to 3,500 |
| Other starter chassis | 6,000 |
| First promotion bonus | 4,000 |

For example, a third-place finish pays 1,000 credits before optional objectives;
two such finishes fund a 2,000-credit handling upgrade. Target one meaningful
early purchase every two to four completed races.

League reward and price bands grow together. Later equipment should not demand
dozens of repeated low-level events. Sponsor objectives reward forward racing,
such as clean overtakes or finishing with an intact shield, with at most one
payout per objective per event.

Basic repairs and ammunition restocking between races are free. Event entry
has no fee. Defeat cannot leave the player unable to compete. Used parts sell
for 70 percent of their purchase price, and owned parts can be moved freely
between compatible builds. Show compatibility and resale value before purchase.

Use earned currency for all gameplay power. Real-money vehicle power, loot
boxes, and energy timers are outside the design.

## Garage and upgrades

Every vehicle has a chassis, drivetrain or lift drive, handling system, reactor,
armor, and cooling system. Three equipment hardpoints support one forward
weapon, one rear or utility weapon, and one defensive module.

The full-career target is ten chassis, five per vehicle class. Chassis offer
different handling and capacity distributions rather than strictly replacing
older vehicles.

| Upgrade area | Benefit | Cost or tradeoff |
| --- | --- | --- |
| Motor or thrusters | Acceleration and top speed | More heat and fitted power |
| Tires or stabilizers | Grip, slide recovery, turning control | Specialist setups sacrifice another handling quality |
| Reactor | Battery capacity or regeneration | Mass and reduced capacity for other high-power parts |
| Armor | More hull protection | Mass reduces acceleration and agility |
| Cooling | Faster heat recovery | Capacity that could support stronger offensive equipment |
| Weapon | Different attack behavior or improved output | Heat, ammo, mass, and fitted power |
| Defense | Protection against selected threats | Activation energy and cooldown |

Upgrade tiers raise the available ceiling while tuning choices remain
meaningful. A missile specialist should give up something tangible to carry
heavy ammunition and a stronger reactor.

The build screen shows acceleration, top speed, handling, hull, battery, heat
recovery, fitted power, and mass, with before-and-after comparisons. Show a
plain-language explanation such as "Stronger armor; slower acceleration."
Allow free test drives and store three named loadouts per vehicle.

Cosmetics include paint, decals, underglow, wheel or lift-pod trim, and boost
trails. Team colors and incoming-threat indicators must remain distinguishable
from decorative effects.

## Weapons and defenses

Weapons are attached in the garage and visible on the vehicle. There are no
random weapon pickups. Track knowledge and the selected loadout determine the
options available during a race.

Prototype balance assumes a 100-point hull, a 100-unit battery, and a heat
meter from 0 to 100. Boost and active defenses consume battery; energy weapons
produce heat. Heavy weapons carry finite ammunition for the whole race.
These separate limits prevent continuous attacks while letting players reserve
energy for an escape.

| Offensive module | Use | Limitation | Opponent response |
| --- | --- | --- | --- |
| Pulse repeater | Aim short forward bursts to pressure a nearby rival | Modest damage and accumulating heat | Change line, use cover, or shield |
| Seeker missile | Threaten an exposed racer ahead | Six rounds per race; one-second visible lock | Break line of sight, deploy decoy, or shield |
| EMP lance | Deny boost and energy-weapon use briefly at close range | Short range, long cooldown, low hull damage | Stay outside range or activate shield before impact |
| Arc mine | Place a visible trap on the racing line behind | Four charges; one-second arming time; short lifetime | Dodge or clear with pulse fire |
| Disruptor cloud | Interrupt lock-on pursuit through a short rear cloud | Three charges and minimal damage | Change route or wait for a clear lock |

EMP never removes steering or brakes. Its initial disable duration is 1.25
seconds. Disruptor effects cannot obscure essential track cues.

| Defensive module | Use | Limitation |
| --- | --- | --- |
| Barrier shield | Absorb up to 30 damage and reject EMP for two seconds | Costs 35 battery; 12-second cooldown |
| Decoy burst | Divert missiles already targeting the vehicle | Costs 20 battery; eight-second cooldown; no protection from pulse fire or mines |
| Repair nanites | Restore 20 hull across four seconds | Costs 40 battery; two uses per race; incoming damage interrupts healing |
| Stability anchor | Resist collision displacement and weapon impulse briefly | Costs 25 battery; ten-second cooldown; does not prevent hull damage |

Passive armor remains available regardless of the chosen defensive module.
Teach unassisted counterplay before expecting the player to own a specific
defense.

At maximum heat, energy weapons lock until heat falls below a recovery
threshold. Driving and defensive activation remain available. The HUD warns
before overheating, and repeated button presses cannot extend or bypass it.

Initial single-hit damage should remain below one-third of a fresh hull.
Disables cannot stack: after an EMP ends, the target receives three seconds of
EMP immunity. Threats need visual direction, audio cues, and optional haptics.

At zero hull, the vehicle is disabled and recovered at a valid safe point after
approximately three seconds. It returns with 60 hull and two seconds of visible
protection, during which it cannot fire or physically block other racers.
Recovery never replenishes ammunition, charges, or battery, and never advances
track progress. Standard races permit recovery; elimination removes racers
through its announced last-place rule.

## Example builds

| Build | Chassis and equipment | Intended approach | Weakness |
| --- | --- | --- | --- |
| Street duelist | Balanced car, pulse repeater, arc mine, barrier | Hold a precise line and defend a narrow lead | Limited reach on long straights |
| Ghost runner | Agile hovercraft, pulse repeater, disruptor cloud, decoy | Preserve momentum and escape missile pressure | Light armor and weak contact resistance |
| Siege driver | Heavy car, seeker missiles, arc mine, repair nanites | Survive crowded packs and force openings | Slow exits and finite ammunition |
| Interceptor | Fast hovercraft, EMP lance, disruptor cloud, stability anchor | Close the gap and deny a rival's boost at a passing zone | Must expose itself at short range |

## Rivals and difficulty

Named rivals have recognizable equipment and habits: a courier who rarely
brakes, an armored factory champion, a precise corporate missile specialist,
and a veteran who saves their shield for the final lap.

AI follows the same equipment budgets, ammunition, cooldowns, checkpoints, and
recovery rules as the player. It uses the same vehicle simulation. Difficulty
changes planning, reaction delay, aim error, and tolerance for risky overtakes.
The game should not secretly increase a rival's top speed because the player
is ahead.

Difficulty can be changed between events without losing career progress.
Optional steering assistance, stronger aim assistance, and race restarts support
learning. Named rival rematches offer cosmetic rewards and additional dialogue.

## Interface and accessibility

The race HUD prioritizes position, lap, next turn or checkpoint, speed, hull,
battery, heat, and the three equipped actions. Incoming missile or EMP warnings
appear near the vehicle with a clear direction and countdown where applicable.
Cooldowns and unavailable actions explain their state.

The garage flow is event selection, build preparation, optional test drive,
then race. Event entry and purchase controls remain visible while equipment
lists scroll. Invalid builds point directly to the necessary change.

Default keyboard controls use WASD or arrows for driving, Space for drift or
air brake, Shift for boost, and three remappable keys for forward weapon,
rear weapon, and defense. Controller controls use triggers for throttle and
braking, a stick for steering, and remappable buttons for slide, boost, and
equipment. All actions work without a mouse during racing.

Provide adjustable camera movement and field of view, reduced flashing,
motion-blur controls, subtitles, scalable HUD text, and shape-coded warnings.
Color alone must not distinguish danger, equipment state, or racing routes.
Single-player races pause when the player deliberately pauses; background-tab
behavior must protect the player from an unseen ongoing race.

## Visual and sound direction

Cars combine low street-racing silhouettes with exposed charging rails,
reinforced panels, and visibly mounted equipment. Hovercraft use distinct lift
pods and stabilizers so their movement is understandable from a distance.
Damage changes bodywork, sparks, and motor sound without hiding brake lights
or weapon cues.

Give each city a specific palette and material character: Kairox mixes warm
shop lighting and magenta signs; Vanta uses cold rain and amber machinery;
Helix uses pale corporate surfaces and controlled cyan lighting; Cinder uses
oxidized metal and furnace orange; Aurora uses gold-white architecture above
dark infrastructure.

Music combines electronic percussion, industrial bass, and melodic synth
layers. Intensity follows race phase and nearby competition. Tires, lift
engines, boost, lock-on warnings, and defensive activation need distinct sound
signatures. Radio calls stay brief during demanding sections.

## First playable and release scope

Build the game in stages so driving and combat can be evaluated before
committing to the full city roster.

| Stage | Playable scope | Completion criteria |
| --- | --- | --- |
| Driving prototype | One greybox circuit, one car, one hovercraft, solo laps | Both classes finish valid laps, recover reliably, and feel distinctly controllable |
| AI race prototype (current) | Same circuit, player car or hovercraft, five physical AI rivals, live positions and results | All six racers finish valid laps; pause, restart and recovery work for the field on both renderers |
| Combat prototype | Same circuit, eight racers, pulse repeater, seeker, mine, barrier, decoy | Full races resolve correctly; threats have usable counters; no stun chains or recovery exploits |
| Career slice | One finished Kairox track, two starters, three events, a rival final, basic upgrades | Race rewards, purchases, saving, reload, and promotion work as one complete loop |
| Initial release | Kairox, three circuit layouts, four chassis, two leagues, circuit and time trial modes | A complete short career with balanced builds and readable browser presentation |
| Full career expansion | Five cities, ten chassis, five leagues, additional modes and equipment | Sustained progression with distinct tracks, rivals, and viable vehicle classes |

The initial release uses Street and District leagues within Kairox. Vanta and
the full geographic progression arrive with the larger career; the short
release still ends with its own championship and credits sequence.

Multiplayer, an open-world city, free flight, vehicle trading, procedural tracks,
and destructive city environments are outside the initial release.

## Validation and implementation boundaries

Playtests should establish whether beginners can finish a race, understand
why they lost position, and choose a useful upgrade without external guidance.
Use lap times, overtakes, combat-induced delays, weapon usage, and spending
patterns alongside player feedback.

Initial acceptance targets:

- New players can finish the first event and buy a useful upgrade within three
  completed races.
- Comparable car and hovercraft builds have clean-lap times within roughly
  three percent across the starter track set, while favoring different sectors.
- Combat creates passing opportunities without routinely removing control or
  making the fastest clean driver irrelevant.
- Every offensive module has a practical counter available in the same league.
- Finishing last repeatedly still funds progression; a failed championship
  never requires selling the starter vehicle.
- Save and reload preserve owned equipment, credits, loadouts, and standings.
  Applying a completed race result twice cannot award money twice.
- The generated browser game is checked in both WebGPU and WebGL2. Target
  smooth 60 fps on agreed desktop reference hardware, with scalable effects;
  this is a performance goal rather than a current measured result.

Keep career rules, vehicle tuning, tracks, equipment definitions, economy, and
UI in RacingGame. Inspect the pinned WebEngine before relying on engine
features. Reusable engine additions must be developed and published in
WebEngine before the game selects the new submodule commit.

The first implementation milestone is the driving prototype. Prove that one
car and one hovercraft are fun on the same track before expanding the garage,
career, or city assets.
