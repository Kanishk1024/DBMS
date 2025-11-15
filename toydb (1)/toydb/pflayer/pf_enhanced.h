/* Per-File Strategy Enhancement for ToyDB Buffer Manager
 * 
 * This header provides an enhanced API that allows each file to have
 * its own replacement strategy, addressing Objective 1's requirement
 * for "strategy selectable when opening file".
 * 
 * DESIGN APPROACH:
 * ----------------
 * Since ToyDB uses a global shared buffer pool (PF_MAX_BUFS=20), we cannot
 * have truly independent buffer pools per file. Instead, we use a hybrid
 * approach:
 * 
 * 1. Store per-file strategy preferences in the file descriptor table (PFftab)
 * 2. When selecting a victim page, prefer pages from files matching the
 *    current file's strategy
 * 3. If no suitable victim is found, fall back to global strategy
 * 
 * This gives files their "preferred" replacement strategy while maintaining
 * the shared buffer pool architecture.
 */

#ifndef PF_ENHANCED_H
#define PF_ENHANCED_H

#include "pf.h"

/* Enhanced API: Open file with specific buffer strategy */
int PF_OpenFileWithStrategy(const char *fname, ReplacementStrategy strategy);

/* Enhanced API: Change strategy for an open file */
int PF_SetFileStrategy(int fd, ReplacementStrategy strategy);

/* Enhanced API: Get current strategy for a file */
ReplacementStrategy PF_GetFileStrategy(int fd);

/* Enhanced API: Get buffer statistics for a specific file */
int PF_GetFileStatistics(int fd, BufferStats *stats);

/* Enhanced API: Reset statistics for a specific file */
void PF_ResetFileStatistics(int fd);

#endif /* PF_ENHANCED_H */
