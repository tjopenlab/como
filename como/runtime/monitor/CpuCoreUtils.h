//=========================================================================
// Copyright (C) 2022 The C++ Component Model(COMO) Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#include <pthread.h>
#include <sched.h>

namespace como {

class CpuCoreUtils {
public:
    static int SetThreadAffinity(pthread_t thread, int iCore);
    static int SetProcessAffinity(pid_t pid, int iCore);
    static unsigned long GetCpuTotalOccupy();

    // return ms, millisecond
    static unsigned long GetCpuProcOccupy(unsigned int pid);

    // CPU usage percentage
    static float GetProcCpuUsagePercent(unsigned int pid);

    static unsigned int GetProcMem(unsigned int pid);
    static unsigned int GetProcVirtualmem(unsigned int pid);
    static int GetPidByNameAndUser(const char* process_name, const char* user = nullptr);

    /**
     * Memory
     * malloc_stats() − print memory allocation statistics
     *      https://fossies.org/linux/man-pages/man3/malloc_stats.3
     *
     *  mallinfo(), mallinfo2() - obtain memory allocation information
     *      https://man7.org/linux/man-pages/man3/mallinfo.3.html
     * struct mallinfo2 {
     *    size_t arena;     // Non-mmapped space allocated (bytes)
     *    size_t ordblks;   // Number of free chunks
     *    size_t smblks;    // Number of free fastbin blocks
     *    size_t hblks;     // Number of mmapped regions
     *    size_t hblkhd;    // Space allocated in mmapped regions (bytes)
     *    size_t usmblks;   // See below
     *    size_t fsmblks;   // Space in freed fastbin blocks (bytes)
     *    size_t uordblks;  // Total allocated space (bytes)
     *    size_t fordblks;  // Total free space (bytes)
     *    size_t keepcost;  // Top-most, releasable space (bytes)
     * };
     */
};

} // namespace como
