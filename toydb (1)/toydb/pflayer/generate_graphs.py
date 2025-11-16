#!/usr/bin/env python3
"""
Create comparison plots for buffer management experiments (LRU vs MRU).
Handles both 'real data' CSVs (with Dataset, ReadPct) and the synthetic format.
"""
import os
import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def _draw_real_dataset_plots(lru_df, mru_df, dest='graphs'):
    """Produce a set of plots when CSVs include per-dataset mixtures."""
    datasets = lru_df['Dataset'].unique()
    # 1) Per-dataset hit ratio vs read mix (grid)
    cols = 3
    rows = 2
    fig, axes = plt.subplots(rows, cols, figsize=(18, 10))
    axes = axes.flatten()
    for i, ds in enumerate(datasets):
        if i >= len(axes):
            break
        ax = axes[i]
        lsub = lru_df[lru_df['Dataset'] == ds]
        msub = mru_df[mru_df['Dataset'] == ds]
        ax.plot(lsub['ReadPct'], lsub['HitRatio'], 'b-o', label='LRU', linewidth=2.5, markersize=6)
        ax.plot(msub['ReadPct'], msub['HitRatio'], 'r-s', label='MRU', linewidth=2.5, markersize=6)
        ax.set_xlabel('Read %')
        ax.set_ylabel('Hit Ratio')
        ax.set_title(ds.replace(".txt", ""), fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(alpha=0.3)
    for j in range(len(datasets), len(axes)):
        axes[j].set_visible(False)
    plt.tight_layout()
    os.makedirs(dest, exist_ok=True)
    out1 = os.path.join(dest, 'hit_ratio_vs_mixture.png')
    plt.savefig(out1, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out1}")
    plt.close()

    # 2) Aggregate physical I/O by read percentage
    l_grp = lru_df.groupby('ReadPct')[['PhysicalReads', 'PhysicalWrites']].sum().reset_index()
    m_grp = mru_df.groupby('ReadPct')[['PhysicalReads', 'PhysicalWrites']].sum().reset_index()
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(14, 5))
    a1.plot(l_grp['ReadPct'], l_grp['PhysicalReads'], 'b-o', label='Physical Reads', linewidth=2)
    a1.plot(l_grp['ReadPct'], l_grp['PhysicalWrites'], 'g--s', label='Physical Writes', linewidth=2)
    a1.set_xlabel('Read %'); a1.set_ylabel('Physical I/O'); a1.set_title('LRU: Physical I/O'); a1.grid(alpha=0.3); a1.legend()
    a2.plot(m_grp['ReadPct'], m_grp['PhysicalReads'], 'b-o', linewidth=2)
    a2.plot(m_grp['ReadPct'], m_grp['PhysicalWrites'], 'g--s', linewidth=2)
    a2.set_xlabel('Read %'); a2.set_ylabel('Physical I/O'); a2.set_title('MRU: Physical I/O'); a2.grid(alpha=0.3); a2.legend()
    plt.tight_layout()
    out2 = os.path.join(dest, 'physical_io_vs_mixture.png')
    plt.savefig(out2, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out2}")
    plt.close()

    # 3) Average hit ratio and total physical I/O comparison
    l_avg = lru_df.groupby('ReadPct')['HitRatio'].mean().reset_index()
    m_avg = mru_df.groupby('ReadPct')['HitRatio'].mean().reset_index()
    lru_df['TotalIO'] = lru_df['PhysicalReads'] + lru_df['PhysicalWrites']
    mru_df['TotalIO'] = mru_df['PhysicalReads'] + mru_df['PhysicalWrites']
    l_io = lru_df.groupby('ReadPct')['TotalIO'].sum().reset_index()
    m_io = mru_df.groupby('ReadPct')['TotalIO'].sum().reset_index()

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    ax1.plot(l_avg['ReadPct'], l_avg['HitRatio'], 'b-o', label='LRU', linewidth=2.5)
    ax1.plot(m_avg['ReadPct'], m_avg['HitRatio'], 'r-s', label='MRU', linewidth=2.5)
    ax1.set_xlabel('Read %'); ax1.set_ylabel('Avg Hit Ratio'); ax1.set_title('Average Hit Ratio: LRU vs MRU'); ax1.grid(alpha=0.3); ax1.legend()
    ax2.plot(l_io['ReadPct'], l_io['TotalIO'], 'b-o', label='LRU', linewidth=2.5)
    ax2.plot(m_io['ReadPct'], m_io['TotalIO'], 'r-s', label='MRU', linewidth=2.5)
    ax2.set_xlabel('Read %'); ax2.set_ylabel('Total Physical I/O'); ax2.set_title('Total Physical I/O: LRU vs MRU'); ax2.grid(alpha=0.3); ax2.legend()
    plt.tight_layout()
    out3 = os.path.join(dest, 'strategy_comparison.png')
    plt.savefig(out3, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out3}")
    plt.close()

    # 4) Hits vs Misses stacked bars (aggregated)
    l_hitsmiss = lru_df.groupby('ReadPct')[['BufferHits', 'BufferMisses']].sum().reset_index()
    m_hitsmiss = mru_df.groupby('ReadPct')[['BufferHits', 'BufferMisses']].sum().reset_index()
    fig, (b1, b2) = plt.subplots(1, 2, figsize=(14, 5))
    x = np.arange(len(l_hitsmiss))
    w = 0.35
    b1.bar(x - w/2, l_hitsmiss['BufferHits'], w, color='green', alpha=0.7, label='Hits')
    b1.bar(x + w/2, l_hitsmiss['BufferMisses'], w, color='red', alpha=0.7, label='Misses')
    b1.set_xticks(x); b1.set_xticklabels([f"{r}%R" for r in l_hitsmiss['ReadPct']]); b1.set_title('LRU: Hits vs Misses'); b1.grid(axis='y', alpha=0.3); b1.legend()
    b2.bar(x - w/2, m_hitsmiss['BufferHits'], w, color='green', alpha=0.7)
    b2.bar(x + w/2, m_hitsmiss['BufferMisses'], w, color='red', alpha=0.7)
    b2.set_xticks(x); b2.set_xticklabels([f"{r}%R" for r in m_hitsmiss['ReadPct']]); b2.set_title('MRU: Hits vs Misses'); b2.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    out4 = os.path.join(dest, 'buffer_performance.png')
    plt.savefig(out4, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out4}")
    plt.close()

