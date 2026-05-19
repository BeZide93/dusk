# BOTW Moves Patch Checklist

## Patch 1: Bullet Time

- [x] Add a combat-time controller with a five-second Bullet Time window.
- [x] Track manual jumps started by the Dawnlight jump button.
- [x] Trigger Bullet Time while aiming the Bow during a manual jump.
- [x] Slow non-exempt actors while keeping Link and arrows running normally.
- [x] Reduce Link's fall speed during Bullet Time without slowing the bow shot animation.
- [ ] Tune slow-motion strength after device testing.
- [ ] Verify behavior with normal Bow, Bomb Arrows, and Hawkeye Bow.

## Patch 2: Flurry Rush Detection

- [ ] Add a perfect-dodge window when locked-on sidehops and backflips begin.
- [ ] Detect enemy attack collision or near-miss events during the dodge window.
- [ ] Store the dodged attacker as the Flurry Rush target.
- [ ] Start combat-time slowdown for five seconds after a valid dodge.
- [ ] Validate the generic detection against common enemies before checking bosses.

## Patch 3: Flurry Rush Attack

- [ ] Let B start a rush toward the stored Flurry Rush target.
- [ ] Reuse existing sword attack or jump-attack procs where possible.
- [ ] Keep Link exempt from slowdown while enemies remain slowed.
- [ ] End Flurry Rush on timeout, target death, scene change, player damage, or lost target.
- [ ] Add tuning for rush speed, attack cancel windows, and boss exclusions.
