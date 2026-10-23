# Race winnings and garage

The Midnight Circuit pays credits and saves separate wheeled and hovercraft
upgrade builds. Vektor and Bastion share wheeled upgrades; Skimmer and Vector
share hover upgrades. New careers start with all four chassis, their base pulse cannon
and shield, and zero credits. There is no entry fee.

## Earn and spend

| Final position | Credits |
| --- | ---: |
| 1 | 1,200 |
| 2 | 900 |
| 3 | 700 |
| 4 | 500 |
| 5 | 400 |
| 6 | 300 |

The final classification, including recovery penalties, determines the payout.
All six racers must finish. Results show pending winnings until then; race-again
and garage commands wait for classification. A completed race pays once. Restarting,
abandoning, or reloading an unfinished race pays nothing. Races themselves do not resume.

Select a vehicle in the garage to view and buy its upgrades. Each purchase installs
the next level immediately. Prices, eligibility and debits are enforced in C++;
disabled buttons explain insufficient funds or a completed upgrade path.

| System | Level 1 / 2 / 3 price | Effect per level |
| --- | --- | --- |
| Drive | 400 / 700 / 1,100 CR | +10% base drive and boost force/torque; +2 m/s forward throttle limit |
| Pulse cannon | 300 / 550 / 900 CR | 0.1 seconds shorter firing interval, from 0.8 to 0.5 seconds |
| Energy shield | 300 / 550 / 900 CR | +25 energy capacity and +3 energy/second recharge, from 100 / 18 to 175 / 27 |

Drive percentages add to the base, rather than compounding. The forward limit
controls throttle taper/cutoff, not an absolute velocity clamp. Reverse, braking,
steering, pulse range, disruption duration, shield drain and block cost retain
their base tuning. AI opponents retain base equipment. Upgrades cannot be bought
during a race, including while paused. Switching classes never transfers upgrades.

The [imported pulse cannon](weapon-upgrade.md) shows Mk 1–4 in the garage. Each
purchased cannon rank adds a luminous indicator to both physical mounts;
Vektor/Bastion and Skimmer/Vector share their respective class's appearance and
firing upgrades.

## Saving

Credits, completed-race count, selected vehicle and all six upgrade ranks save
automatically after changes in this browser's local storage, under
`neon-ascension.career.v1`. Switching renderers preserves the career on the same
origin. Browser profiles, hostnames and ports have separate saves. Clearing site
data removes progression. There is no cloud account or cross-device sync.

`src/progression.hpp` owns prices, rewards, bounds and the strict `NA5` codec.
It wraps the original ten fields plus a validated `LC2` championship snapshot with latest/best player records;
NA1/NA2/NA3/NA4 saves migrate. Custom body paint is saved independently for each chassis; see [vehicle paint](vehicle-paint.md). See [league career](championship.md). The original NA2 layout
is retained here to explain legacy migration. The
snapshot contains ten bounded unsigned decimal integers: credits, completed
races, selected class, then drive/cannon/shield ranks for car and hovercraft,
followed by the selected chassis (Vektor=0, Skimmer=1, Bastion=2, Vector=3).
Legacy `NA1` saves migrate with all values preserved and choose Vektor or
Skimmer from the saved class. Inconsistent chassis/class pairs are rejected.
The wallet caps at 999,999,999 credits and the completed counter at 1,000,000.
`StarterScene` awards only final results and installs upgrades into the vehicle
and combat simulation. `browser_main.cpp` bridges snapshots to the host.

The browser adapter replaces one complete localStorage value synchronously;
it does not save transient race or combat state. Unknown versions, malformed data
and invalid ranks are rejected as a whole. Unreadable original saves are kept,
and the garage explicitly labels further play as session-only. Storage access
or quota failures also show session-only status rather than claiming a save.
Detected changes in another tab stop writes from the older tab and ask for a
reload, avoiding ordinary stale-tab overwrites. This is a local prototype save,
not a server-authoritative or tamper-resistant economy.

## Verification

See [recorded progression validation](progression-validation.md) for build
identity, race prizes, purchase/reload results and evidence paths.

`python scripts/dev.py native` includes codec/bounds, all prices and payouts,
purchase eligibility, actual acceleration comparisons, upgraded combat timing,
stock and fully upgraded physical races, delayed classification and once-only
reward checks. All four chassis use imported geometry; native chassis checks
also cover wheel bindings, selection persistence and countdown grid stability.

`scripts/prototype-smoke.cjs` checks physical races, actual earned-credit
purchases and reload persistence on both renderers. `scripts/progression-smoke.cjs`
uses explicit saved-profile fixtures to exercise all 18 purchases, installed
upgrades while driving/firing/shielding, narrow garage scrolling, new-session
restoration, renderer switching, invalid saves and storage failure. Run browser
suites serially against the same frozen generated site.
