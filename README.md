# ECEN4610-Capstone-Lab-Adaptive-Wheelchair

## Get Files Locally

```text
git clone <URL>                         # Download the repository locally
git status                              # Check the current repository state
git pull                                # Get the latest changes from GitHub
git fetch                               # Download remote changes without merging
```

## Branches

Use a separate branch for each new feature, fix, or major change. Name branches based on the task being worked on.

The `main` branch should contain the latest tested version of the project.

### View Branches

```text
git branch                              # List local branches
git branch -a                           # List local and remote branches
git branch -r                           # List remote branches
```

### Create and Switch Branches

```text
git switch <branchname>                 # Switch to an existing branch
git switch -c <branchname>              # Create a new branch and switch to it
```

Example:

```text
git switch -c app-drive-control         # Create and switch to a new branch
```

## Making Changes

### Check Your Changes

```text
git status                              # Show changed, staged, and untracked files
git diff                                # Review unstaged changes
git diff --staged                       # Review changes ready to commit
```

### Stage Changes

```text
git add filename1.c filename2.c         # Stage specific files
git add foldername/                     # Stage changes in a folder
git add -A                              # Stage all changes, new files, and deletions
```

### Commit Changes

```text
git commit -m "Describe the change"     # Save staged changes as a commit
```

Use a short message that describes what was changed.

Example:

```text
git commit -m "Add app joystick speed control"
```

### Push Changes

For the first push of a new branch:

```text
git push -u origin <branchname>         # Push branch and set its upstream
```

After the upstream has been set:

```text
git push                                # Push new commits to the current branch
```

## Typical Branch Workflow

Start with an up-to-date `main`:

```text
git switch main                         # Switch to main
git pull                                # Get the latest version of main
```

Create a branch for your task:

```text
git switch -c <branchname>              # Create and switch to the new branch
```

Make and test your changes.

Then:

```text
git status                              # Review the current changes
git diff                                # Review what you changed
git add -A                              # Stage all changes
git commit -m "Describe the change"     # Commit the changes
git push -u origin <branchname>         # First push of the new branch
```

For additional changes on the same branch:

```text
git add -A                              # Stage new changes
git commit -m "Describe the change"     # Commit the changes
git push                                # Push to the existing branch
```

## Updating Your Branch

Before continuing work, you may need the latest changes from GitHub:

```text
git pull                                # Download and merge the latest changes
```

To start new work from the latest `main`:

```text
git switch main                         # Switch to main
git pull                                # Update main
git switch -c <branchname>              # Create a new branch from updated main
```

## Merging a Branch

Only merge a branch into `main` after the changes have been tested.

First update `main`:

```text
git switch main                         # Switch to main
git pull                                # Get the latest main
```

Merge your branch:

```text
git merge <branchname>                  # Merge the branch into main
```

Push the updated `main`:

```text
git push origin main                    # Push the merged main branch to GitHub
```

## Deleting Branches

After a branch has been successfully merged:

```text
git branch -d <branchname>              # Delete the branch locally
git push origin --delete <branchname>   # Delete the branch from GitHub
```

## Undoing Changes

Discard changes to a file that have **not been committed**:

```text
git restore <filename>                  # Discard uncommitted changes to a file
```

Unstage a file without deleting its changes:

```text
git restore --staged <filename>         # Remove a file from staging
```

**Be careful:** `git restore <filename>` discards your uncommitted changes to that file.

## Useful Commands

```text
git branch --show-current               # Show the branch you are currently on
git log --oneline                       # Show recent commits
git log --oneline --graph --all         # Show commits and branch relationships
git remote -v                           # Show the configured GitHub remote
```

## Team Rules

* Do not develop directly on `main`.
* Pull `main` before creating a new branch.
* Use a separate branch for each significant task or change.
* Test changes before merging them into `main`.
* Use descriptive commit messages.
* Check `git status` before committing or switching branches.
* Do not force-push to `main`.
