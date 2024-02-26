#! /bin/bash
sudo apt update

# Use hostnamectl command to set the hostname on master and worker nodes.
sudo hostnamectl set-hostname "k8s-master"
# sudo hostnamectl set-hostname "k8s-worker1"

# Disable Swap on all nodes
sudo swapoff -a
sudo sed -i '/ swap / s/^\(.*\)$/#\1/g' /etc/fstab

# Configure Firewall Rules for Kubernetes Cluster
sudo apt -y install ufw
sudo ufw enable
sudo ufw allow 6443/tcp
sudo ufw allow 2379/tcp
sudo ufw allow 2380/tcp
sudo ufw allow 10250/tcp
sudo ufw allow 10251/tcp
sudo ufw allow 10252/tcp
sudo ufw allow 10255/tcp

# Calico Pod Network Addon
sudo ufw allow 179/tcp
sudo ufw allow 4789/udp
sudo ufw allow 51820/udp
sudo ufw allow 51821/udp

sudo ufw allow ssh
sudo ufw reload


#4) Install Containerd run time on all nodes
cat <<EOF | sudo tee /etc/modules-load.d/containerd.conf
overlay
br_netfilter
EOF

sudo modprobe overlay
sudo modprobe br_netfilter

cat <<EOF | sudo tee /etc/sysctl.d/99-kubernetes-k8s.conf
net.bridge.bridge-nf-call-iptables = 1
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-ip6tables = 1
EOF

sudo sysctl --system


# 4.1) docker
sudo apt-get -y remove docker docker-engine docker.io containerd runc
sudo apt-get update
 sudo apt-get install \
    ca-certificates \
    curl \
    gnupg

sudo mkdir -m 0755 -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/debian/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

echo \
  "deb [arch="$(dpkg --print-architecture)" signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/debian \
  "$(. /etc/os-release && echo "$VERSION_CODENAME")" stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

sudo chmod a+r /etc/apt/keyrings/docker.gpg
sudo apt-get update
# sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
sudo apt-get install -y containerd.io
sudo containerd config default | sudo tee /etc/containerd/config.toml >/dev/null 2>&1

sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/g' /etc/containerd/config.toml
# Set cgroupdriver to systemd on all the nodes,
# Edit the file '/etc/containerd/config.toml' and look for the section '[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]' and add SystemdCgroup = true
# sudo nano /etc/containerd/config.toml
# SystemdCgroup = true


# 5) Enable Kubernetes Apt Repository
sudo apt-get update
sudo apt-get install -y apt-transport-https ca-certificates curl
sudo curl -fsSLo /etc/apt/keyrings/kubernetes-archive-keyring.gpg https://packages.cloud.google.com/apt/doc/apt-key.gpg
echo "deb [signed-by=/etc/apt/keyrings/kubernetes-archive-keyring.gpg] https://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee /etc/apt/sources.list.d/kubernetes.list



# 6) Install Kubelet, Kubectl and Kubeadm on all nodes
sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl
sudo apt-mark hold kubelet kubeadm kubectl


# 7) Create Kubernetes Cluster with Kubeadm (master only)
sudo kubeadm init --control-plane-endpoint=k8s-master

# To start interacting with cluster, run following commands on master node, (if you are root)
#export KUBECONFIG=/etc/kubernetes/admin.conf

# To start interacting with cluster, run following commands on master node,
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config


# Run following kubectl command to get nodes and cluster information,
sudo kubectl get nodes
sudo kubectl cluster-info


# 8) Install Calico Pod Network Addon (master only)
sudo kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.25.0/manifests/calico.yaml

# Verify the status of Calico pods, run
sudo kubectl get pods -n kube-system





