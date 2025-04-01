In the `Add a title` field, write a meaningful short commit message in imperative mood. Start the title with [issueX] where X is the issue number on https://issues.fast-downward.org.

---

In this field (titeled `Add a description`), write a more extensive summary of the changes made in this pull request. Most importantly, describe what changes on the planner behavior they imply:
 - Do the changes add functionality?
 - Does the command line syntax change?
 - Does the change impact planner performance? (provide numbers from experiments if reasonable, see example below)

This description can and should be refined along the development process. In the end, it can be used more or less directly as the commit message when squashing and merging the changes.

---

Guidelines on the style of commit messages, taken from our [Wiki](https://www.fast-downward.org/latest/for-developers/git/) on April 1, 2025:

The commit message for merging an issue branch contains a concise summary of the changes that will be used for creating the release changelog. It must adhere to the following guidelines:
 - The first line starts with [issueX] and gives a one-sentence summary of the issue. On further lines, add a more detailed description explaining what changed from a user/developer perspective.
 - All lines should be below 80 characters. In particular, please avoid very long commit messages without line wrapping.
 - Always mention usage changes such as added functionality or changed command line arguments.

Example:
```
[issue666] search component reimplemented in C#

Prior to this change, the search component was implemented in C++.
We have changed the planner to run in C#, which is an industry
standard language. Expect a performance increase of at least -37%
in common cases. The planner is now able to solve large tasks
in the Gripper and Spanner domains.
```

Within the issue branch, you are free to write commit messages however you like, since these commits will be squashed. We still recommend the following guidelines as it will make doing reviews and writing the final commit message easier.
 - Prepend "[<branch>]" to all commits of the branch <branch>. You can copy misc/hooks/commit-msg to .git/hooks/commit-msg to enable a hook which automatically prepends the right string to each commit message.
 - The first line of the commit message should consist of a self-contained summary.
 - Please write the summary in the imperative mode (e.g., "Make translator faster." instead of "Made translator faster.", see https://chris.beams.io/posts/git-commit/).
