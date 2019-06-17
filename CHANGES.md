# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<http://hg.fast-downward.org>) and the issue tracker
(<http://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Fast Downward 19.06+

- When the merge-and-shrink computation terminates with several factors (due to
  using a time limit), only keep those factors that are non-trivial, i.e.,
  which consist of at least a goal state or which represent a non-total
  function. <http://issues.fast-downward.org/issue999>

## Fast Downward 19.06

Released on June 11, 2019.
First time-based release.
