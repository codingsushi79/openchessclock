# Contributing to OpenChessClock

Thanks for helping! This project is hardware, firmware and docs together, so there are lots of ways
in, and most of them don't need a finished clock.

- [Ways to help](#ways-to-help)
- [Reporting a problem](#reporting-a-problem)
- [Making a change (step by step)](#making-a-change-step-by-step)
- [Changing the firmware](#changing-the-firmware)
- [Changing the 3D model](#changing-the-3d-model)
- [Changing the docs](#changing-the-docs)
- [What CI checks](#what-ci-checks)
- [Pull request checklist](#pull-request-checklist)

## Ways to help

| You have… | You can… |
|---|---|
| 5 minutes | Fix a typo, a broken link or an unclear sentence in the docs |
| A computer | Run the [simulator](test/run_tests.sh), try firmware changes, tweak the [CAD](cad/chess_clock.scad) |
| A 3D printer | Print the parts and report fit problems ([3D model issue](https://github.com/codingsushi79/openchessclock/issues/new?template=model_issue.yml)) |
| A finished clock | File a [build report](https://github.com/codingsushi79/openchessclock/issues/new?template=build_report.yml): it's the most useful thing right now, because the firmware hasn't been run on a real Pico yet |

Issues labelled [`good first issue`](https://github.com/codingsushi79/openchessclock/labels/good%20first%20issue)
are small and well-described: a good place to start.

## Reporting a problem

Open an issue and [pick a template](https://github.com/codingsushi79/openchessclock/issues/new/choose):

| Template | Use it for |
|---|---|
| **Firmware bug** | Wrong times, buttons or lever doing the wrong thing, display glitches, compile errors |
| **3D model / print problem** | Parts that don't fit, print badly, break, or have errors in the OpenSCAD file |
| **Assembly or docs problem** | Steps that are wrong, missing or confusing; wiring mistakes; dead links |
| **Feature request** | New timing modes, presets, display layouts, hardware ideas |
| **Build report** | You built one! Tell us what worked and what you had to change |

Please search [existing issues](https://github.com/codingsushi79/openchessclock/issues) first.
Photos, simulator output and exact part numbers make issues much easier to fix.

Questions that aren't bugs? Use [Discussions](https://github.com/codingsushi79/openchessclock/discussions).

## Making a change (step by step)

New to GitHub pull requests? This is the whole process:

1. **Fork** the repository (the *Fork* button at the top right of the GitHub page).
2. **Clone** your fork and make a branch:
   ```sh
   git clone https://github.com/<your-username>/openchessclock.git
   cd openchessclock
   git checkout -b fix-short-description
   ```
3. **Make your change.** Keep each pull request to one topic: a bug fix *or* a new feature *or* a
   docs cleanup.
4. **Check it locally:**
   ```sh
   ./scripts/check_repo.py   # required files present, local links work
   ./test/run_tests.sh       # only if you touched the firmware
   ```
5. **Commit** with a message that says what changed and why:
   ```sh
   git add -A
   git commit -m "Firmware: keep the move counter when resuming from pause"
   ```
6. **Push** and open a pull request:
   ```sh
   git push -u origin fix-short-description
   ```
   GitHub will show a *Compare & pull request* button. Fill in the template, and link the issue
   it fixes (`Fixes #12`).
7. **CI runs automatically.** If a check fails, click *Details* to see why, push a fix to the same
   branch, and the pull request updates itself.
8. A maintainer reviews it. Small requests for changes are normal, not a rejection.

## Changing the firmware

- The sketch is [`firmware/chess_clock_pico/chess_clock_pico.ino`](firmware/chess_clock_pico/chess_clock_pico.ino).
- Core 0 runs the clock logic and reads inputs; core 1 only draws the screen. Don't add anything
  slow (delays, blocking I/O) to `loop()` on core 0: it would delay lever presses.
- Run `./test/run_tests.sh` before and after your change and compare the printed clock values.
  If you add a feature, add a scenario for it to [`test/harness.cpp`](test/harness.cpp).
- If your change affects what's on screen, attach `test/shots/contact_sheet.png` (or the relevant
  `.pgm` frames) to the pull request.
- If you tested on a real Pico, say so in the pull request. That's very valuable.
- Keep the user settings (`DISPLAY_ROTATION`, `SWAP_LEVER_SIDES`, …) at the top of the sketch and
  document new ones in [README.md](README.md#flashing-the-firmware).

## Changing the 3D model

- Edit [`cad/chess_clock.scad`](cad/chess_clock.scad). Prefer changing or adding a named parameter
  at the top over hard-coding numbers inside modules.
- Check the fit with `part = "assembly"`, `"assembly_up"` and `"section"` before exporting.
- **Re-export every STL you changed** into [`stl/`](stl/), using the existing file names and print
  orientation (set `part`, then F6, then F7). STLs and the `.scad` source must stay in sync.
- Update [PARTS.md](PARTS.md) / [ASSEMBLY.md](ASSEMBLY.md) if hardware, screws or steps change.
- If you printed and test-fitted the change, say so (with a photo if you can).

## Changing the docs

- Use relative links to other files (`[WIRING.md](WIRING.md)`), never absolute GitHub URLs to files
  in this repo, so links work in forks and branches. `scripts/check_repo.py` checks them.
- Measurements in millimetres; part names as they appear in [PARTS.md](PARTS.md).
- If you add a new required file, add it to `REQUIRED_FILES` in
  [`scripts/check_repo.py`](scripts/check_repo.py) and link it from [README.md](README.md).

## What CI checks

Every push and pull request runs these [GitHub Actions workflows](.github/workflows/):

| Workflow | What it checks |
|---|---|
| [Repo check](.github/workflows/repo-check.yml) | All required files exist (docs, firmware, CAD, every STL, images, templates) and every relative link in the Markdown files points at something real |
| [Firmware](.github/workflows/firmware.yml) | The sketch compiles for a real Raspberry Pi Pico, and the desktop simulator builds and plays its scripted games |
| [CAD](.github/workflows/cad.yml) | Every printable part in `cad/chess_clock.scad` renders in OpenSCAD without errors |

## Pull request checklist

- [ ] One topic per pull request, linked to an issue if there is one
- [ ] `./scripts/check_repo.py` passes
- [ ] Firmware changes: `./test/run_tests.sh` passes and the output looks right
- [ ] CAD changes: the matching STLs in `stl/` are re-exported
- [ ] Docs updated if behaviour, parts, wiring or steps changed
- [ ] [CHANGELOG.md](CHANGELOG.md) has a line under *Unreleased* for user-visible changes

By contributing you agree that your contribution is released under the project's
[MIT license](LICENSE), and to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
