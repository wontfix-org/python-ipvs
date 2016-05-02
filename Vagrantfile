# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "debian/jessie64"
  config.vm.synced_folder ".", "/vagrant", type: "virtualbox"
  config.vm.provision "shell", inline: <<-SHELL
	sudo sed -i 's/main/main contrib non-free/' /etc/apt/sources.list
    apt-get update
	apt-get dist-upgrade
    apt-get install -y --force-yes python-tox python-all-dev ipvsadm build-essential dh-python virtualenvwrapper vim tig git
	update-rc.d sshd enable
  SHELL
end
