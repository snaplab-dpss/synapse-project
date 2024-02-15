# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # config.vm.box = "ubuntu/lunar64"
  config.vm.box = "ubuntu/focal64"
  
  # If true, then any SSH connections made will enable agent forwarding.
  # Default value: false
  config.ssh.forward_agent = true
  
  config.vm.synced_folder "../synapse", "/home/vagrant/synapse"
  
  config.vm.provider :virtualbox do |vb|
    vb.name = "synapse"
    vb.memory = 8192
    vb.cpus = 8
  end

end
