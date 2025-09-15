# DumpCreator - Memory Dump Usage Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Types of Dumps and Their Contents](#types-of-dumps-and-their-contents)
3. [Setting up Visual Studio for Dump Analysis](#setting-up-visual-studio-for-dump-analysis)
4. [Practical Usage Examples](#practical-usage-examples)
5. [Recommendations for Choosing Dump Type](#recommendations-for-choosing-dump-type)
6. [Troubleshooting](#troubleshooting)

## Introduction

DumpCreator is a cross-platform library for creating memory dumps during application crashes. Memory dumps contain a snapshot of the process state at the moment of failure and are an indispensable tool for debugging critical errors.

### Key Benefits of Dumps

- **State Preservation**: Complete snapshot of process memory at the moment of crash
- **Analysis Without Reproduction**: Can analyze errors that are difficult to reproduce
- **Detailed Information**: Call stack, variables, threads, modules
- **Production Debugging**: Analysis of crashes on client machines

## Types of Dumps and Their Contents

### 1. Basic Mini-Dumps (64KB)

#### MINI_DUMP_NORMAL

**Size**: ~64KB  
**Contents**:

- Basic process information
- Call stack
- CPU registers
- Basic thread information

**When to use**: Quick analysis of simple crashes when only call stack is needed.

#### MINI_DUMP_WITHOUT_OPTIONAL_DATA

**Size**: ~32KB  
**Contents**:

- Minimal process information
- Only critically important data
- Call stack without additional information

**When to use**: When dump size is critical and only call stack is needed.

#### MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY

**Size**: ~64KB  
**Contents**:

- Standard mini-dump information
- Ignores inaccessible memory regions
- Faster creation

**When to use**: When the process has memory access issues.

### 2. Medium Mini-Dumps (128-512KB)

#### MINI_DUMP_WITH_DATA_SEGS

**Size**: ~128KB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- Data segments (.data, .bss)
- Global variables
- Static variables

**When to use**: When you need to analyze the state of global variables.

#### MINI_DUMP_WITH_HANDLE_DATA

**Size**: ~256KB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- Handle information
- Open files, sockets, mutexes
- Resource information

**When to use**: For resource problems, handle leaks.

#### MINI_DUMP_WITH_UNLOADED_MODULES

**Size**: ~512KB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- List of loaded modules
- List of unloaded modules
- DLL information

**When to use**: For problems with library loading/unloading.

#### MINI_DUMP_WITH_THREAD_INFO

**Size**: ~256KB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- Detailed information about all threads
- Thread state
- Thread local variables

**When to use**: For multithreaded crashes, deadlocks.

#### MINI_DUMP_WITH_CODE_SEGMENTS

**Size**: ~512KB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- Code segments (.text)
- Machine code of functions
- Breakpoint information

**When to use**: For assembly code analysis, optimization.

### 3. Large Mini-Dumps (1MB+)

#### MINI_DUMP_WITH_PROCESS_THREAD_DATA

**Size**: ~1MB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- Complete process information
- Detailed information about all threads
- Execution context

**When to use**: For complex multithreaded crashes.

#### MINI_DUMP_WITH_FULL_AUXILIARY_STATE

**Size**: ~1MB  
**Contents**:

- Everything from MINI_DUMP_NORMAL
- Complete auxiliary state
- Additional debug information
- Extended metadata

**When to use**: For deep analysis of application state.

### 4. Full Memory Dumps (Variable Size)

#### MINI_DUMP_WITH_FULL_MEMORY

**Size**: Variable (can be very large)  
**Contents**:

- Everything from previous types
- **ALL PROCESS MEMORY**
- All variables, arrays, objects
- Complete application state

**When to use**:

- For critical crashes
- When full state analysis is needed
- For memory leak analysis
- **WARNING**: Can be very large (gigabytes)

#### MINI_DUMP_WITH_FULL_MEMORY_INFO

**Size**: Variable  
**Contents**:

- Everything from MINI_DUMP_WITH_FULL_MEMORY
- Detailed memory structure information
- Process memory map
- Information about allocated blocks

**When to use**: For memory management analysis.

#### MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY

**Size**: Variable  
**Contents**:

- Everything from MINI_DUMP_WITH_FULL_MEMORY
- Private read/write memory
- Thread stacks
- Heap

**When to use**: For memory corruption analysis, buffer overflows.

### 5. Kernel Dumps

#### KERNEL_FULL_DUMP

**Size**: Variable (very large)  
**Contents**:

- Complete dump of all system memory
- System kernel state
- All processes in the system
- Drivers and system components

**When to use**: For critical system crashes, BSOD analysis.

#### KERNEL_KERNEL_DUMP

**Size**: Variable  
**Contents**:

- Kernel memory only
- System structures
- Drivers

**When to use**: For driver problems.

#### KERNEL_SMALL_DUMP

**Size**: ~64KB  
**Contents**:

- Minimal kernel crash information
- Basic registers
- Kernel stack

**When to use**: Quick analysis of critical system crashes.

## Setting up Visual Studio for Dump Analysis

### 1. Installing Required Components

```bash
# Install Windows SDK (if not installed)
# Download from Microsoft official website

# Install Debugging Tools for Windows
# Included with Windows SDK
```

### 2. Setting up Debug Symbols

#### In Visual Studio

1. **Tools** → **Options** → **Debugging** → **Symbols**
2. Add symbol paths:

   ```console
   https://msdl.microsoft.com/download/symbols
   C:\Symbols
   C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\sym
   ```

#### In DumpCreator Code

```cpp
// Enable debug symbols
DumpCreator::enableDebugSymbols(true);
DumpCreator::setSymbolPath("C:\\Symbols");
```

### 3. Setting up Source Code Paths

#### Visual Studio

1. **Tools** → **Options** → **Debugging** → **General**
2. Enable:
   - ✅ "Enable .NET Framework source stepping"
   - ✅ "Enable source server support"
   - ✅ "Require source files to exactly match the original version"

### 4. Analyzing Dumps in Visual Studio

#### Opening a Dump

1. **File** → **Open** → **File**
2. Select `.dmp` file
3. Visual Studio will automatically load the dump

#### Main Windows for Analysis

##### **Call Stack**

- Shows sequence of function calls
- Double-click to navigate to source code
- Shows function parameters

##### **Locals**

- Values of local variables in current frame
- Objects and their fields
- Arrays and containers

##### **Watch**

- Add expressions for monitoring
- Evaluate complex expressions
- Analyze pointers

##### **Memory**

- View raw memory
- Analyze pointers
- Search for patterns in memory

##### **Modules**

- List of loaded DLLs
- Module versions
- Module paths

##### **Threads**

- List of all threads
- State of each thread
- Switch between threads

### 5. Advanced Analysis Techniques

#### Exception Analysis

```cpp
// In Watch window add:
$exception
$exception.StackTrace
$exception.Message
```

#### Pointer Analysis

```cpp
// For pointer analysis:
*(int*)0x12345678
// For array analysis:
(int[10])0x12345678
```

#### Object Analysis

```cpp
// For C++ objects:
((MyClass*)0x12345678)->member
// For STL containers:
((std::vector<int>*)0x12345678)->size()
```

## Practical Usage Examples

### Example 1: Simple Crash Analysis

```cpp
// Use MINI_DUMP_NORMAL for simple crashes
DumpCreator::initialize("", DumpType::MINI_DUMP_NORMAL, true);

void crashFunction() {
    int* ptr = nullptr;
    *ptr = 42; // Access violation
}
```

**What to look for in the dump:**

1. Call Stack will show the exact crash location
2. Locals will show that `ptr = nullptr`
3. Memory will show that there's no data at address 0x00000000

### Example 2: Multithreaded Crash Analysis

```cpp
// Use MINI_DUMP_WITH_THREAD_INFO
DumpCreator::initialize("", DumpType::MINI_DUMP_WITH_THREAD_INFO, true);

void threadFunction() {
    std::mutex mtx;
    mtx.lock();
    // ... code ...
    // Forgot unlock() - deadlock
}
```

**What to look for in the dump:**

1. Threads will show all threads
2. Find the blocked thread
3. Call Stack will show where the deadlock occurred

### Example 3: Memory Leak Analysis

```cpp
// Use MINI_DUMP_WITH_FULL_MEMORY
DumpCreator::initialize("", DumpType::MINI_DUMP_WITH_FULL_MEMORY, true);

void memoryLeak() {
    int* data = new int[1000000];
    // Forgot delete[] - memory leak
}
```

**What to look for in the dump:**

1. Memory will show allocated blocks
2. Address search will show leaks
3. Heap analysis will show unreleased memory

### Example 4: Data Corruption Analysis

```cpp
// Use MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY
DumpCreator::initialize("", DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY, true);

void bufferOverflow() {
    char buffer[10];
    strcpy(buffer, "This string is too long!"); // Buffer overflow
}
```

**What to look for in the dump:**

1. Memory will show the corrupted area
2. Call Stack will show the overflow location
3. Analysis of adjacent variables will show corruption

## Recommendations for Choosing Dump Type

### For Development

- **MINI_DUMP_NORMAL** - fast debugging
- **MINI_DUMP_WITH_DATA_SEGS** - global variable analysis
- **MINI_DUMP_WITH_THREAD_INFO** - multithreaded debugging

### For Testing

- **MINI_DUMP_WITH_HANDLE_DATA** - resource analysis
- **MINI_DUMP_WITH_UNLOADED_MODULES** - module loading analysis
- **MINI_DUMP_WITH_PROCESS_THREAD_DATA** - comprehensive analysis

### For Production

- **MINI_DUMP_WITH_FULL_MEMORY** - full analysis of critical crashes
- **KERNEL_FULL_DUMP** - system crash analysis
- **MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY** - memory corruption analysis

### Complete Table of All Dump Types

| #   | Name                                        | Full Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  | Features                                                                     |
| --- | ------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| 1   | MINI_DUMP_NORMAL                            | **Basic mini-dump with minimal set of critically important process information.** Contains basic process structure, call stack of current thread, CPU registers at crash moment, basic information about all process threads, exception information that caused the crash, and basic process metadata. This is the fastest and most compact dump type, ideal for initial diagnosis of simple crashes when you need to quickly determine the error location in code.                                               | Call stack, registers, basic thread information (~64KB)                      |
| 2   | MINI_DUMP_WITH_DATA_SEGS                    | **Mini-dump with inclusion of program data segments.** Extends basic dump by adding all data segments (.data, .bss, .rdata), including global variables, static variables, constants, initialized and uninitialized data. Allows analyzing global variable state at crash moment, which is critical for diagnosing initialization problems, application state, and global objects. Especially useful when debugging singleton issues, global containers, and static objects.                                      | Includes .data and .bss segments, global and static variables (~128KB)       |
| 3   | MINI_DUMP_WITH_FULL_MEMORY                  | **Full dump of all process virtual memory.** Contains absolutely all memory allocated to the process, including all code and data segments, heaps, stacks of all threads, loaded DLLs, memory allocated via malloc/new, system structures, and all other areas of virtual address space. This is the most informative dump type, allowing complete application state recovery, but requiring significant disk space and creation time. Critical for analyzing complex crashes, memory leaks, and data corruption. | All process memory, all variables and objects (variable size)                |
| 4   | MINI_DUMP_WITH_HANDLE_DATA                  | **Mini-dump with detailed information about system handles.** Includes complete information about all open handles: files, sockets, mutexes, semaphores, events, processes, threads, memory sections, registry keys, and other system resources. Contains file names, paths, handle types, access rights, lock states. Necessary for diagnosing resource leaks, file operation problems, network connections, thread synchronization, and other system resources.                                                 | Open files, sockets, mutexes, resource information (~256KB)                  |
| 5   | MINI_DUMP_FILTER_MEMORY                     | **Filtered mini-dump with exclusion of certain memory areas.** Allows configuring filters to exclude certain memory types or areas from the dump, significantly reducing file size. Can exclude other thread stacks, large buffers, temporary data, or confidential information. Useful when basic crash information is needed but dump size is critical, or when sensitive data needs to be excluded from the dump for third-party transmission.                                                                 | Excludes certain memory areas to reduce size (~64KB)                         |
| 6   | MINI_DUMP_SCAN_MEMORY                       | **Scanned mini-dump with memory analysis for important data.** Performs intelligent memory scanning to find and include only relevant data in the dump: object pointers, strings, data structures that may be related to the crash cause. Uses heuristic algorithms to determine memory area importance. Effective for quick analysis without creating full dumps when specific data or patterns need to be found in memory.                                                                                      | Analyzes memory for important data (~128KB)                                  |
| 7   | MINI_DUMP_WITH_UNLOADED_MODULES             | **Mini-dump with complete information about loaded and unloaded modules.** Contains detailed information about all DLLs and executable modules that were loaded into the process, including those unloaded by crash time. Includes module paths, versions, load/unload timestamps, base addresses, sizes, symbol information. Critical for diagnosing library loading problems, version conflicts, corrupted modules, and dependency issues.                                                                      | List of loaded and unloaded DLLs, module information (~512KB)                |
| 8   | MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY | **Mini-dump with indirectly referenced memory through pointers.** Includes memory referenced through pointers from stack, registers, and other memory areas. Performs pointer tracing to find related data: objects referenced by variables, array elements, structure fields, data in STL containers. Allows analyzing object state even if they're not directly in the stack, which is critical for understanding crash context in object-oriented code.                                                        | Memory referenced through pointers (variable size)                           |
| 9   | MINI_DUMP_FILTER_MODULE_PATHS               | **Mini-dump with filtering of full module paths for anonymity.** Creates dump with complete debug information but replaces full module paths with anonymous identifiers or relative paths. This allows transmitting dumps to third parties (e.g., support service) without revealing file system structure, usernames, or other sensitive path data. Preserves all debugging functionality while ensuring confidentiality.                                                                                        | Excludes full module paths for anonymity (~64KB)                             |
| 10  | MINI_DUMP_WITH_PROCESS_THREAD_DATA          | **Mini-dump with complete process and all its threads data.** Contains exhaustive information about process state: all threads with their stacks, registers, local variables, execution context, priorities, synchronization state. Includes information about parent process, child processes, process groups, sessions, security tokens. Necessary for analyzing complex multithreaded crashes, deadlocks, race conditions, synchronization problems, and inter-process communication.                          | Complete process and all threads information (~1MB)                          |
| 11  | MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY    | **Mini-dump with process private read/write memory.** Includes all process memory areas accessible for reading and writing: heaps, all thread stacks, private data, buffers, temporary areas. Excludes only code segments and system areas. Allows analyzing state of all user data, variables, objects, containers, buffers at crash moment. Critical for diagnosing memory corruption, buffer overflows, memory leaks, memory management problems.                                                              | Private memory, thread stacks, heap (variable size)                          |
| 12  | MINI_DUMP_WITHOUT_OPTIONAL_DATA             | **Mini-dump without additional optional data.** Creates minimally possible dump containing only critically important information for basic diagnosis: call stack, registers, basic process information. Excludes all additional data that may be useful but not critical for determining crash cause. Used when dump size is critical (e.g., limited disk space) but basic debugging capability needs to be preserved.                                                                                            | Only critically important information (~32KB)                                |
| 13  | MINI_DUMP_WITH_FULL_MEMORY_INFO             | **Mini-dump with complete information about process memory structure and state.** Contains detailed virtual address space map: all allocated memory areas, their types, access rights, sizes, state, heap information, stacks, loaded modules. Includes memory allocation metadata, usage statistics, fragmentation information. Necessary for analyzing memory management problems, leaks, fragmentation, incorrect memory deallocation, heap problems.                                                          | Detailed memory map, allocated block information (variable size)             |
| 14  | MINI_DUMP_WITH_THREAD_INFO                  | **Mini-dump with detailed information about all process threads.** Contains detailed information about each thread: complete call stack, registers, local variables, synchronization state, priorities, creation time, execution time, locked resources. Allows analyzing state of all threads at crash moment, finding blocked threads, analyzing deadlocks, race conditions, synchronization problems. Critical for multithreaded applications and diagnosing parallel execution problems.                      | Detailed information about all threads, their state (~256KB)                 |
| 15  | MINI_DUMP_WITH_CODE_SEGMENTS                | **Mini-dump with code segments and machine instructions.** Includes all code segments (.text) of loaded modules, machine code of functions, breakpoint information, assembly-level debug information. Allows analyzing execution at machine instruction level, optimizing performance, finding compilation problems, analyzing assembly code. Necessary for low-level debugging, performance analysis, compiler optimization problems, debugging inline functions and templates.                                  | Machine code of functions, .text segments (~512KB)                           |
| 16  | MINI_DUMP_WITHOUT_AUXILIARY_STATE           | **Mini-dump without auxiliary state and additional debug information.** Creates dump with basic crash information but excludes auxiliary data: additional debug information, metadata, extended contexts, service information. Used when basic diagnosis is needed without excessive information, or when dump size is critical but basic debugging functionality must be preserved.                                                                                                                              | Excludes additional debug information (~64KB)                                |
| 17  | MINI_DUMP_WITH_FULL_AUXILIARY_STATE         | **Mini-dump with complete auxiliary state and extended metadata.** Includes all possible auxiliary data: extended execution contexts, additional metadata, service information about system state, detailed debug information, extended symbols, additional module and process information. Provides maximum complete picture of system state at crash moment, but significantly increases dump size.                                                                                                             | Extended metadata and debug information (~1MB)                               |
| 18  | MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY    | **Mini-dump with private memory using copy-on-write mechanism.** Includes memory areas that use copy-on-write mechanism: shared memory pages that can be modified by the process, page copies created on first write. Allows analyzing memory state that was shared but became private to the process. Useful for diagnosing shared memory problems, inter-process communication, memory page copying.                                                                                                            | Memory with copy-on-write mechanism (variable size)                          |
| 19  | MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY        | **Mini-dump ignoring inaccessible memory areas.** Creates dump by skipping memory areas that are inaccessible at dump creation moment. This can happen due to memory protection, virtualization, or temporary page unavailability. Speeds up dump creation and avoids errors when trying to access protected memory, but may skip important information. Used when process has memory access problems, or when dump needs to be created quickly without stopping on access errors.                                | Skips inaccessible memory areas (~64KB)                                      |
| 20  | MINI_DUMP_WITH_TOKEN_INFORMATION            | **Mini-dump with security token and privilege information.** Includes detailed information about process security tokens: privileges, security groups, access rights, security context, user information under which the process runs. Necessary for diagnosing security problems, access rights, privileges, authentication, authorization. Useful when debugging file access, registry, system resources, network operations requiring special rights.                                                          | Security token and privilege information (~128KB)                            |
| 21  | KERNEL_FULL_DUMP                            | **Complete kernel dump of all system memory and kernel state.** Contains complete snapshot of all system physical memory, operating system kernel state, all processes and threads, drivers, system structures, kernel buffers, interrupt contexts. This is the most complete dump type, allowing analysis of entire system state at critical crash moment. Necessary for diagnosing BSOD (Blue Screen of Death), critical system crashes, driver problems, hardware failures.                                    | All system memory, kernel state, all processes (very large)                  |
| 22  | KERNEL_KERNEL_DUMP                          | **Kernel dump of operating system kernel space only.** Contains only memory and structures related to system kernel: kernel code, system structures, drivers, kernel buffers, interrupt contexts, system queues. Excludes user processes and their memory. Used for diagnosing system kernel problems, drivers, system services, without analyzing user applications. Effective for analyzing system crashes, driver problems, critical kernel errors.                                                            | Kernel memory only, system structures, drivers (variable size)               |
| 23  | KERNEL_SMALL_DUMP                           | **Small kernel dump with minimal kernel crash information.** Contains only critically important crash information: basic CPU registers, kernel stack at crash moment, exception information, basic system structures. Most compact kernel dump type, used for quick diagnosis of critical system crashes. Allows quickly determining BSOD cause without creating large dump files.                                                                                                                                | Minimal kernel crash information, basic registers (~64KB)                    |
| 24  | KERNEL_AUTOMATIC_DUMP                       | **Automatic kernel dump with flexible size.** System automatically selects appropriate dump size and content depending on crash type, available disk space, and error criticality. Can create dumps of different sizes: from minimal to full, depending on situation. Provides balance between dump informativeness and disk space requirements. Used in systems with limited disk space where full dump is impossible but maximum possible crash information is needed.                                          | Flexible size, automatically selects appropriate data volume (variable size) |
| 25  | KERNEL_ACTIVE_DUMP                          | **Active kernel dump focused on active processes and threads.** Creates dump including complete system kernel information but focusing on active processes and threads at crash moment. Excludes inactive or sleeping processes, concentrating on those that were active and could be related to crash cause. Effective for analyzing crashes related to active processes without including excessive information about inactive system components.                                                               | Similar to full but smaller size (variable size)                             |
| 26  | CORE_DUMP_FULL                              | **Complete UNIX core dump with all process memory.** Contains complete snapshot of process virtual memory in UNIX systems, including all code and data segments, stacks, heaps, loaded libraries, system structures. Equivalent to MINI_DUMP_WITH_FULL_MEMORY for UNIX systems. Allows complete process state recovery for analyzing crashes, memory leaks, data corruption. Standard debugging tool in UNIX/Linux systems.                                                                                       | Complete dump with all process memory (variable size)                        |
| 27  | CORE_DUMP_KERNEL_ONLY                       | **UNIX kernel space dump only.** Contains only operating system kernel information in UNIX systems: system structures, drivers, kernel buffers, interrupt contexts. Excludes user process space. Used for diagnosing system kernel problems, drivers, system services in UNIX/Linux systems.                                                                                                                                                                                                                      | Kernel space only (variable size)                                            |
| 28  | CORE_DUMP_USER_ONLY                         | **UNIX user space dump only.** Contains only user space process memory and structures in UNIX systems, excluding system kernel. Includes process code and data, stacks, heaps, loaded libraries, but not system kernel structures. Used for analyzing user application problems without analyzing system components.                                                                                                                                                                                              | User space only (variable size)                                              |
| 29  | CORE_DUMP_COMPRESSED                        | **Compressed UNIX core dump for disk space economy.** Creates complete core dump but compresses data to reduce file size. Uses compression algorithms (usually gzip) for significant size reduction while preserving all information. Necessary in systems with limited disk space where complete uncompressed dump is impossible. Requires decompression before analysis but provides complete debugging functionality.                                                                                          | Compressed dump for space economy (variable size)                            |
| 30  | CORE_DUMP_FILTERED                          | **Filtered UNIX core dump with exclusion of certain memory areas.** Allows configuring filters to exclude certain memory types or areas from the dump in UNIX systems. Can exclude large buffers, temporary data, confidential information, or areas not related to crash cause. Useful when debug information is needed but dump size is critical, or when sensitive data needs to be excluded.                                                                                                                  | Excludes certain memory areas (variable size)                                |

### Dump Sizes (Approximate)

| Dump Type                          | Size       | Creation Time  | Informativeness |
| ---------------------------------- | ---------- | -------------- | --------------- |
| MINI_DUMP_NORMAL                   | 64KB       | < 1 sec        | Basic           |
| MINI_DUMP_WITH_DATA_SEGS           | 128KB      | 1-2 sec        | Medium          |
| MINI_DUMP_WITH_THREAD_INFO         | 256KB      | 2-3 sec        | High            |
| MINI_DUMP_WITH_PROCESS_THREAD_DATA | 1MB        | 5-10 sec       | Very High       |
| MINI_DUMP_WITH_FULL_MEMORY         | 100MB-10GB | 30 sec - 5 min | Maximum         |

## Troubleshooting

### Problem: Dump won't open in Visual Studio

**Solution:**

1. Ensure Windows SDK is installed
2. Check that dump was created on the same architecture (x86/x64)
3. Ensure debug symbols are available

### Problem: No debug symbols

**Solution:**

```cpp
// Enable debug information in compiler
// MSVC: /Zi /DEBUG
// GCC: -g -g3

// Set symbol paths
DumpCreator::enableDebugSymbols(true);
DumpCreator::setSymbolPath("C:\\Symbols");
```

### Problem: Dump is too large

**Solution:**

1. Use filtered dumps
2. Set maximum size:

```cpp
DumpConfiguration config;
config.maxSizeBytes = 100 * 1024 * 1024; // 100MB
DumpCreator::initialize(config, true);
```

### Problem: No access to private members

**Solution:**

1. Ensure you're using debug build
2. Check that symbols are loaded
3. Use full memory dumps

### Problem: Dump is created but empty

**Solution:**

1. Check dump directory access permissions
2. Ensure process doesn't terminate too quickly
3. Check UAC settings

## Conclusion

Choosing the right dump type is critical for effective debugging. Start with simple dumps for basic debugging and move to full dumps only when deep analysis is needed. Remember that large dumps require more time for creation and analysis, but provide the most complete information about application state at the moment of crash.

### Key Principles

1. **Start small** - use simple dumps for initial diagnosis
2. **Symbols are critical** - analysis is practically impossible without them
3. **Size matters** - choose appropriate size for your needs
4. **Practice** - regularly analyze dumps to develop skills
5. **Document** - keep records of found problems and their solutions
