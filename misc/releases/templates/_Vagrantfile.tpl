# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # For a complete reference of vagrant options see https://docs.vagrantup.com.

  # We increase the RAM from a default of 1GB for compiling with SoPlex.
  # See issue1101 for details.
  config.vm.provider "virtualbox" do |v|
    v.memory = 2048
  end

  config.vm.box = "ubuntu/jammy64"

  # To compile the planner with support for CPLEX, download the 64-bit Linux
  # installer of CPLEX 22.1.1 and set the environment variable
  # DOWNWARD_LP_INSTALLERS to an absolute path containing them before
  # provisioning the VM.
  provision_env = {}
  if !ENV["DOWNWARD_LP_INSTALLERS"].nil?
      cplex_installer = ENV["DOWNWARD_LP_INSTALLERS"] + "/cplex_studio2211.linux_x86_64.bin"
      if File.file?(cplex_installer)
          config.vm.synced_folder ENV["DOWNWARD_LP_INSTALLERS"], "/lp", :mount_options => ["ro"]
          provision_env["CPLEX_INSTALLER"] = "/lp/" + File.basename(cplex_installer)
      end
  end

  config.vm.provision "shell", env: provision_env, inline: <<-SHELL

    apt-get update && apt-get install --no-install-recommends -y \
        ca-certificates \
        cmake           \
        default-jre     \
        g++             \
        git             \
        libgmp3-dev     \
        make            \
        python3         \
        unzip           \
        zlib1g-dev

    if [ -f "$CPLEX_INSTALLER" ]; then
        # Set environment variables for CPLEX.
        cat > /etc/profile.d/downward-cplex.sh <<-EOM
			export cplex_DIR="/opt/ibm/ILOG/CPLEX_Studio2211/cplex"
		EOM
        source /etc/profile.d/downward-cplex.sh

        # Install CPLEX.
        $CPLEX_INSTALLER -DLICENSE_ACCEPTED=TRUE -i silent
    fi

    # Set environment variables for SoPlex.
    cat > /etc/profile.d/downward-soplex.sh <<-EOM
		export soplex_DIR="/opt/soplex"
	EOM
    source /etc/profile.d/downward-soplex.sh
    git clone --depth 1 --branch release-710 https://github.com/scipopt/soplex.git soplex
    cd soplex
    cmake -DCMAKE_INSTALL_PREFIX="$soplex_DIR" -S . -B build
    cmake --build build
    cmake --install build

    cd /home/vagrant

    if ! [ -e downward ] ; then
        git clone --branch BRANCH https://github.com/aibasel/downward.git downward
        ./downward/build.py release debug
        chown -R vagrant.vagrant downward
    fi

  SHELL
end
