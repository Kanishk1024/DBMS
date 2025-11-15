#!/usr/bin/env python3
"""
Generate performance graphs for buffer management comparison (LRU vs MRU)
Supports both real data format (Dataset,Workload) and synthetic format (ReadPct,WritePct)
"""
import pandas as pd
import matplotlib.pyplot as plt
import sys
import os
import numpy as np

def plot_realdata_graphs(lru_data, mru_data, output_dir='graphs'):
    """Generate graphs for real database with read/write mixtures"""
    
    # Get unique datasets
    datasets = lru_data['Dataset'].unique()
    
    # 1. Hit Ratio vs Read/Write Mixture (Per Dataset)
    num_datasets = len(datasets)
    fig, axes = plt.subplots(2, 3, figsize=(18, 10))
    axes = axes.flatten()
    
    for idx, dataset in enumerate(datasets):
        if idx >= len(axes):
            break
        ax = axes[idx]
        lru_subset = lru_data[lru_data['Dataset'] == dataset]
        mru_subset = mru_data[mru_data['Dataset'] == dataset]
        
        ax.plot(lru_subset['ReadPct'], lru_subset['HitRatio'], 
                'b-o', label='LRU', linewidth=2.5, markersize=6)
        ax.plot(mru_subset['ReadPct'], mru_subset['HitRatio'], 
                'r-s', label='MRU', linewidth=2.5, markersize=6)
        
        ax.set_xlabel('Read Percentage (%)', fontsize=11)
        ax.set_ylabel('Hit Ratio', fontsize=11)
        ax.set_title(f'{dataset.replace(".txt", "")}', fontsize=12, fontweight='bold')
        ax.legend(fontsize=10)
        ax.grid(True, alpha=0.3)
    
    # Hide unused subplots
    for idx in range(len(datasets), len(axes)):
        axes[idx].set_visible(False)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/hit_ratio_vs_mixture.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/hit_ratio_vs_mixture.png")
    plt.close()
    
    # 2. Physical I/O vs Read/Write Mixture (Aggregated across all datasets)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    # Group by read percentage and sum physical I/O
    lru_grouped = lru_data.groupby('ReadPct').agg({
        'PhysicalReads': 'sum',
        'PhysicalWrites': 'sum'
    }).reset_index()
    
    mru_grouped = mru_data.groupby('ReadPct').agg({
        'PhysicalReads': 'sum',
        'PhysicalWrites': 'sum'
    }).reset_index()
    
    ax1.plot(lru_grouped['ReadPct'], lru_grouped['PhysicalReads'], 
             'b-o', label='Physical Reads', linewidth=2, markersize=6)
    ax1.plot(lru_grouped['ReadPct'], lru_grouped['PhysicalWrites'], 
             'g--s', label='Physical Writes', linewidth=2, markersize=6)
    ax1.set_xlabel('Read Percentage (%)', fontsize=12)
    ax1.set_ylabel('Physical I/O Operations', fontsize=12)
    ax1.set_title('LRU Strategy: Physical I/O vs Read/Write Mix', fontsize=13, fontweight='bold')
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    ax2.plot(mru_grouped['ReadPct'], mru_grouped['PhysicalReads'], 
             'b-o', label='Physical Reads', linewidth=2, markersize=6)
    ax2.plot(mru_grouped['ReadPct'], mru_grouped['PhysicalWrites'], 
             'g--s', label='Physical Writes', linewidth=2, markersize=6)
    ax2.set_xlabel('Read Percentage (%)', fontsize=12)
    ax2.set_ylabel('Physical I/O Operations', fontsize=12)
    ax2.set_title('MRU Strategy: Physical I/O vs Read/Write Mix', fontsize=13, fontweight='bold')
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/physical_io_vs_mixture.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/physical_io_vs_mixture.png")
    plt.close()
    
    # 3. Average Hit Ratio Comparison (LRU vs MRU across all datasets)
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    # Average hit ratio across all datasets
    lru_avg = lru_data.groupby('ReadPct')['HitRatio'].mean().reset_index()
    mru_avg = mru_data.groupby('ReadPct')['HitRatio'].mean().reset_index()
    
    ax1 = axes[0]
    ax1.plot(lru_avg['ReadPct'], lru_avg['HitRatio'], 
             'b-o', label='LRU', linewidth=2.5, markersize=8)
    ax1.plot(mru_avg['ReadPct'], mru_avg['HitRatio'], 
             'r-s', label='MRU', linewidth=2.5, markersize=8)
    ax1.set_xlabel('Read Percentage (%)', fontsize=12)
    ax1.set_ylabel('Average Hit Ratio', fontsize=12)
    ax1.set_title('Average Hit Ratio: LRU vs MRU', fontsize=13, fontweight='bold')
    ax1.legend(fontsize=11)
    ax1.grid(True, alpha=0.3)
    
    # Total I/O comparison
    lru_data['TotalIO'] = lru_data['PhysicalReads'] + lru_data['PhysicalWrites']
    mru_data['TotalIO'] = mru_data['PhysicalReads'] + mru_data['PhysicalWrites']
    
    lru_io = lru_data.groupby('ReadPct')['TotalIO'].sum().reset_index()
    mru_io = mru_data.groupby('ReadPct')['TotalIO'].sum().reset_index()
    
    ax2 = axes[1]
    ax2.plot(lru_io['ReadPct'], lru_io['TotalIO'], 
             'b-o', label='LRU', linewidth=2.5, markersize=8)
    ax2.plot(mru_io['ReadPct'], mru_io['TotalIO'], 
             'r-s', label='MRU', linewidth=2.5, markersize=8)
    ax2.set_xlabel('Read Percentage (%)', fontsize=12)
    ax2.set_ylabel('Total Physical I/O', fontsize=12)
    ax2.set_title('Total Physical I/O: LRU vs MRU', fontsize=13, fontweight='bold')
    ax2.legend(fontsize=11)
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/strategy_comparison.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/strategy_comparison.png")
    plt.close()
    
    # 4. Buffer Hits vs Misses (Aggregated)
    lru_grouped = lru_data.groupby('ReadPct').agg({
        'BufferHits': 'sum',
        'BufferMisses': 'sum'
    }).reset_index()
    
    mru_grouped = mru_data.groupby('ReadPct').agg({
        'BufferHits': 'sum',
        'BufferMisses': 'sum'
    }).reset_index()
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    x = np.arange(len(lru_grouped))
    width = 0.35
    
    ax1.bar(x - width/2, lru_grouped['BufferHits'], width, label='Hits', color='green', alpha=0.7)
    ax1.bar(x + width/2, lru_grouped['BufferMisses'], width, label='Misses', color='red', alpha=0.7)
    ax1.set_xlabel('Read/Write Mixture', fontsize=11)
    ax1.set_ylabel('Count', fontsize=11)
    ax1.set_title('LRU: Buffer Hits vs Misses', fontsize=13, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels([f"{r}%R" for r in lru_grouped['ReadPct']], fontsize=9)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3, axis='y')
    
    ax2.bar(x - width/2, mru_grouped['BufferHits'], width, label='Hits', color='green', alpha=0.7)
    ax2.bar(x + width/2, mru_grouped['BufferMisses'], width, label='Misses', color='red', alpha=0.7)
    ax2.set_xlabel('Read/Write Mixture', fontsize=11)
    ax2.set_ylabel('Count', fontsize=11)
    ax2.set_title('MRU: Buffer Hits vs Misses', fontsize=13, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels([f"{r}%R" for r in mru_grouped['ReadPct']], fontsize=9)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/buffer_performance.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/buffer_performance.png")
    plt.close()

def plot_comparison_graphs(lru_file, mru_file, output_dir='graphs'):
    """Generate comparison graphs from LRU and MRU CSV files"""
    
    # Create output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Read CSV files
    try:
        lru_data = pd.read_csv(lru_file)
        mru_data = pd.read_csv(mru_file)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        print("Please run the test programs first to generate CSV files")
        sys.exit(1)
    
    # Detect format based on columns
    if 'Dataset' in lru_data.columns and 'ReadPct' in lru_data.columns:
        print("Detected real data format - generating read/write mixture graphs...")
        plot_realdata_graphs(lru_data, mru_data, output_dir)
        print(f"\n{'='*60}")
        print("All graphs generated successfully!")
        print(f"{'='*60}")
        print(f"\nGraphs saved in '{output_dir}/' directory:")
        print("  1. hit_ratio_vs_mixture.png - Hit ratio vs read/write mix (per dataset)")
        print("  2. physical_io_vs_mixture.png - Physical I/O vs read/write mix")
        print("  3. strategy_comparison.png - LRU vs MRU comparison")
        print("  4. buffer_performance.png - Buffer hits vs misses")
        return
    
    # Original synthetic data format
    # Set style
    plt.style.use('seaborn-v0_8-darkgrid')
    
    # 1. Physical I/O Comparison
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    ax1.plot(lru_data['ReadPct'], lru_data['PhysicalReads'], 
             'b-o', label='Physical Reads', linewidth=2, markersize=6)
    ax1.plot(lru_data['ReadPct'], lru_data['PhysicalWrites'], 
             'r--s', label='Physical Writes', linewidth=2, markersize=6)
    ax1.set_xlabel('Read Percentage (%)', fontsize=12)
    ax1.set_ylabel('Physical I/O Operations', fontsize=12)
    ax1.set_title('LRU Strategy: Physical I/O vs Read/Write Mix', fontsize=14, fontweight='bold')
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    ax2.plot(mru_data['ReadPct'], mru_data['PhysicalReads'], 
             'b-o', label='Physical Reads', linewidth=2, markersize=6)
    ax2.plot(mru_data['ReadPct'], mru_data['PhysicalWrites'], 
             'r--s', label='Physical Writes', linewidth=2, markersize=6)
    ax2.set_xlabel('Read Percentage (%)', fontsize=12)
    ax2.set_ylabel('Physical I/O Operations', fontsize=12)
    ax2.set_title('MRU Strategy: Physical I/O vs Read/Write Mix', fontsize=14, fontweight='bold')
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/physical_io_comparison.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/physical_io_comparison.png")
    plt.close()
    
    # 2. Hit Ratio Comparison
    plt.figure(figsize=(10, 6))
    plt.plot(lru_data['ReadPct'], lru_data['HitRatio'], 
             'b-o', label='LRU', linewidth=2.5, markersize=8)
    plt.plot(mru_data['ReadPct'], mru_data['HitRatio'], 
             'r-s', label='MRU', linewidth=2.5, markersize=8)
    plt.xlabel('Read Percentage (%)', fontsize=12)
    plt.ylabel('Buffer Hit Ratio', fontsize=12)
    plt.title('Buffer Hit Ratio: LRU vs MRU', fontsize=14, fontweight='bold')
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(f'{output_dir}/hit_ratio_comparison.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/hit_ratio_comparison.png")
    plt.close()
    
    # 3. Buffer Performance (Hits vs Misses)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    x = range(len(lru_data))
    width = 0.35
    
    ax1.bar([i - width/2 for i in x], lru_data['BufferHits'], 
            width, label='Hits', color='green', alpha=0.7)
    ax1.bar([i + width/2 for i in x], lru_data['BufferMisses'], 
            width, label='Misses', color='red', alpha=0.7)
    ax1.set_xlabel('Read/Write Mixture', fontsize=12)
    ax1.set_ylabel('Count', fontsize=12)
    ax1.set_title('LRU: Buffer Hits vs Misses', fontsize=14, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels([f"{r}%R\n{w}%W" for r, w in 
                          zip(lru_data['ReadPct'], lru_data['WritePct'])], 
                         fontsize=8)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3, axis='y')
    
    ax2.bar([i - width/2 for i in x], mru_data['BufferHits'], 
            width, label='Hits', color='green', alpha=0.7)
    ax2.bar([i + width/2 for i in x], mru_data['BufferMisses'], 
            width, label='Misses', color='red', alpha=0.7)
    ax2.set_xlabel('Read/Write Mixture', fontsize=12)
    ax2.set_ylabel('Count', fontsize=12)
    ax2.set_title('MRU: Buffer Hits vs Misses', fontsize=14, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels([f"{r}%R\n{w}%W" for r, w in 
                          zip(mru_data['ReadPct'], mru_data['WritePct'])], 
                         fontsize=8)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/buffer_performance.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/buffer_performance.png")
    plt.close()
    
    # 4. Total I/O Comparison
    plt.figure(figsize=(10, 6))
    lru_total_io = lru_data['PhysicalReads'] + lru_data['PhysicalWrites']
    mru_total_io = mru_data['PhysicalReads'] + mru_data['PhysicalWrites']
    
    plt.plot(lru_data['ReadPct'], lru_total_io, 
             'b-o', label='LRU Total I/O', linewidth=2.5, markersize=8)
    plt.plot(mru_data['ReadPct'], mru_total_io, 
             'r-s', label='MRU Total I/O', linewidth=2.5, markersize=8)
    plt.xlabel('Read Percentage (%)', fontsize=12)
    plt.ylabel('Total Physical I/O Operations', fontsize=12)
    plt.title('Total Physical I/O: LRU vs MRU', fontsize=14, fontweight='bold')
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(f'{output_dir}/total_io_comparison.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/total_io_comparison.png")
    plt.close()
    
    # 5. Summary Statistics Table
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.axis('tight')
    ax.axis('off')
    
    summary_data = []
    for i in range(len(lru_data)):
        summary_data.append([
            f"{lru_data.iloc[i]['ReadPct']}% / {lru_data.iloc[i]['WritePct']}%",
            f"{lru_data.iloc[i]['HitRatio']:.4f}",
            f"{mru_data.iloc[i]['HitRatio']:.4f}",
            f"{lru_data.iloc[i]['PhysicalReads'] + lru_data.iloc[i]['PhysicalWrites']}",
            f"{mru_data.iloc[i]['PhysicalReads'] + mru_data.iloc[i]['PhysicalWrites']}"
        ])
    
    table = ax.table(cellText=summary_data,
                     colLabels=['Read/Write Mix', 'LRU Hit Ratio', 'MRU Hit Ratio', 
                               'LRU Total I/O', 'MRU Total I/O'],
                     cellLoc='center',
                     loc='center',
                     colWidths=[0.15, 0.15, 0.15, 0.15, 0.15])
    
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1, 2)
    
    # Color header
    for i in range(5):
        table[(0, i)].set_facecolor('#4CAF50')
        table[(0, i)].set_text_props(weight='bold', color='white')
    
    plt.title('Performance Summary: LRU vs MRU', 
              fontsize=14, fontweight='bold', pad=20)
    plt.savefig(f'{output_dir}/summary_table.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_dir}/summary_table.png")
    plt.close()
    
    print("\n" + "="*60)
    print("All graphs generated successfully!")
    print("="*60)
    print(f"\nGraphs saved in '{output_dir}/' directory:")
    print("  1. physical_io_comparison.png - Physical I/O patterns")
    print("  2. hit_ratio_comparison.png - Hit ratio comparison")
    print("  3. buffer_performance.png - Hits vs Misses")
    print("  4. total_io_comparison.png - Total I/O comparison")
    print("  5. summary_table.png - Performance summary table")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python3 generate_graphs.py <lru_csv> <mru_csv>")
        print("Example: python3 generate_graphs.py lru_results.csv mru_results.csv")
        sys.exit(1)
    
    lru_file = sys.argv[1]
    mru_file = sys.argv[2]
    
    plot_comparison_graphs(lru_file, mru_file)
