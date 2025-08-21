# Quick start
This page provides you with different options for setting up Fast Downward on your machine. 

Fast Downward is released in four flavours: Tarball, Apptainer (formerly
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
To run the Apptainer image, first install one of the [Apptainer releases](https://github.com/apptainer/apptainer/releases) or follow the steps provided on the [Apptainer page](https://apptainer.org/docs/user/main/quick_start.html#installation).
We tested with versions 1.2.2 and 1.4.1.

To download the Fast Downward image, run:

    apptainer pull fast-downward.sif docker://aibasel/downward:latest

### Apptainer Usage
You can run the planner as follows:

    ./fast-downward.sif <your-planner-options>

Assume you want to solve a planning task (e.g. from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks)) using A* search with the LM-cut heuristic. To do so you can run the following:

    ./fast-downward.sif <domain.pddl> <task.pddl> --search "astar(lmcut())"

See more planner options at [planner usage](planner-usage.md).

## Docker

### Docker Installation
To install Docker on your machine follow the steps provided on the [Docker page](https://docs.docker.com/get-docker/).

### Docker Usage
You can run the planner as follows:

    sudo docker run aibasel/downward <your-planner-options>

Note:

-   The use of `sudo` (Docker usually requires root privileges).
-   The Docker image for Fast Downward is installed on your machine as a side-effect of the command.
-   You can use the Docker flag `-rm` for cleaning up the container (recommended)

Assume you want to solve a planning task (e.g. from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks)) using A* search with the LM-cut heuristic. To do so you can run the following:

    sudo docker run --rm -v <path-to-benchmarks>:/benchmarks aibasel/downward \
    /benchmarks/<domain.pddl> /benchmarks/<task.pddl> \
    --search "astar(lmcut())"

Here `<path-to-benchmarks>` refers to your local directory containing your domain and task file.

Note:

-   The Docker flag `-v` mounts the local directory `<path-to-benchmarks>` of your host 
    machine under the container directory `/benchmarks`, which is the
    place where the containerised planner looks for the problem.
-   `<path-to-benchmarks>` must be an absolute path; relative paths are only supported as of Docker Engine version 23.

See more planner options at [planner usage](planner-usage.md).

## Vagrant

### Vagrant Installation
To install Vagrant on your machine follow the steps provided on the [Vagrant page](https://developer.hashicorp.com/vagrant/install).
For Vagrant to work you also need a "provider" such as VirtualBox, VMware, Hyper-V.

Note:

- Not all provider versions and Vagrant versions are compatible!
- We recommend using Vagrant 2.4.1 and [VirtualBox 7.0.20](https://www.virtualbox.org/wiki/Download_Old_Builds_7_0).

To create your virtual machine using Vagrant create a new directory `<my-fast-downward-vm>` containing only the Fast Downward Vagrantfile for
the desired [release](https://www.fast-downward.org/latest/releases). 

If you want to use LP solvers follow the steps under [LP Solvers](#lp_solvers).

Otherwise, you can set up your virtual machine directly from the `<my-fast-downward-vm>` directory using:
    
    vagrant up

#### LP solvers

The SoPlex LP solver is included automatically. If you want to also
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

Assume you want to solve a planning task (e.g. from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks)) using A* search with the LM-cut heuristic. To do so you can run the following:

``` bash
# From the <my-fast-downward-vm> directory 

# Upload the benchmarks to the benchmarks directory on your vagrant machine
vagrant upload <path-to-benchmarks> benchmarks/

# Log into the VM.
vagrant ssh

# Run the planner.
downward/fast-downward.py \
/vagrant/benchmarks/<domain.pddl> /vagrant/benchmarks/<task.pddl> \
--search "astar(lmcut())"

# Log out from the VM.
logout

# Shut down the machine
vagrant halt
```

Here `<path-to-benchmarks>` refers to your local directory containing your domain and task file.

See more planner options at [planner usage](planner-usage.md).

## Source Code

### Source Code Installation

We recommend using the [latest release](https://www.fast-downward.org/latest/releases), especially
for scientific experiments. See the [build instructions](https://github.com/aibasel/downward/blob/main/BUILD.md)
for a complete description on how to build the planner from source.

#### Tarball
Download the tarball from the [latest release](https://www.fast-downward.org/latest/releases).
E.g. to build Fast Downward 24.06.01 download the tarball from the [Fast Downward 24.06 release page](https://www.fast-downward.org/latest/releases/24.06/#downloads) and run the following: 

    tar -xvzf fast-downward-24.06.1.tar.gz
    cd fast-downward-24.06.1
    ./build.py

#### Repository
Alternatively, you can fork/clone the [repository](https://github.com/aibasel/downward). If you are using the main branch, be aware that things can break or degrade with every commit.

### Source Code Usage
You can run the planner as follows:

    ./fast-downward.py <your-planner-options>

Assume you want to solve a planning task (e.g. from the [Fast Downward benchmarks](https://github.com/aibasel/downward-benchmarks)) using A* search with the LM-cut heuristic. To do so you can run the following:

    ./fast-downward.py <domain.pddl> <task.pddl> --search "astar(lmcut())"

See more planner options at [planner usage](planner-usage.md).


# Next steps

-   See other ways of invoking the planner on
    [planner usage](planner-usage.md).
-   Read about recommended [experiment
    setups](/#scientific_experiments).
