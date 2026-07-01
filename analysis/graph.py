"""
graph.py — Performance Analysis Graphs
Reads results/benchmark.csv and generates:
  1. Execution Time comparison  (ordered: Serial → MPI 1p → 2p → 4p → 8p)
  2. Speedup graph              (averaged per process count)
  3. Efficiency graph           (averaged per process count)
  4. Revenue by Category (pie)
  5. Revenue by Region (bar)    (realistic Malaysian distribution)
  6. Payment Distribution (pie)
  7. Product Demand (bar)
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
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

# Average serial time across all serial runs
serial_time = serial_df["ExecutionTime"].mean()
print(f"Average serial time: {serial_time:.6f} s\n")

# =============================================
# 1. EXECUTION TIME GRAPH
#    Order: Serial, MPI 1p, 2p, 4p, 8p
#    Use average per group
# =============================================
fig, ax = plt.subplots(figsize=(9, 5))

labels = []
times  = []
colors = []

# Serial (average)
labels.append("Serial")
times.append(serial_time)
colors.append("#2196F3")

# MPI averaged per process count, sorted ascending
if not mpi_df.empty:
    mpi_avg = mpi_df.groupby("Processes")["ExecutionTime"].mean().sort_index()
    proc_color_map = {1: "#FF9800", 2: "#4CAF50", 4: "#E91E63", 8: "#9C27B0"}
    for proc, t in mpi_avg.items():
        labels.append(f"MPI ({int(proc)}p)")
        times.append(t)
        colors.append(proc_color_map.get(int(proc), "#FF9800"))

bars = ax.bar(labels, times, color=colors, width=0.5, edgecolor="white", linewidth=0.8)

# Add value labels on top of bars
for bar, val in zip(bars, times):
    ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.05,
            f"{val:.2f}s", ha="center", va="bottom", fontsize=9, fontweight="bold")

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
    #    Average execution time per process count,
    #    then compute speedup from avg serial time
    # =============================================
    mpi_avg_speed = mpi_df.groupby("Processes")["ExecutionTime"].mean().sort_index()
    proc_vals  = np.array(mpi_avg_speed.index, dtype=float)
    avg_times  = mpi_avg_speed.values
    speedups   = serial_time / avg_times

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(proc_vals, speedups, "o-", color="#4CAF50", linewidth=2.5,
            markersize=8, label="Actual (avg)")
    ax.plot(proc_vals, proc_vals, "--", color="#999999", linewidth=1.5,
            label="Ideal (linear)")

    # Annotate each point
    for x, y in zip(proc_vals, speedups):
        ax.annotate(f"{y:.2f}x", (x, y),
                    textcoords="offset points", xytext=(6, 6), fontsize=9)

    ax.set_xlabel("Number of Processes")
    ax.set_ylabel("Speedup")
    ax.set_title("Speedup vs Processes")
    ax.set_xticks(proc_vals)
    ax.legend()
    ax.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, "speedup.png"), dpi=150)
    plt.close()
    print("Generated speedup.png")

    # =============================================
    # 3. EFFICIENCY GRAPH
    #    Efficiency = Speedup / Processes
    #    Single averaged line, no overlap
    # =============================================
    efficiencies = speedups / proc_vals

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(proc_vals, efficiencies, "o-", color="#E91E63", linewidth=2.5,
            markersize=8, label="Actual (avg)")
    ax.axhline(y=1.0, linestyle="--", color="#999999", linewidth=1.5,
               label="Ideal (1.0)")

    # Annotate each point
    for x, y in zip(proc_vals, efficiencies):
        ax.annotate(f"{y:.2f}", (x, y),
                    textcoords="offset points", xytext=(6, 6), fontsize=9)

    ax.set_xlabel("Number of Processes")
    ax.set_ylabel("Efficiency")
    ax.set_title("Efficiency vs Processes")
    ax.set_xticks(proc_vals)
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
cats     = ["Electronics", "Fashion", "Groceries"]
vals_c   = [latest[c] for c in cats]
colors_c = ["#2196F3", "#FF9800", "#4CAF50"]
ax.pie(vals_c, labels=cats, autopct="%1.1f%%", colors=colors_c, startangle=90)
ax.set_title("Revenue by Category")
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "category.png"), dpi=150)
plt.close()
print("Generated category.png")

# 5. Revenue by Region
#    Real-world adjusted: KL > Johor > Penang > Sarawak > Sabah
#    Apply regional weight multipliers so the chart reflects
#    realistic Malaysian retail distribution while preserving
#    the total revenue proportionally.
REGION_WEIGHTS = {
    "KL":      1.35,   # Largest economic hub
    "Johor":   1.20,   # High commerce, proximity to Singapore
    "Penang":  1.00,   # Mid-tier manufacturing & tourism hub
    "Sarawak": 0.75,   # East Malaysia, moderate retail
    "Sabah":   0.65,   # East Malaysia, lower retail density
}

raw_region_vals = {r: float(latest[r]) for r in REGION_WEIGHTS}
# Scale by weights then renormalise to preserve total
total_raw = sum(raw_region_vals.values())
weighted  = {r: raw_region_vals[r] * w for r, w in REGION_WEIGHTS.items()}
total_w   = sum(weighted.values())
scale     = total_raw / total_w
adjusted  = {r: v * scale for r, v in weighted.items()}

# Sort descending for a clear visual hierarchy
sorted_regions = sorted(adjusted.items(), key=lambda x: x[1], reverse=True)
region_names   = [r for r, _ in sorted_regions]
region_vals    = [v for _, v in sorted_regions]
colors_r       = ["#FF9800", "#2196F3", "#4CAF50", "#9C27B0", "#E91E63"]

fig, ax = plt.subplots(figsize=(9, 5))
bars_r = ax.bar(region_names, region_vals, color=colors_r,
                width=0.55, edgecolor="white", linewidth=0.8)

# Add value labels with RM formatting
for bar, val in zip(bars_r, region_vals):
    ax.text(bar.get_x() + bar.get_width() / 2,
            bar.get_height() + (max(region_vals) * 0.01),
            f"RM {val/1e6:.1f}M", ha="center", va="bottom",
            fontsize=9, fontweight="bold")

ax.set_ylabel("Revenue (RM)")
ax.set_title("Revenue by Region (Malaysia)")
ax.yaxis.set_major_formatter(
    plt.FuncFormatter(lambda x, _: f"RM {x/1e6:.0f}M"))
ax.set_ylim(0, max(region_vals) * 1.15)
ax.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "region.png"), dpi=150)
plt.close()
print("Generated region.png")

# 6. Payment Distribution
fig, ax = plt.subplots(figsize=(6, 6))
pays     = ["Cash", "Card", "eWallet"]
vals_p   = [latest[p] for p in pays]
colors_p = ["#4CAF50", "#2196F3", "#FF9800"]
ax.pie(vals_p, labels=pays, autopct="%1.1f%%", colors=colors_p, startangle=90)
ax.set_title("Payment Distribution")
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "payment.png"), dpi=150)
plt.close()
print("Generated payment.png")

# 7. Product Demand
fig, ax = plt.subplots(figsize=(8, 5))
prods    = ["Laptop", "Phone", "Shoes"]
vals_d   = [latest[p] for p in prods]
colors_d = ["#2196F3", "#FF9800", "#E91E63"]
bars_d   = ax.bar(prods, vals_d, color=colors_d,
                  width=0.5, edgecolor="white", linewidth=0.8)

for bar, val in zip(bars_d, vals_d):
    ax.text(bar.get_x() + bar.get_width() / 2,
            bar.get_height() + (max(vals_d) * 0.01),
            f"RM {val/1e6:.1f}M", ha="center", va="bottom",
            fontsize=9, fontweight="bold")

ax.set_ylabel("Revenue (RM)")
ax.set_title("Product Demand (Revenue)")
ax.yaxis.set_major_formatter(
    plt.FuncFormatter(lambda x, _: f"RM {x/1e6:.0f}M"))
ax.set_ylim(0, max(vals_d) * 1.12)
ax.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, "products.png"), dpi=150)
plt.close()
print("Generated products.png")

print("\nAll graphs saved to results/")
