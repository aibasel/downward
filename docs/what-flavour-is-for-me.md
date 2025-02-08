Back to [quick start](quick-start.md).

## Run-time comparison

These are results of a very informal experiment (wall-clock time) run in
2019. Please treat it as anecdotal and possibly outdated evidence.
Native OS and compilers identical to container versions (we think; can
verify these later). Three runs each.

-   gripper prob01, blind search:
    -   native build: 0.098 seconds, 0.101 seconds, 0.102 seconds
    -   Apptainer (then called Singularity): 0.320 seconds, 0.350
        seconds, 0.364 seconds
    -   Docker: 1.124 seconds. 1.135 seconds, 1.203 seconds
    -   Vagrant: 1.726 seconds, 1.736 seconds, 1.792 seconds
-   gripper prob07, blind search:
    -   native build: 19.985 seconds, 20.082 seconds, 20.134 seconds
    -   Apptainer: 20.386 seconds, 20.488 seconds, 20.501 seconds
    -   Docker: 26.841 seconds, 27.030 seconds, 26.623 seconds
    -   Vagrant: 22.576 seconds, 22.616 seconds, 22.624 seconds

Vagrant results using "vagrant ssh -c". Most of the penalty over
native build seems to be establishing the ssh connection. For Docker, we
observed significantly different overheads when running similar tests on
different machines. In our tests, Apptainer/Singularity always caused a
very small overhead when compared to the native build, Docker sometimes
shows runtime overheads of around 40%. Again, keep in mind that these
are very informal tests.

## Other notes

-   points in favour of Apptainer over Docker:
    -   once Apptainer is installed, setup is trivial/non-existent
    -   no root privileges needed
    -   leaves no permanent traces on the machine: the planner is
        just a single file that can be used like a shell script with
        no dependencies or changes to the system
-   Apptainer and Docker are made for different purposes and have
    different isolation models. If you care about security, it is good
    to be aware of what they do or don't do.
    -   Apptainer: emphasis on running scientific experiments
    -   Docker: emphasis on deployment and composition of services
