# Quick start
This page provides you with different options for setting up Fast Downward on your machine. 

Fast Downward is released in four flavours: tarball, Apptainer (formerly
known as Singularity), Docker and Vagrant. Here we provide instructions
to get you started as quickly as possible. You can find more usage
information at [planner usage](planner-usage.md).

## What flavour is for me?

-   **running experiments**: We recommend Apptainer or the tarball.
    Docker is an alternative, but be mindful of its significant
    overhead.
-   **teaching:** We recommend Vagrant.
-   **development:** We recommend working on a clone of the master
    repository.

See [what-flavour-is-for-me](what-flavour-is-for-me.md) for a more
detailed discussion.

## Apptainer

### Apptainer Installation
We have tested Apptainer 1.2.2.
To install Apptainer 1.2.2 on your machine follow the steps provided [here](https://apptainer.org/docs/user/1.2/quick_start.html#quick-installation).
If you want to use the newest version of Apptainer follow [these](https://apptainer.org/docs/user/main/quick_start.html#installation) steps instead.

To download the Fast Downward image, run:

    apptainer pull fast-downward.sif docker://aibasel/downward:latest

### Apptainer Usage
You can run the planner as follows:

    ./fast-downward.sif [your planner options]

Assume you want to solve the gripper planning task `prob01.pddl` using A* search with the blind heuristic. To do so you can run the following:

    ./fast-downward.sif path-to-benchmarks/gripper/prob01.pddl --search "astar(blind())"

Here `path-to-benchmarks` can be an absolute path, relative path or a predefined path variable, e.g. `$BENCHMARKS`.

See more planner options at [planner usage](planner-usage.md).

## Docker

### Docker Installation
To install Docker on your machine follow the steps provided [here](https://docs.docker.com/get-docker/).

### Docker Usage
You can run the planner as follows:

    sudo docker run aibasel/downward [your planner options]

Note:

-   The use of `sudo` (Docker usually requires root privileges).
-   The Docker image for Fast Downward is installed on your machine as a side-effect of the command.
-   You can use the Docker flag `-rm` for cleaning up the container (recommended)

Assume you want to solve the gripper planning task `prob01.pddl` using A* search with the blind heuristic. To do so you can run the following:

    sudo docker run --rm -v path-to-benchmarks:/benchmarks aibasel/downward /benchmarks/gripper/prob01.pddl --search "astar(blind())"

Here `path-to-benchmarks` can be an absolute path, relative path (only as of Docker Engine version 23) or a predefined path variable, e.g. `$BENCHMARKS`.

Note:

-   The Docker flag `-v` mounts the local directory `path-to-benchmarks` of your host 
    machine under the container directory `/benchmarks`, which is the
    place where the containerized planner looks for the problem.

See more planner options at [planner usage](planner-usage.md).

## Vagrant

### Vagrant Installation
To install Vagrant on your machine follow the steps provided [here](https://developer.hashicorp.com/vagrant/install).

You want to create your Vagrant VM as subdirectory `my-fast-downward-vm` of the current directory. 
The subdirectory should not exist yet. The current directory must contain the Fast Downward `Vagrantfile` for
the desired [release](https://www.fast-downward.org/latest/releases).

You can create your virtual machine from the current directory as follows:

``` bash
# Set up the VM.
mkdir my-fast-downward-vm
cp Vagrantfile my-fast-downward-vm/

# Skip next line if you don't need LP support.
export DOWNWARD_LP_INSTALLERS=/path/to/lp/installers

# Set up the VM.
# After this step you can now safely delete the LP installers and unset the environment variable.
cd my-fast-downward-vm
vagrant up
```

Note:

-   The !SoPlex LP solver is included automatically. If you want to also
    use the CPLEX LP solver within the planner, its installer file must
    be present in the directory `/path/to/lp/installers`. As of Fast
    Downward 23.06, you will need CPLEX 22.1.1 (installer filename
    `cplex_studio2211.linux-x86-64.bin`). 

### Vagrant Usage
You can run the planner as follows:

``` bash
# From the my-fast-downward-vm directory

# log into the VM.
vagrant ssh

# Run the planner.
downward/fast-downward.py [your planner options]

# Log out from the VM.
logout
```

Assume you want to solve the gripper planning task `prob01.pddl` using A* search with the blind heuristic. To do so you can run the following:

``` bash
# From the my-fast-downward-vm directory 

# Upload the benchmarks to the destination path on your vagrant machine
vagrant upload path-to-benchmarks/gripper destination

# Log into the VM.
vagrant ssh

# Run the planner.
downward/fast-downward.py /vagrant/destination/gripper/prob01.pddl --search "astar(blind())"

# Log out from the VM.
logout
```

Here `path-to-benchmarks` can be an absolute or relative path. Alternatively a predefined path variable, e.g. `$BENCHMARKS` can be used.

See more planner options at [planner usage](planner-usage.md).

## Source Code

### Source Code Installation
See the [build
instructions](https://github.com/aibasel/downward/blob/main/BUILD.md)
for a complete description on how to build the planner from source. We
recommend using the [latest release](https://www.fast-downward.org/latest/releases), especially
for scientific experiments. If you are using the main branch instead, be
aware that things can break or degrade with every commit.

### Source Code Usage
You can run the planner as follows:

    ./fast-downward.py [your planner options]

Assume you want to solve the gripper planning task `prob01.pddl` using A* search with the blind heuristic. To do so you can run the following:

    ./fast-downward.py path-to-benchmarks/gripper/prob01.pddl --search "astar(blind())"

Here `path-to-benchmarks` can be an absolute or relative path. Alternatively a predefined path variable, e.g. `$BENCHMARKS` can be used.

See more planner options at [planner usage](planner-usage.md).


# Next steps

-   See other ways of invoking the planner on
    [planner usage](planner-usage.md).
-   Read about recommended [experiment
    setups](https://github.com/aibasel/downward#scientific-experiments).
