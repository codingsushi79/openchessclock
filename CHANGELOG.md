# Changelog

All notable changes to this project are listed here. Versions follow
[Semantic Versioning](https://semver.org/).

## Unreleased

## 1.0.1 - 2026-11-21
- Fix: the firmware failed to compile for a real Pico (Arduino's auto-generated prototypes
  came before the types they use). Shared types now live in `clock_types.h`.
- CI no longer runs twice when a tag is pushed.

## 1.0.0 - 2026-11-21
First public release.

- Rocker-lever body with framed channel, magnetic end stops and slotted KW11 switch ribs
- Raspberry Pi Pico firmware: Fischer, US delay, Bronstein and sudden death; 19 presets;
  low-time warnings; dual-core drawing so the screen never delays a lever press
- Desktop simulator that runs the real firmware and saves every screen
- Parts list, wiring guide and step-by-step assembly guide
- Contributing guide, issue templates and CI checks
