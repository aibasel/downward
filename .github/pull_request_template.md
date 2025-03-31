[issueX] Write a meaningful, short commit message in imperative mode.
---

Write a more extensive summary of the changes here. Most importantly, describe what changes on the planner behavior it implies:
- Does the command line syntax change?
- Do the changes add functionality?
- Does the change impact planner performance? (faster, slower, ... in numbers)

This description can and should be refined along the development process. In the end, it can be used more or less directly as the commit message when squashing and merging the changes.

Guidelines on the style of commit messages, taken from our Wiki on March 31, 2025:

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
