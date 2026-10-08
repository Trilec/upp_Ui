# Repository workflow

Work directly on `main`. Do not create or switch to a task branch unless the
user explicitly requests one. Publishing means committing the intended changes
and pushing `main`.

Fetch origin before publishing. Preserve unrelated staged, unstaged and
untracked work; stage only the intended changes. Reconcile newer main commits
without force-pushing or discarding concurrent work.

Follow the native Ui contracts and validation guidance in
`docs/00_UPP_CODING_GUIDE.md` and `docs/ACTIVE_WORK.md`.
