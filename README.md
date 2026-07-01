# Cloud-Based Parallel Commerce Analytics

## Overview

Cloud-based HPC benchmarking project that processes retail transaction datasets
using **MPI + OpenMP** hybrid parallelism. Compares serial vs parallel execution
time, speedup, and efficiency on an Azure Ubuntu VM.

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

- Ubuntu 20.04+ (WSL or Azure VM)
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
gcc -O2 -o src/serial_analytics src/serial_analytics.c

# MPI + OpenMP
mpicc -O2 -fopenmp -o src/mpi_analytics src/mpi_analytics.c
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

**Clear benchmark.csv before a fresh benchmark run:**

```bash
rm -f results/benchmark.csv
```

## Full Benchmark Script

```bash
rm -f results/benchmark.csv

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

## Azure Deployment

1. Create an Ubuntu VM on Azure (Standard_D8s_v3 recommended for 8 vCPUs)
2. SSH into the VM
3. Install dependencies (see above)
4. Upload the project: `scp -r "HPPC Project" user@<vm-ip>:~/`
5. Generate dataset, compile, and run as above
6. Download results: `scp user@<vm-ip>:~/HPPC\ Project/results/* ./results/`

## Analytics Computed

| Metric                | Description                          |
|-----------------------|--------------------------------------|
| Total Revenue         | Sum of all total_price               |
| ATV                   | Total Revenue / Transaction Count    |
| Revenue by Category   | Electronics, Fashion, Groceries      |
| Revenue by Region     | Johor, KL, Penang, Sabah, Sarawak    |
| Payment Distribution  | Cash, Card, eWallet                  |
| Product Demand        | Laptop, Phone, Shoes                 |
