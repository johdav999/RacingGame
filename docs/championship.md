# League career and Master Championship

The garage's **League career** opens a persistent five-level ladder:
Street League, District League, Metro League, Elite League, Master Championship.
Each event uses the existing six named racers and three-lap physical race rules.
Kairox and Desert Solar alternate; odd-numbered league indices start in the desert.
These are shared existing tracks, not five new environments.

## Competition rules

The first four leagues have four points rounds. Top three in the final standings
qualify for a separate promotion final. The final keeps the six-racer field;
qualification controls the player's entry, and winning promotes the player.
A failed final can be retried without repeating the four rounds. Outside the top
three, start a new season. Already earned credits, upgrades and titles are kept.
Previously unlocked leagues remain playable and there is no demotion.

Master Championship has six points rounds with no separate final. First in total
standings wins the Master title. A lost season can be replayed.

| Finishing position | 1 | 2 | 3 | 4 | 5 | 6 | DNF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Points | 15 | 12 | 10 | 8 | 6 | 4 | 0 |

Ties compare total points, number of wins, then seconds through sixths, then the
latest differing round result (DNF last), then stable racer ID. Finals award normal
race credits but do not add season points. DNF is supported by the classification
contract; the current physical race waits for all six finishers and has no new
retirement button or automatic timeout.

| League | Rival difficulty | Equipment rank ceiling |
| --- | --- | ---: |
| Street | Easy | 0 / stock |
| District | Normal | 1 |
| Metro | Normal | 2 |
| Elite | Hard | 3 |
| Master | Hard | 3 |

Owned player upgrades are temporarily capped to the ceiling during league events;
rivals receive ceiling equipment. Purchases are optional and stay owned. Garage
and ordinary races restore the player's full upgrades and selected difficulty.
Quick races and time trials never change championship points.

## Results, continuation and saving

Points commit only when all six racers have final classifications, together with
the existing once-only race payout. The results screen opens championship standings
in the garage. Standings show points, gap, wins and each round's finishing position.
The schedule marks completed and next rounds. Continue loads the scheduled track
when needed, opens the career panel and lets the player start it. Abandoning,
restarting or reloading an unfinished race leaves that round unscored and available.
Live driving state is not saved. Finals retain qualification and retry count.

Save codec `NA5` stores the existing wallet/chassis/upgrades/paint and `LC2` championship
in one bounded (16 KiB) snapshot at the existing `neon-ascension.career.v1` key.
`NA1`–`NA4` profiles migrate without losing credits, purchases, chassis or paint. Saved
championship state contains the current season's complete round results, final
attempts/result, contiguous earned-title bits and monotonic result revision.
The garage has a direct **Continue League** action for the saved next round or
retryable final. It loads the scheduled track and starts the event, preserving
the selected chassis. A one-use revision token prevents a stale URL from starting
an old event; unfinished races restart at the beginning of their unscored round.

**My League Results** shows current, latest and best standings per league,
including every racer's round positions, points and wins, the player's overall
position and final result/attempt count. Finished points seasons and final outcomes
are archived automatically. Switching leagues or restarting stores the current
partial run before resetting its points. Latest records can change on replay;
best records prefer a title, then completed/further-progressed seasons, better
overall position, points and final finish. There are five latest/best pairs and
one active season per profile, so storage remains bounded. A worse replay cannot
erase a best title result. Legacy saves can recover only their available current
season; earlier results already replaced by old builds cannot be reconstructed.

Malformed, inconsistent and future versions reject the whole snapshot. Existing
unreadable originals, concurrent-tab write protection and visible session-only
storage failures remain supported. Track navigation is disabled when storage is
unavailable to avoid discarding session progress. Local saves have the existing
same-origin/browser-profile limits; there is no server, cloud sync or anti-tamper claim.

## Ownership and validation

`src/championship.hpp` owns deterministic league rules, scoring, ties, schedule,
validation and codec. `Progression` owns the complete economy/career snapshot.
`StarterScene` captures a result revision, applies temporary equipment/difficulty
and records only final classifications. Points are outside disposable scenes.
The browser bridge publishes derived read-only standings; `web/championship.js`
owns presentation and scheduled page travel. No WebEngine API or pin changes.

Native: `python scripts/dev.py native` includes `racing_championship`; focused
rerun: `ctest --test-dir build/native -R '^racing_championship$' --output-on-failure`.
Browser: `python scripts/dev.py build`, serve the generated site, then run
`node scripts/championship-smoke.cjs` with `DEMO_URL` set to that server.
The smoke reports actual renderer and served Wasm hash.
`node scripts/championship-ui-smoke.cjs` additionally checks keyboard driving,
quick-race preference preservation and a labelled per-driver points presentation fixture. Controlled saved-profile
fixtures cover later league states; they do not establish naturally earned full
career balance. See the recorded validation below for the accepted report paths.
Native validation covers portable gameplay/rules, not a rendered native RacingGame
host (the selected consumer currently builds native tests and a browser game).

See [recorded championship validation](championship-validation.md) for the original career acceptance and [garage continuation and saved results validation](league-records-validation.md) for the fresh follow-up checks.
