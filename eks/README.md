# EKS Configuration

This directory contains configuration files for deploying the raycast-fps project to Amazon EKS (Elastic Kubernetes Service).

## Files

- `eks-cluster-config.yaml` - EKS cluster configuration for eksctl

## Cluster Configuration

The cluster is configured with:

- **Name**: raycast-cluster
- **Region**: us-west-2
- **Kubernetes Version**: 1.28
- **Node Group**: raycast-workers
  - Instance Type: t3.medium
  - Desired Capacity: 3
  - Min Size: 2
  - Max Size: 10

## Creating the Cluster

To create the EKS cluster:

```bash
eksctl create cluster -f eks/eks-cluster-config.yaml
```

This will:

1. Create the EKS cluster in AWS
2. Set up worker nodes
3. Configure kubectl to access the cluster
4. Install required addons (VPC CNI, CoreDNS, kube-proxy, EBS CSI driver)

## Deleting the Cluster

To delete the cluster and clean up resources:

```bash
eksctl delete cluster --name raycast-cluster --region us-west-2
```

## Prerequisites

- AWS CLI configured with appropriate permissions
- eksctl installed
- kubectl installed
