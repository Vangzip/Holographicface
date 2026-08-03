# Project Code Index Design

## Purpose

Create a durable architectural index for MergeHolo so future maintenance can
identify a class, its ownership, dependencies, callers, and external boundary
without re-reading the whole source tree.

## Deliverable

Add `docs/PROJECT_CODE_INDEX.md` as the single entry point. It will contain:

1. A short reading guide and maintenance rules.
2. A build and runtime entry graph.
3. A module dependency graph.
4. A directory and file-role inventory.
5. Per-module class entries for all maintained source.
6. External SDK and library boundary entries.
7. Configuration, scripts, and test indexes.

## Scope

The index includes every maintained class, struct, interface, and meaningful
free-function module under `apps`, `camera`, `pipeline`, `printing`,
`settings`, `widgets`, `vendor/base`, `vendor/point_cloud`, and
`vendor/multiview`.

Qt, OpenCV, PCL, OSG, CUDA, the JpLF camera SDK, and the IMC60G SDK are not
indexed internally. Their direct adapters, used types, link/configuration
points, and runtime requirements are recorded as boundaries.

Generated build products, deployed runtime binaries, sample data, and test
executables are excluded. Test source and qmake test projects are included as
verification ownership rather than application architecture.

## Entry Format

Each maintained type or module is recorded with:

```text
Name and source file
Role
Direct project dependencies
Primary callers or owning flow
State, input, and output boundary
```

Entries use file paths and stable Markdown headings so a later reader can load
one module section instead of the full document.

## Graphs

The document uses Mermaid graphs for the executable-to-runtime flow and the
module-level dependency direction. Graphs show only project-owned modules;
external libraries are grouped into named boundary nodes. Fine-grained class
dependencies remain textual to keep the document readable.

## Maintenance and Validation

The index must be updated when `mergeholo.pro`, a `.pri` source list, a public
project type, a module boundary, or an external SDK adapter changes. Validation
will compare indexed source ownership against qmake source/header lists and
check that every documented path exists.

## Non-Goals

This is not API reference documentation, generated Doxygen output, or a
substitute for source-level comments. It records architecture and navigation
facts only.
