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

# Check if worktree already exists
if [ -d "$REPO_ROOT/$WORKTREE_DIR" ]; then
    echo "Worktree directory '$WORKTREE_DIR' already exists, skipping creation..."
    # Verify it's actually a worktree
    if ! git worktree list | grep -q "$REPO_ROOT/$WORKTREE_DIR"; then
        echo "Warning: Directory exists but is not a git worktree!"
        echo "You may need to remove it manually or use a different name."
        exit 1
    fi
else
    # Create worktree
    echo "Creating worktree for branch '$BRANCH_NAME' from '$BASE_BRANCH'..."
    git worktree add -b "$BRANCH_NAME" "$WORKTREE_DIR" "$BASE_BRANCH"
fi

# Change to worktree directory and launch Claude
echo "Switching to worktree directory: $REPO_ROOT/$WORKTREE_DIR"
cd "$REPO_ROOT/$WORKTREE_DIR"

# Set terminal title to branch name
echo -ne "\033]0;${BRANCH_NAME}\007"

echo "Launching Claude Code..."
claude --dangerously-skip-permissions

# After Claude exits, return to parent directory
echo "Returning to repository root..."
cd "$REPO_ROOT"

# Restore terminal title to repository name
REPO_NAME=$(basename "$REPO_ROOT")
echo -ne "\033]0;${REPO_NAME}\007"
