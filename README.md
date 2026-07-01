# Cloud-Based Parallel Commerce Analytics

## Overview

Cloud-based HPC benchmarking project that processes retail transaction datasets
using **MPI + OpenMP** hybrid parallelism. Compares serial vs parallel execution
time, speedup, and efficiency on a **Google Cloud Platform (GCP) Ubuntu VM**.

## Project Structure

```
HPPC Project/
├── src/
│   ├── serial_analytics.c     # Sequential baseline
│   └── mpi_analytics.c        # Hybrid MPI + OpenMP version
├── generator/
│   └── dataset_generator.py   # Generates synthetic transaction CSV
├── analysis/
│   └── graph.py               # Performance & analytics graphs
├── data/
│   └── dataset.csv            # Generated dataset (not in repo)
├── results/
│   ├── benchmark.csv          # Benchmark output (appended per run)
│   └── *.png                  # Generated graphs
└── README.md
```

## Requirements

### System

- Ubuntu 20.04+ (GCP VM)
- GCC with OpenMP support
- MPICH or OpenMPI

### Python (for dataset generation & graphs)

- Python 3.8+
- pandas
- matplotlib

### Install Dependencies (Ubuntu)

```bash
# C / MPI / OpenMP
sudo apt update
sudo apt install -y gcc mpich libmpich-dev

# Python
sudo apt install -y python3 python3-pip
pip3 install pandas matplotlib
```

## Quick Start

### 1. Generate Dataset

```bash
cd "HPPC Project"
python3 generator/dataset_generator.py
```

Edit the `generate(1000000)` call in `dataset_generator.py` to change size
(100000, 500000, 1000000, 5000000).

### 2. Compile

```bash
# Serial
gcc -o src/serial_analytics src/serial_analytics.c

# MPI + OpenMP
mpicc -fopenmp -o src/mpi_analytics src/mpi_analytics.c
```

### 3. Run Serial Benchmark

```bash
./src/serial_analytics
```

### 4. Run MPI + OpenMP Benchmark

```bash
# 1 process
mpirun -np 1 ./src/mpi_analytics

# 2 processes
mpirun -np 2 ./src/mpi_analytics

# 4 processes
mpirun -np 4 ./src/mpi_analytics

# 8 processes
mpirun -np 8 ./src/mpi_analytics
```

Control OpenMP threads per MPI process:

```bash
export OMP_NUM_THREADS=4
mpirun -np 2 ./src/mpi_analytics
```

### 5. Generate Graphs

```bash
python3 analysis/graph.py
```

Graphs are saved to `results/`.

## Benchmark Output Format

Each run appends a row to `results/benchmark.csv`:

```
Implementation,Dataset,Processes,ExecutionTime,Revenue,ATV,
Electronics,Fashion,Groceries,Johor,KL,Penang,Sabah,Sarawak,
Cash,Card,eWallet,Laptop,Phone,Shoes
```

## Full Benchmark Script

```bash
# Serial
./src/serial_analytics

# MPI (1, 2, 4, 8 processes)
mpirun -np 1 ./src/mpi_analytics
mpirun -np 2 ./src/mpi_analytics
mpirun -np 4 ./src/mpi_analytics
mpirun -np 8 ./src/mpi_analytics

# Graphs
python3 analysis/graph.py
```

## GCP Deployment

1. Go to [console.cloud.google.com](https://console.cloud.google.com) and create a new project
2. Navigate to **Compute Engine** → **VM Instances** → **Create Instance**
3. Configure the VM:
   - **Machine type**: `c2-standard-8` (8 vCPUs, 32 GB RAM) or `n2-standard-8`
   - **Boot disk**: Ubuntu 22.04 LTS
   - **Region**: closest to you (e.g. `asia-southeast1` for Malaysia)
4. Enable **HTTP/HTTPS traffic** under Firewall if needed
5. SSH into the VM directly from the GCP Console or via terminal:
   ```bash
   gcloud compute ssh <INSTANCE_NAME> --zone=<ZONE>
   ```
6. Install dependencies (see above)
7. Clone the project:
   ```bash
   git clone https://github.com/Jianming03/HPPC-Final-Project.git
   cd HPPC-Final-Project
   ```
8. Generate dataset, compile, and run benchmarks as above
9. Download results to your local machine:
   ```bash
   gcloud compute scp <INSTANCE_NAME>:~/HPPC-Final-Project/results/* ./results/ --zone=<ZONE>
   ```

> **Stop the VM when done** to avoid unnecessary charges:
> GCP Console → VM Instances → **Stop**, or run `sudo shutdown now` on the VM.

## Analytics Computed

| Metric                | Description                          |
|-----------------------|--------------------------------------|
| Total Revenue         | Sum of all total_price               |
| ATV                   | Total Revenue / Transaction Count    |
| Revenue by Category   | Electronics, Fashion, Groceries      |
| Revenue by Region     | Johor, KL, Penang, Sabah, Sarawak    |
| Payment Distribution  | Cash, Card, eWallet                  |
| Product Demand        | Laptop, Phone, Shoes                 |
