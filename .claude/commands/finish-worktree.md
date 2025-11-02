# finish-worktree

Commit all changes in the current worktree, merge the branch to master, delete the worktree and branch.

## Instructions

You are helping the user finish work on a git worktree. Follow these steps IN ORDER without asking questions:

1. **Check current status**:
   - Get the current branch name: `git branch --show-current`
   - Check if there are uncommitted changes: `git status --short`

2. **Reset hardcoded dates to use system time**:
   - Check if HARDCODE_DATE_TIME is enabled in `keyboards/handwired/temanyl/chocmanyl36/keymaps/default/display/display.h`
   - Look for line with `#define HARDCODE_DATE_TIME` (uncommented)
   - If found (not already commented), comment it out by changing it to `// #define HARDCODE_DATE_TIME`
   - This ensures the master branch always uses real system date/time from the computer, not test values
   - Use the Edit tool to make this change if needed

3. **Commit changes** (if any exist):
   - Stage all modified files: `git add -u`
   - Create a meaningful commit with the format:
     ```
     git commit -m "[Keyboard] <summary of changes>

     <description>

     🤖 Generated with [Claude Code](https://claude.com/claude-code)

     Co-Authored-By: Claude <noreply@anthropic.com>"
     ```
   - If there are no changes, skip this step

4. **Switch to main repository and merge**:
   - The main repository is at `/Users/BBT/dev/my_qmk`
   - Switch to master: Run `git -C /Users/BBT/dev/my_qmk checkout master`
   - Merge the branch: Run `git -C /Users/BBT/dev/my_qmk merge <branch-name> -m "Merge branch '<branch-name>' - <summary>"`

5. **Delete the worktree**:
   - Get the worktree path: Extract from `git -C /Users/BBT/dev/my_qmk worktree list` output
   - Force remove the worktree directory: `rm -rf <worktree-path>`
   - Prune worktrees: `git -C /Users/BBT/dev/my_qmk worktree prune`

6. **Delete the branch**:
   - Delete the branch: `git -C /Users/BBT/dev/my_qmk branch -d <branch-name>`

## Important Notes

- Use `git -C /Users/BBT/dev/my_qmk` for all git commands to avoid path issues
- The worktree path is typically `/Users/BBT/dev/my_qmk/<branch-name>`
- Do NOT ask for confirmation - execute all steps automatically
- If a step fails, report the error but continue with remaining steps if possible
- At the end, report what was done and verify with `git -C /Users/BBT/dev/my_qmk worktree list`
