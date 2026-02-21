#!/usr/bin/env bash
set -euo pipefail

# Sync script: fetch upstream 'dev' and merge into update branch, preserving local .github and .devcontainer
# Defaults (can be overridden with env vars):
UPSTREAM_URL="${UPSTREAM_URL:-https://git.planet-casio.com/Lephenixnoir/PythonExtra.git}"
UPSTREAM_BRANCH="${UPSTREAM_BRANCH:-dev}"
BASE_BRANCH="${BASE_BRANCH:-main}"
UPDATE_BRANCH="${UPDATE_BRANCH:-update-from-upstream-dev-${BASE_BRANCH}}"
BACKUP_BRANCH="pre-update-backup-${BASE_BRANCH}"

echo "Repo root: $(pwd)"
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || { echo "Not a git repository"; exit 1; }

echo "Base branch: $BASE_BRANCH"

# Ensure base branch is available locally
git fetch origin "$BASE_BRANCH" || true

# Create backup branch (if not exists) to preserve local workflows/devcontainer
if git show-ref --verify --quiet refs/heads/$BACKUP_BRANCH; then
  echo "Backup branch $BACKUP_BRANCH exists"
else
  echo "Creating backup branch $BACKUP_BRANCH from origin/$BASE_BRANCH"
  if git show-ref --verify --quiet refs/remotes/origin/$BASE_BRANCH; then
    git checkout -b "$BACKUP_BRANCH" "origin/$BASE_BRANCH"
  else
    git checkout -b "$BACKUP_BRANCH" "$BASE_BRANCH"
  fi
fi

# Add or update upstream remote
if git remote get-url upstream-base >/dev/null 2>&1; then
  git remote set-url upstream-base "$UPSTREAM_URL"
  echo "Updated remote upstream-base -> $UPSTREAM_URL"
else
  git remote add upstream-base "$UPSTREAM_URL"
  echo "Added remote upstream-base -> $UPSTREAM_URL"
fi

# Fetch upstream branch
git fetch upstream-base "$UPSTREAM_BRANCH" || { echo "Failed to fetch $UPSTREAM_BRANCH from upstream-base"; exit 1; }

# Create or checkout update branch from base
if git show-ref --verify --quiet refs/heads/$UPDATE_BRANCH; then
  git checkout "$UPDATE_BRANCH"
else
  if git show-ref --verify --quiet refs/remotes/origin/$BASE_BRANCH; then
    git checkout -b "$UPDATE_BRANCH" "origin/$BASE_BRANCH"
  else
    git checkout -b "$UPDATE_BRANCH" "$BASE_BRANCH"
  fi
fi

echo "Merging upstream-base/$UPSTREAM_BRANCH into $UPDATE_BRANCH"
if git merge --no-commit --no-ff --allow-unrelated-histories "upstream-base/$UPSTREAM_BRANCH"; then
  echo "Merge prepared (no commit)"
else
  echo "Merge failed or conflicts detected. Aborting merge and exiting."
  git merge --abort || true
  exit 1
fi

# Restore local .github and .devcontainer from backup branch to preserve workflows and devcontainer
git checkout "$BACKUP_BRANCH" -- .github .devcontainer || true

# Finalize commit
git add -A
if git commit -m "Merge $UPSTREAM_URL:$UPSTREAM_BRANCH into $BASE_BRANCH — preserve .github and .devcontainer"; then
  echo "Merge committed on $UPDATE_BRANCH"
else
  echo "Nothing to commit (maybe there were no changes)"
fi

# Ensure git user configured
git config user.name >/dev/null 2>&1 || git config user.name "github-actions[bot]"
git config user.email >/dev/null 2>&1 || git config user.email "github-actions[bot]@users.noreply.github.com"

echo "Pushing $UPDATE_BRANCH to origin"
git push origin "$UPDATE_BRANCH"

echo "Done"
