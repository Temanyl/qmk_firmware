#!/bin/bash
# worktree-claude.sh - Create a git worktree and launch Claude Code in it

set -e

# Check if branch name is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <branch-name> [base-branch]"
    echo "Example: $0 my-feature"
    echo "Example: $0 my-feature master"
    exit 1
fi

BRANCH_NAME="$1"
BASE_BRANCH="${2:-HEAD}"
WORKTREE_DIR="$BRANCH_NAME"

# Get the repository root
REPO_ROOT=$(git rev-parse --show-toplevel)

# Create worktree
echo "Creating worktree for branch '$BRANCH_NAME' from '$BASE_BRANCH'..."
git worktree add -b "$BRANCH_NAME" "$WORKTREE_DIR" "$BASE_BRANCH"

# Change to worktree directory and launch Claude
echo "Switching to worktree directory: $REPO_ROOT/$WORKTREE_DIR"
cd "$REPO_ROOT/$WORKTREE_DIR"

echo "Launching Claude Code..."
exec claude --dangerously-skip-permissions
