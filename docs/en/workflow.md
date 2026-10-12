[English](../../README.md) · [Español](../es/workflow.md)

# Keep local work and publish manually

The repository is **Sol1vaN/AVP-Tyrargo**, public and independent on GitHub. It retains NakedAVP 1.2.3 as Git ancestry and an `upstream` remote for reference. It was prepared privately before its first public release; it is not in GitHub’s fork network.

The original local project, probes and game installation are separate from the publication checkout. Creating Git commits or a remote repository does not erase those folders. Git tracks committed files; it does not automatically back up ignored saves, assets or old probe folders. Continue backing those up normally.

## What each action does

| Action | Effect |
|---|---|
| Edit / build / test | Local work only |
| `git add` | Select files for the next local commit |
| `git commit` | Store a local history checkpoint |
| `git push` | Explicitly send selected commits to GitHub |
| Draft release | Prepare an unpublished release entry; no automatic build publication |
| Publish release / change visibility | A separate deliberate decision after review |

## Normal iteration

1. Continue local engineering and save-based testing. Commit a coherent candidate only after reviewing the diff.
2. If changes were made in a separate probe checkout, copy only the selected current source into this publication checkout, keeping documentation and original probes. Do not copy game data/saves/build trees.
3. Update version and affected catalog/roadmap entries. Regenerate documentation and verify:

```sh
python tools/update_docs.py
python tools/update_docs.py --check
python tools/check_docs.py
git status --short
git diff --stat
```

4. Select and commit the intended files locally. Inspect staged contents before committing:

```sh
git add src CMakeLists.txt docs README.md README.es.md README_TYRARGO.md
git diff --cached --stat
git commit -m "Describe the tested change"
```

5. Only when you decide to send that candidate:

```sh
git push origin main
```

There are no automatic pushes, merges or releases. Documentation verification runs locally on demand. No GitHub Actions build, push or release workflow is installed.

## Safe regression review

List checkpoints with `git log --oneline`. To inspect an older revision without overwriting current files, create a separate checkout:

```sh
git worktree add --detach ../Tyrargo-regression <commit>
```

Replace `<commit>` with an existing reviewed hash; keep original probe/build/game folders separate. A new Git revert commit is preferable to rewriting remote history when a published change must be undone. Do not use destructive reset/clean commands without understanding what local work they discard.

The first Tyrargo import preserves upstream commits, but the 249 historical probes are not manufactured as 249 Git commits. Their original folders remain local; the tracked milestone inventory makes them discoverable.

## Private review and first public release

Review documentation and provenance, test the candidate in-game, prepare complete distribution notices and relinking materials where required, then deliberately choose public visibility and release publication. Visibility and release publication are manual actions, authorized by the maintainer. Nothing modifies atsb's repository.

## Contacting atsb

An issue/discussion can carry a brief announcement if the project's contribution policy welcomes it; a pull request is for a concrete proposed code contribution, and Projects are planning boards. A local [announcement draft](../../MAINTAINER_MESSAGE_DRAFT.md) is prepared, not sent. The repository and release links can now be included in that draft. NakedAVP currently has Issues enabled and Discussions disabled; check its contribution policy before posting an announcement.
