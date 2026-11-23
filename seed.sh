#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "=== SmartMart Data Seeder ==="
echo "Clearing existing data..."
rm -f data/*.dat
rm -f receipts/*.txt 2>/dev/null || true
rm -f shopping_lists/*.txt 2>/dev/null || true

echo "Rebuilding..."
make clean >/dev/null
make >/dev/null

echo "Running once to trigger seeding..."
printf "8\n" | ./smartmart > /dev/null

echo ""
echo "Seed complete."