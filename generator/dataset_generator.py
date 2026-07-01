import csv
import random
import os
from datetime import datetime, timedelta

# ==========================
# Project Directories
# ==========================

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "..", "data")
os.makedirs(DATA_DIR, exist_ok=True)

OUTPUT_FILE = os.path.join(DATA_DIR, "dataset.csv")

# ==========================
# Product Catalog
# ==========================

products = [
    ("P1001", "Laptop", "Electronics", 3500),
    ("P1002", "Phone", "Electronics", 2500),
    ("P1003", "Headphones", "Electronics", 300),

    ("P2001", "Shoes", "Fashion", 150),
    ("P2002", "Shirt", "Fashion", 80),
    ("P2003", "Jacket", "Fashion", 180),

    ("P3001", "Rice", "Groceries", 40),
    ("P3002", "Cooking Oil", "Groceries", 35),
    ("P3003", "Milk", "Groceries", 15)
]

regions = [
    "Johor",
    "KL",
    "Penang",
    "Sabah",
    "Sarawak"
]

payments = [
    "Cash",
    "Card",
    "eWallet"
]

memberships = [
    "Regular",
    "Silver",
    "Gold"
]

# ==========================
# Random Timestamp
# ==========================

def random_datetime():
    start = datetime(2025, 1, 1)
    end = datetime(2025, 12, 31)

    delta = end - start
    seconds = random.randint(0, int(delta.total_seconds()))

    return (start + timedelta(seconds=seconds)).strftime("%Y-%m-%d %H:%M:%S")

# ==========================
# Dataset Generator
# ==========================

def generate(rows):

    with open(OUTPUT_FILE, "w", newline="", encoding="utf-8") as f:

        writer = csv.writer(f)

        writer.writerow([
            "transaction_id",
            "timestamp",
            "customer_id",
            "membership",
            "product_id",
            "product_name",
            "category",
            "region",
            "payment_method",
            "quantity",
            "unit_price",
            "discount",
            "total_price"
        ])

        for i in range(rows):

            pid, product, category, price = random.choice(products)

            quantity = random.randint(1, 5)

            discount = random.choice([
                0.00,
                0.05,
                0.10,
                0.15
            ])

            total = quantity * price * (1 - discount)

            writer.writerow([
                f"T{i+1:07d}",
                random_datetime(),
                f"C{random.randint(100000,999999)}",
                random.choice(memberships),
                pid,
                product,
                category,
                random.choice(regions),
                random.choice(payments),
                quantity,
                price,
                discount,
                round(total, 2)
            ])

    print("-----------------------------------")
    print("Dataset generated successfully!")
    print(f"Rows: {rows:,}")
    print(f"Location: {OUTPUT_FILE}")
    print("-----------------------------------")

# ==========================
# Main
# ==========================

if __name__ == "__main__":

    # Change this value easily for benchmarking
    generate(1000000)