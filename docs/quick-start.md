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
To run the Apptainer image, first install one of the [releases](https://github.com/apptainer/apptainer/releases) or follow the steps provided [here](https://apptainer.org/docs/user/main/quick_start.html#installation).
We tested with versions 1.2.2 and 1.4.1.

To download the Fast Downward image, run:

    apptainer pull fast-downward.sif docker://aibasel/downward:latest

### Apptainer Usage
You can run the planner as follows:

    ./fast-downward.sif <your-planner-options>

Assume you want to solve the gripper planning task `prob01.pddl` from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks) using A* search with the LM-cut heuristic. To do so you can run the following:

    ./fast-downward.sif <path-to-benchmarks>/gripper/prob01.pddl --search "astar(lmcut())"

Here `<path-to-benchmarks>` refers to your local benchmarks directory containing the [gripper benchmarks](https://github.com/aibasel/downward-benchmarks/tree/master/gripper).
It can be specified as an absolute path, relative path or using a predefined path variable, e.g. `$BENCHMARKS`.

See more planner options at [planner usage](planner-usage.md).

## Docker

### Docker Installation
To install Docker on your machine follow the steps provided [here](https://docs.docker.com/get-docker/).

### Docker Usage
You can run the planner as follows:

    sudo docker run aibasel/downward <your-planner-options>

Note:

-   The use of `sudo` (Docker usually requires root privileges).
-   The Docker image for Fast Downward is installed on your machine as a side-effect of the command.
-   You can use the Docker flag `-rm` for cleaning up the container (recommended)

Assume you want to solve the gripper planning task `prob01.pddl` from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks) using A* search with the LM-cut heuristic. To do so you can run the following:

    sudo docker run --rm -v <path-to-benchmarks>:/benchmarks aibasel/downward /benchmarks/gripper/prob01.pddl --search "astar(lmcut())"

Here `<path-to-benchmarks>` refers to your local benchmarks directory containing the [gripper benchmarks](https://github.com/aibasel/downward-benchmarks/tree/master/gripper).
It can be specified as an absolute path, relative path (only as of Docker Engine version 23) or using a predefined path variable, e.g. `$BENCHMARKS`.

Note:

-   The Docker flag `-v` mounts the local directory `<path-to-benchmarks>` of your host 
    machine under the container directory `/benchmarks`, which is the
    place where the containerised planner looks for the problem.

See more planner options at [planner usage](planner-usage.md).

## Vagrant

### Vagrant Installation
To install Vagrant on your machine follow the steps provided [here](https://developer.hashicorp.com/vagrant/install).
For Vagrant to work you also need a "provider" such as VirtualBox, VMware, Hyper-V.

Note:

- Not all provider versions and Vagrant versions are compatible!
- We recommend using Vagrant 2.4.1 and [VirtualBox 7.0.20](https://www.virtualbox.org/wiki/Download_Old_Builds_7_0).

To create your virtual machine using Vagrant create a new directory `<my-fast-downward-vm>` containing only the Fast Downward Vagrantfile for
the desired [release](https://www.fast-downward.org/latest/releases). 

If you want to use LP solvers see [LP Solvers](#lp_solvers).

Otherwise, you can set up your virtual machine from the `<my-fast-downward-vm>` directory using:
    
    vagrant up

#### LP solvers

The !SoPlex LP solver is included automatically. If you want to also
use the CPLEX LP solver within the planner, its installer file must
be present in the directory `<path-to-lp-installers>`. As of Fast
Downward 23.06, you will need CPLEX 22.1.1 (installer filename
`cplex_studio2211.linux-x86-64.bin`). 

To set up your virtual machine from the `<my-fast-downward-vm>` directory as follows:

``` bash
export DOWNWARD_LP_INSTALLERS=<path-to-lp-installers>

vagrant up
```

You can now safely delete the LP installers and unset the environment variable.

### Vagrant Usage
You can run the planner from *within* the vagrant VM as follows:

    downward/fast-downward.py <your-planner-options>

Assume you want to solve the gripper planning task `prob01.pddl` from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks) using A* search with the LM-cut heuristic. To do so you can run the following:

``` bash
# From the <my-fast-downward-vm> directory 

# Upload the benchmarks to the benchmarks directory on your vagrant machine
vagrant upload <path-to-benchmarks> benchmarks/

# Log into the VM.
vagrant ssh

# Run the planner.
downward/fast-downward.py /vagrant/benchmarks/gripper/prob01.pddl --search "astar(lmcut())"

# Log out from the VM.
logout

# Shut down the machine
vagrant halt
```

Here `<path-to-benchmarks>` refers to your local benchmarks directory containing the [gripper benchmarks](https://github.com/aibasel/downward-benchmarks/tree/master/gripper).
It can be specified as an absolute path, relative path (only as of Docker Engine version 23) or using a predefined path variable, e.g. `$BENCHMARKS`.

See more planner options at [planner usage](planner-usage.md).

## Source Code

### Source Code Installation

We recommend using the [latest release](https://www.fast-downward.org/latest/releases), especially
for scientific experiments. 

#### Tarball
Download the tarball from the [latest release](https://www.fast-downward.org/latest/releases).
E.g. to build Fast Downward 24.06.01 download the tarball from [here](https://www.fast-downward.org/latest/releases/24.06/#downloads) and run the following: 

    tar -xvzf fast-downward-24.06.1.tar.gz
    cd fast-downward-24.06.1
    ./build.py

#### Repository
Alternatively, you can fork/clone the latest release branch of the [repository](https://github.com/aibasel/downward) and build Fast Downward using:
    
    ./build.py

Note:

-   If you are using the main branch, be aware that things can break or degrade with every commit.

See the [build instructions](https://github.com/aibasel/downward/blob/main/BUILD.md)
for a complete description on how to build the planner from source.

### Source Code Usage
You can run the planner as follows:

    ./fast-downward.py <your-planner-options>

Assume you want to solve the gripper planning task `prob01.pddl` from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks) using A* search with the LM-cut heuristic. To do so you can run the following:

    ./fast-downward.py <path-to-benchmarks>/gripper/prob01.pddl --search "astar(lmcut())"

Here `<path-to-benchmarks>` refers to your local benchmarks directory containing the [gripper benchmarks](https://github.com/aibasel/downward-benchmarks/tree/master/gripper).
It can be specified as an absolute path, relative path or using a predefined path variable, e.g. `$BENCHMARKS`.

See more planner options at [planner usage](planner-usage.md).


# Next steps

-   See other ways of invoking the planner on
    [planner usage](planner-usage.md).
-   Read about recommended [experiment
    setups](https://github.com/aibasel/downward#scientific-experiments).
