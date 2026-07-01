"""
graph.py — Performance Analysis Graphs
Reads results/benchmark.csv and generates:
  1. Execution Time comparison
  2. Speedup graph
  3. Efficiency graph
  4. Revenue by Category (pie)
  5. Revenue by Region (bar)
  6. Payment Distribution (pie)
  7. Product Demand (bar)
"""

import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

# ---- Setup ----
RESULTS_DIR = "results"
CSV_PATH    = os.path.join(RESULTS_DIR, "benchmark.csv")

if not os.path.exists(CSV_PATH):
    print(f"Error: {CSV_PATH} not found. Run serial and MPI benchmarks first.")
    sys.exit(1)

df = pd.read_csv(CSV_PATH)
print(f"Loaded {len(df)} rows from {CSV_PATH}")
print(df.to_string(index=False))
print()

# ---- Filter serial and MPI rows ----
serial_df = df[df["Implementation"] == "serial"]
mpi_df    = df[df["Implementation"] == "mpi"].copy()

if serial_df.empty:
    print("Error: no serial results found in benchmark.csv")
    sys.exit(1)

if mpi_df.empty:
    print("Warning: no MPI results found. Only generating execution time graph.")

# Use the latest serial run as baseline
serial_time = serial_df.iloc[-1]["ExecutionTime"]
print(f"Serial baseline time: {serial_time:.6f} s\n")

# =============================================
# 1. EXECUTION TIME GRAPH
# =============================================
fig, ax = plt.subplots(figsize=(8, 5))

# Serial bar
ax.bar("Serial (1)", serial_time, color="#2196F3", width=0.5)

# MPI bars
if not mpi_df.empty:
    for _, row in mpi_df.iterrows():
        label = f"MPI ({int(row['Processes'])}p)"
        ax.bar(label, row["ExecutionTime"], color="#FF9800", width=0.5)

ax.set_ylabel("Execution Time (seconds)")
ax.set_title("Execution Time Comparison")
ax.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "execution_time.png"), dpi=150)
plt.close()
print("Generated execution_time.png")

if not mpi_df.empty:

    # =============================================
    # 2. SPEEDUP GRAPH
    # =============================================
    mpi_df["Speedup"] = serial_time / mpi_df["ExecutionTime"]

    fig, ax = plt.subplots(figsize=(8, 5))
    procs = mpi_df["Processes"].values
    speedups = mpi_df["Speedup"].values

    ax.plot(procs, speedups, "o-", color="#4CAF50", linewidth=2, label="Actual")
    ax.plot(procs, procs, "--", color="#999999", linewidth=1, label="Ideal (linear)")
    ax.set_xlabel("Number of Processes")
    ax.set_ylabel("Speedup")
    ax.set_title("Speedup vs Processes")
    ax.set_xticks(procs)
    ax.legend()
    ax.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, "speedup.png"), dpi=150)
    plt.close()
    print("Generated speedup.png")

    # =============================================
    # 3. EFFICIENCY GRAPH
    # =============================================
    mpi_df["Efficiency"] = mpi_df["Speedup"] / mpi_df["Processes"]

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(procs, mpi_df["Efficiency"].values, "o-", color="#E91E63", linewidth=2)
    ax.axhline(y=1.0, linestyle="--", color="#999999", linewidth=1, label="Ideal (1.0)")
    ax.set_xlabel("Number of Processes")
    ax.set_ylabel("Efficiency")
    ax.set_title("Efficiency vs Processes")
    ax.set_xticks(procs)
    ax.set_ylim(0, 1.5)
    ax.legend()
    ax.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, "efficiency.png"), dpi=150)
    plt.close()
    print("Generated efficiency.png")

# =============================================
# 4-7. BUSINESS ANALYTICS CHARTS
# Use the latest row with full data
# =============================================
latest = df.iloc[-1]

# 4. Revenue by Category
fig, ax = plt.subplots(figsize=(6, 6))
cats   = ["Electronics", "Fashion", "Groceries"]
vals_c = [latest[c] for c in cats]
colors_c = ["#2196F3", "#FF9800", "#4CAF50"]
ax.pie(vals_c, labels=cats, autopct="%1.1f%%", colors=colors_c, startangle=90)
ax.set_title("Revenue by Category")
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "category.png"), dpi=150)
plt.close()
print("Generated category.png")

# 5. Revenue by Region
fig, ax = plt.subplots(figsize=(8, 5))
regions  = ["Johor", "KL", "Penang", "Sabah", "Sarawak"]
vals_r   = [latest[r] for r in regions]
colors_r = ["#2196F3", "#FF9800", "#4CAF50", "#E91E63", "#9C27B0"]
ax.bar(regions, vals_r, color=colors_r)
ax.set_ylabel("Revenue (RM)")
ax.set_title("Revenue by Region")
ax.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "region.png"), dpi=150)
plt.close()
print("Generated region.png")

# 6. Payment Distribution
fig, ax = plt.subplots(figsize=(6, 6))
pays   = ["Cash", "Card", "eWallet"]
vals_p = [latest[p] for p in pays]
colors_p = ["#4CAF50", "#2196F3", "#FF9800"]
ax.pie(vals_p, labels=pays, autopct="%1.1f%%", colors=colors_p, startangle=90)
ax.set_title("Payment Distribution")
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "payment.png"), dpi=150)
plt.close()
print("Generated payment.png")

# 7. Product Demand
fig, ax = plt.subplots(figsize=(8, 5))
prods  = ["Laptop", "Phone", "Shoes"]
vals_d = [latest[p] for p in prods]
colors_d = ["#2196F3", "#FF9800", "#E91E63"]
ax.bar(prods, vals_d, color=colors_d)
ax.set_ylabel("Revenue (RM)")
ax.set_title("Product Demand (Revenue)")
ax.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "products.png"), dpi=150)
plt.close()
print("Generated products.png")

print("\nAll graphs saved to results/")
