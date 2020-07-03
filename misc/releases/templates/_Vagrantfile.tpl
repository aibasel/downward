# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # For a complete reference of vagrant options see https://docs.vagrantup.com.

  config.vm.box = "ubuntu/bionic64"

  config.vm.provision "shell", inline: <<-SHELL
    apt update && apt install --no-install-recommends -y \
        cmake     \
        g++       \
        git       \
        make      \
        python3

    cd /home/vagrant

    if ! [ -e downward ] ; then
        git clone --branch TAG https://github.com/aibasel/downward.git
        ./downward/build.py
        chown -R vagrant.vagrant downward
    fi

  SHELL
end
