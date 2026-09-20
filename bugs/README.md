# Bug Tracker Layout

Recommended repository layout:

```text
bugs/
├── bugs_summary.md     # Index of all bugs with their metadata.
├── bugs_001-025.md     # Full descriptions of bugs 1-25.
├── bugs_026-050.md     # Full descriptions of bugs 26-50.
└── ...
```

The summary file is the index. Range files contain the full descriptions.

Bug IDs are permanent and are never renumbered. New bugs are appended to the next available ID.

Use fixed ranges of 25 IDs per detail file. Do not move a bug to another range when its severity changes.

Create a new range file when the current one reaches 25 bugs. For example, if `bugs_001-025.md` has 25 bugs, the next bug should be added to `bugs_026-050.md`.