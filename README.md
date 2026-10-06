# ECEN4610 Capstone Lab — Adaptive Wheelchair

## Get Files Locally

```text
git clone <URL>                             # Download the repository to your computer
git status                                  # View the current repository state
git pull                                    # Download and merge the latest changes from GitHub
git fetch                                   # Download remote information without merging changes
```

## Branches

Use branches to create a new version of the repository for a specific task or feature. Work on new code in the branch, test it, and merge it into `main` when it is ready.

Name branches based on the task you are working on.

```text
git branch                                  # List local branches
git branch feature/<task-or-branch-name>    # Create a branch without switching to it
git checkout <branchname>                   # Switch to an existing branch
git checkout -b <branchname>                # Create a new branch and switch to it
```

### Push a New Branch to GitHub

The first time you push a new branch:

```text
git push -u origin feature/<branchname>     # Push branch and set its upstream
```

After the upstream has been set:

```text
git push                                    # Push new commits to the current branch
```

### Delete a Branch

```text
git push origin --delete feature/<branchname>  # Delete the branch from GitHub
git branch -d feature/<branchname>              # Delete the branch locally
```

## Commit Changes to GitHub

### 1. Check Your Changes

```text
git status                                  # Show modified, staged, and untracked files
```

### 2. Stage Changes

```text
git add filename1.c filename2.c             # Stage specific files
git add foldername/                         # Stage everything inside a folder
git add .                                   # Stage everything in the current directory
```

### 3. Commit Changes

```text
git commit -m "message"                     # Save staged changes as a commit
```

Use a short commit message that describes what changed.

Example:

```text
git commit -m "Add app joystick speed control"
```

### 4. Push Changes

```text
git push                                    # Upload commits to the current GitHub branch
```

## Typical Feature Workflow

Create and switch to a feature branch:

```text
git checkout -b feature/<task-name>         # Create and enter the new branch
```

Make your code changes, then:

```text
git status                                  # Review changed files
git add .                                   # Stage the changes
git commit -m "Describe the changes"        # Commit the changes
git push -u origin feature/<task-name>      # First push of the new branch
```

For later changes on the same branch:

```text
git add .
git commit -m "Describe the changes"
git push
```

When the feature is finished, merge it into `main`.