def generate_plots(lru_csv, mru_csv, dest='graphs'):
    """Read two CSVs and branch to the appropriate plotting routines."""
    if not os.path.exists(dest):
        os.makedirs(dest, exist_ok=True)
    try:
        lru = pd.read_csv(lru_csv)
        mru = pd.read_csv(mru_csv)
    except FileNotFoundError as err:
        print(f"Error: {err}")
        print("Run the benchmarking utilities to create the CSV files first.")
        sys.exit(1)

    # If the CSVs include per-dataset rows, call the specialized routine
    if 'Dataset' in lru.columns and 'ReadPct' in lru.columns:
        print("Real-data CSV format detected — generating detailed mixture plots...")
        _draw_real_dataset_plots(lru, mru, dest)
        print("\n" + "="*60)
        print("All graphs generated successfully!")
        print("="*60)
        return

    # Fallback: synthetic/simple CSV format plotting
    plt.style.use('seaborn-v0_8-darkgrid')

    # Physical I/O comparison
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    ax1.plot(lru['ReadPct'], lru['PhysicalReads'], 'b-o', label='Physical Reads', linewidth=2)
    ax1.plot(lru['ReadPct'], lru['PhysicalWrites'], 'r--s', label='Physical Writes', linewidth=2)
    ax1.set_xlabel('Read %'); ax1.set_ylabel('Physical I/O'); ax1.set_title('LRU: Physical I/O'); ax1.grid(alpha=0.3); ax1.legend()
    ax2.plot(mru['ReadPct'], mru['PhysicalReads'], 'b-o', linewidth=2)
    ax2.plot(mru['ReadPct'], mru['PhysicalWrites'], 'r--s', linewidth=2)
    ax2.set_xlabel('Read %'); ax2.set_ylabel('Physical I/O'); ax2.set_title('MRU: Physical I/O'); ax2.grid(alpha=0.3); ax2.legend()
    plt.tight_layout()
    p1 = os.path.join(dest, 'physical_io_comparison.png')
    plt.savefig(p1, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {p1}")
    plt.close()

    # Hit ratio comparison
    plt.figure(figsize=(10, 6))
    plt.plot(lru['ReadPct'], lru['HitRatio'], 'b-o', label='LRU', linewidth=2.5)
    plt.plot(mru['ReadPct'], mru['HitRatio'], 'r-s', label='MRU', linewidth=2.5)
    plt.xlabel('Read %'); plt.ylabel('Hit Ratio'); plt.title('Hit Ratio: LRU vs MRU'); plt.grid(alpha=0.3); plt.legend()
    out_hr = os.path.join(dest, 'hit_ratio_comparison.png')
    plt.savefig(out_hr, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out_hr}")
    plt.close()

    # Buffer hits vs misses
    fig, (c1, c2) = plt.subplots(1, 2, figsize=(14, 5))
    x = range(len(lru))
    width = 0.35
    c1.bar([i - width/2 for i in x], lru['BufferHits'], width, color='green', alpha=0.7, label='Hits')
    c1.bar([i + width/2 for i in x], lru['BufferMisses'], width, color='red', alpha=0.7, label='Misses')
    c1.set_xticks(x); c1.set_xticklabels([f"{r}%R\n{w}%W" for r, w in zip(lru['ReadPct'], lru['WritePct'])], fontsize=8)
    c1.set_title('LRU: Hits vs Misses'); c1.grid(axis='y', alpha=0.3); c1.legend()
    c2.bar([i - width/2 for i in x], mru['BufferHits'], width, color='green', alpha=0.7)
    c2.bar([i + width/2 for i in x], mru['BufferMisses'], width, color='red', alpha=0.7)
    c2.set_xticks(x); c2.set_xticklabels([f"{r}%R\n{w}%W" for r, w in zip(mru['ReadPct'], mru['WritePct'])], fontsize=8)
    c2.set_title('MRU: Hits vs Misses'); c2.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    out_bp = os.path.join(dest, 'buffer_performance.png')
    plt.savefig(out_bp, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out_bp}")
    plt.close()

    # Total I/O comparison
    plt.figure(figsize=(10, 6))
    l_total = lru['PhysicalReads'] + lru['PhysicalWrites']
    m_total = mru['PhysicalReads'] + mru['PhysicalWrites']
    plt.plot(lru['ReadPct'], l_total, 'b-o', label='LRU Total I/O', linewidth=2.5)
    plt.plot(mru['ReadPct'], m_total, 'r-s', label='MRU Total I/O', linewidth=2.5)
    plt.xlabel('Read %'); plt.ylabel('Total Physical I/O'); plt.title('Total I/O: LRU vs MRU'); plt.grid(alpha=0.3); plt.legend()
    out_tio = os.path.join(dest, 'total_io_comparison.png')
    plt.savefig(out_tio, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out_tio}")
    plt.close()

    # Summary table
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.axis('off')
    summary = []
    for i in range(len(lru)):
        summary.append([
            f"{lru.iloc[i]['ReadPct']}% / {lru.iloc[i]['WritePct']}%",
            f"{lru.iloc[i]['HitRatio']:.4f}",
            f"{mru.iloc[i]['HitRatio']:.4f}",
            f"{int(lru.iloc[i]['PhysicalReads'] + lru.iloc[i]['PhysicalWrites'])}",
            f"{int(mru.iloc[i]['PhysicalReads'] + mru.iloc[i]['PhysicalWrites'])}"
        ])
    tbl = ax.table(cellText=summary,
                   colLabels=['Mix', 'LRU Hit', 'MRU Hit', 'LRU I/O', 'MRU I/O'],
                   cellLoc='center', loc='center', colWidths=[0.18]*5)
    tbl.auto_set_font_size(False); tbl.set_fontsize(9); tbl.scale(1, 2)
    for col in range(5):
        tbl[(0, col)].set_facecolor('#4CAF50'); tbl[(0, col)].set_text_props(weight='bold', color='white')
    plt.title('Performance Summary', fontsize=14, fontweight='bold', pad=20)
    out_tab = os.path.join(dest, 'summary_table.png')
    plt.savefig(out_tab, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {out_tab}")
    plt.close()

    print("\n" + "="*60)
    print("All graphs generated successfully!")
    print("="*60)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python3 generate_graphs.py <lru_csv> <mru_csv>")
        sys.exit(1)
    lru_csv = sys.argv[1]
    mru_csv = sys.argv[2]
    generate_plots(lru_csv, mru_csv)
