# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.define "ledgerd" do |ledgerd|
    ledgerd.vm.hostname = "ledgerd.blakesmith.me"
    ledgerd.vm.box = "ubuntu/trusty64"
    ledgerd.vm.network "private_network", ip: "192.168.55.10"
    ledgerd.vm.provider "virtualbox" do |vb|
      vb.gui = false
    end
    ledgerd.vm.provision "puppet" do |puppet|
      puppet.manifests_path = "puppet/manifests"
      puppet.module_path = "puppet/modules"
      puppet.manifest_file  = "ledgerd.pp"
    end
  end
end
