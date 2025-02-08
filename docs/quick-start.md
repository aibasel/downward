# Quick start

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

## Running the Apptainer image

We have tested Apptainer 1.2.2. If Apptainer isn't installed on your
machine, check the following section.

To download the Fast Downward image, run:

    apptainer pull fast-downward.sif docker://aibasel/downward:latest

Then run the planner. The example uses the LAMA-first configuration to
solve a planning task located in the `$BENCHMARKS` directory.
LAMA-first is designed to find solutions quickly without much regard for
plan cost:

    ./fast-downward.sif --alias lama-first $BENCHMARKS/gripper/prob01.pddl

Unlike Docker (see below), `$BENCHMARKS` does not have to be an
absolute path.

### Installing Apptainer

Apptainer's predecessor, Singularity, used to be shipped with Ubuntu
Linux for some time, making its installation very convenient. As of this
writing, this is no longer the case, but Ubuntu (deb) packages are
available from the Apptainer developers. For a typical Ubuntu system,
download the AMD64 deb package from
<https://github.com/apptainer/apptainer/releases> and install it like so
(example for Apptainer 1.2.2):

    sudo apt install ./apptainer_1.2.2_amd64.deb

## Running the Docker image

We assume that [Docker](https://docs.docker.com/get-docker/)
is installed on your machine. You want to solve a planning problem
located on the `$BENCHMARKS` directory. You can run the same
LAMA-first configuration as before:

    sudo docker run --rm -v $BENCHMARKS:/benchmarks aibasel/downward --alias lama-first /benchmarks/gripper/prob01.pddl

Note the use of `sudo` (Docker usually requires root privileges).

Note that this mounts the local directory `$BENCHMARKS` of your host
machine under the container directory `/benchmarks`, which is the
place where the containerized planner looks for the problem. The path
stored in the `$BENCHMARKS` variable must be absolute.

The Docker image for Fast Downward is installed on your machine as a
side-effect of the command.

## Using a Vagrant machine

We assume that

-   [Vagrant](https://www.vagrantup.com/) is installed on
    your machine
-   The current directory contains the Fast Downward `Vagrantfile` for
    the desired [release](https://www.fast-downward.org/releases), the PDDL files
    `domain.pddl` and `problem.pddl` for the planning task you want
    to solve.
-   The !SoPlex LP solver is included automatically. If you want to also
    use the CPLEX LP solver within the planner, its installer file must
    be present in the directory `/path/to/lp/installers`. As of Fast
    Downward 23.06, you will need CPLEX 22.1.1 (installer filename
    `cplex_studio2211.linux-x86-64.bin`).
-   You want to create your Vagrant VM as subdirectory
    `my-fast-downward-vm` of the current directory. The subdirectory
    does not exist yet.

Create and provision your virtual machine as follows:

``` bash
# Set up the VM. Only run this one.
mkdir my-fast-downward-vm
cp Vagrantfile my-fast-downward-vm/
cp domain.pddl problem.pddl my-fast-downward-vm/
# Skip next line if you don't need LP support.
export DOWNWARD_LP_INSTALLERS=/path/to/lp/installers
cd my-fast-downward-vm
vagrant up
# The VM is now set up.
# You can now safely delete the LP installers and unset the environment variable.

# Log into the VM and run the planner.
vagrant ssh
downward/fast-downward.py --alias lama-first /vagrant/domain.pddl /vagrant/problem.pddl

# Log out from the VM.
exit
```

## Source code

See the [build
instructions](https://github.com/aibasel/downward/blob/main/BUILD.md)
for a complete description on how to build the planner from source. We
recommend using the [latest release](https://www.fast-downward.org/releases), especially
for scientific experiments. If you are using the main branch instead, be
aware that things can break or degrade with every commit.

# Next steps

-   See other ways of invoking the planner on
    [planner usage](planner-usage.md).
-   Read about recommended [experiment
    setups](https://github.com/aibasel/downward#scientific-experiments).
