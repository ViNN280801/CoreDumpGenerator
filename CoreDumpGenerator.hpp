#ifndef CORE_DUMP_GENERATOR_HPP
#define CORE_DUMP_GENERATOR_HPP

// C++ Standard feature detection macros
#if __cplusplus >= 201103L
  #define CPP11_OR_GREATER 1
#else
  #define CPP11_OR_GREATER 0
#endif

#if __cplusplus >= 201402L
  #define CPP14_OR_GREATER 1
#else
  #define CPP14_OR_GREATER 0
#endif

#if __cplusplus >= 201703L
  #define CPP17_OR_GREATER 1
#else
  #define CPP17_OR_GREATER 0
#endif

#if __cplusplus >= 202002L
  #define CPP20_OR_GREATER 1
#else
  #define CPP20_OR_GREATER 0
#endif

#if __cplusplus >= 202302L
  #define CPP23_OR_GREATER 1
#else
  #define CPP23_OR_GREATER 0
#endif

// Feature detection for specific C++ features
#if CPP11_OR_GREATER
  #define HAS_TYPE_TRAITS 1
#else
  #define HAS_TYPE_TRAITS 0
#endif

#if CPP14_OR_GREATER
  #define HAS_MAKE_UNIQUE 1
#else
  #define HAS_MAKE_UNIQUE 0
#endif

#if CPP17_OR_GREATER
  #define HAS_OPTIONAL 1
  #define HAS_VARIANT 1
  #define HAS_ANY 1
  #define HAS_STRING_VIEW 1
#else
  #define HAS_OPTIONAL 0
  #define HAS_VARIANT 0
  #define HAS_ANY 0
  #define HAS_STRING_VIEW 0
#endif

#if CPP20_OR_GREATER
  #define HAS_CONCEPTS 1
  #define HAS_RANGES 1
  #define HAS_SPAN 1
  #define HAS_FORMAT 1
#else
  #define HAS_CONCEPTS 0
  #define HAS_RANGES 0
  #define HAS_SPAN 0
  #define HAS_FORMAT 0
#endif

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

#if HAS_STRING_VIEW
  #include <string_view>
#endif

#if HAS_OPTIONAL
  #include <optional>
#endif

#if HAS_SPAN
  #include <span>
#endif

#if CPP11_OR_GREATER
  #include <atomic>
#endif

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
  #define DUMP_CREATOR_WINDOWS 1
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
  #define DUMP_CREATOR_UNIX 1
#else
  #error "Unsupported platform"
#endif

// Windows-specific includes
#if DUMP_CREATOR_WINDOWS
  #include <windows.h>

  #include <dbghelp.h>
  #include <direct.h>
  #include <io.h>
  #include <shlwapi.h>
  #include <tchar.h>

  #pragma comment(lib, "dbghelp.lib")
  #pragma comment(lib, "shlwapi.lib")
#endif

// UNIX-specific includes
#if DUMP_CREATOR_UNIX
  #include <csignal>
  #include <cstdlib>
  #include <errno.h>
  #include <fcntl.h>
  #include <limits.h>
  #include <sys/prctl.h>
  #include <sys/resource.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>

#endif

/**
 * @enum DumpType
 * @brief Comprehensive enumeration of all supported crash dump types across platforms
 *
 * This enumeration defines the complete set of dump types that can be generated
 * by the CoreDumpGenerator, including Windows-specific mini-dump types, kernel-mode
 * dump variants, and UNIX core dump types. Each type is designed to capture
 * different levels of system state information for debugging purposes.
 *
 * @details The enumeration is based on Microsoft's MINIDUMP_TYPE documentation
 * and POSIX core dump specifications, providing a unified interface for
 * cross-platform crash dump generation.
 *
 * @note All enum values are explicitly typed as std::int8_t for memory efficiency
 * and ABI stability across different compilers and platforms.
 *
 * @since Version 1.0
 * @see https://docs.microsoft.com/en-us/windows/win32/api/minidumpapiset/ne-minidumpapiset-minidump_type
 * @see https://pubs.opengroup.org/onlinepubs/9699919799/functions/core.html
 */
enum class DumpType : std::int8_t
{
  // Windows-specific dump types (based on MINIDUMP_TYPE)
  MINI_DUMP_NORMAL                            = 0,  ///< Basic mini-dump (64KB)
  MINI_DUMP_WITH_DATA_SEGS                    = 1,  ///< Include data segments
  MINI_DUMP_WITH_FULL_MEMORY                  = 2,  ///< Full memory dump (largest)
  MINI_DUMP_WITH_HANDLE_DATA                  = 3,  ///< Include handle data
  MINI_DUMP_FILTER_MEMORY                     = 4,  ///< Filter memory
  MINI_DUMP_SCAN_MEMORY                       = 5,  ///< Scan memory
  MINI_DUMP_WITH_UNLOADED_MODULES             = 6,  ///< Include unloaded modules
  MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY = 7,  ///< Include indirectly referenced memory
  MINI_DUMP_FILTER_MODULE_PATHS               = 8,  ///< Filter module paths
  MINI_DUMP_WITH_PROCESS_THREAD_DATA          = 9,  ///< Include process/thread data
  MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY    = 10, ///< Include private read/write memory
  MINI_DUMP_WITHOUT_OPTIONAL_DATA             = 11, ///< Without optional data
  MINI_DUMP_WITH_FULL_MEMORY_INFO             = 12, ///< Include full memory info
  MINI_DUMP_WITH_THREAD_INFO                  = 13, ///< Include thread info
  MINI_DUMP_WITH_CODE_SEGMENTS                = 14, ///< Include code segments
  MINI_DUMP_WITHOUT_AUXILIARY_STATE           = 15, ///< Without auxiliary state
  MINI_DUMP_WITH_FULL_AUXILIARY_STATE         = 16, ///< With full auxiliary state
  MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY    = 17, ///< Include private write-copy memory
  MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY        = 18, ///< Ignore inaccessible memory
  MINI_DUMP_WITH_TOKEN_INFORMATION            = 19, ///< Include token information

  // Windows kernel-mode dump types (based on Microsoft documentation)
  KERNEL_FULL_DUMP      = 20, ///< Полный дамп памяти - largest kernel dump
  KERNEL_KERNEL_DUMP    = 21, ///< Дамп памяти ядра - kernel memory only
  KERNEL_SMALL_DUMP     = 22, ///< Небольшой дамп памяти - 64KB
  KERNEL_AUTOMATIC_DUMP = 23, ///< Автоматический дамп памяти - flexible size
  KERNEL_ACTIVE_DUMP    = 24, ///< Активный дамп памяти - similar to full but smaller

  // UNIX/Linux core dump types
  CORE_DUMP_FULL        = 25, ///< Full core dump with all memory
  CORE_DUMP_KERNEL_ONLY = 26, ///< Kernel-space only core dump
  CORE_DUMP_USER_ONLY   = 27, ///< User-space only core dump
  CORE_DUMP_COMPRESSED  = 28, ///< Compressed core dump
  CORE_DUMP_FILTERED    = 29, ///< Filtered core dump (exclude certain memory regions)

  // Default types
  DEFAULT_WINDOWS = MINI_DUMP_WITH_FULL_MEMORY, ///< Default Windows dump type
  DEFAULT_UNIX    = CORE_DUMP_FULL,             ///< Default UNIX dump type
  DEFAULT_AUTO    = -1                          ///< Auto-detect based on platform
};

/**
 * @struct DumpConfiguration
 * @brief Comprehensive configuration structure for crash dump generation
 *
 * This structure encapsulates all configurable parameters for generating
 * crash dumps, including dump type selection, file naming, directory
 * specification, size limits, compression settings, and platform-specific
 * filtering options. It provides a unified interface for configuring
 * dump generation across different platforms and dump types.
 *
 * @details The structure is designed to be:
 * - Copyable and movable for efficient parameter passing
 * - Default-constructible with sensible defaults
 * - Thread-safe for concurrent access (read-only operations)
 * - Extensible for future platform-specific options
 * - Value semantics with proper move semantics
 *
 * @invariant All string members must be valid UTF-8 encoded strings
 * @invariant maxSizeBytes must be 0 (unlimited) or a positive value
 * @invariant directory must be a valid, accessible path
 * @invariant filename must be a valid filename (no path separators)
 *
 * @note This structure uses public member variables for simplicity and
 * performance, as it serves as a data transfer object (DTO) rather than
 * an encapsulated class with behavior.
 *
 * @thread_safety This structure is thread-safe for read-only operations.
 * Modifications should be synchronized externally.
 *
 * @since Version 1.0
 * @see DumpFactory::createConfiguration()
 * @see CoreDumpGenerator::initialize()
 */
struct DumpConfiguration {
public:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  DumpType type = DumpType::DEFAULT_AUTO; ///< Type of dump to generate
  std::string filename;                   ///< Custom filename (empty for auto-generated)
  std::string directory;                  ///< Directory for dump files
  bool compress               = false;    ///< Whether to compress the dump
  bool includeUnloadedModules = true;     ///< Include unloaded modules (Windows)
  bool includeHandleData      = true;     ///< Include handle data (Windows)
  bool includeThreadInfo      = true;     ///< Include thread information
  bool includeProcessData     = true;     ///< Include process data
  size_t maxSizeBytes         = 0;        ///< Maximum size in bytes (0 = unlimited)
  std::vector<std::string> memoryFilters; ///< Memory region filters (UNIX)
  bool enableSymbols    = true;           ///< Enable symbol information
  bool enableSourceInfo = true;           ///< Enable source file information
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  // Constructors and assignment operators
  DumpConfiguration() noexcept  = default;
  ~DumpConfiguration() noexcept = default;

  // Move constructor and assignment for performance
  DumpConfiguration(DumpConfiguration &&) noexcept            = default;
  DumpConfiguration &operator=(DumpConfiguration &&) noexcept = default;

  // Copy constructor and assignment
  DumpConfiguration(DumpConfiguration const &)            = default;
  DumpConfiguration &operator=(DumpConfiguration const &) = default;

  // Equality comparison for testing and validation
  bool operator==(DumpConfiguration const &other) const noexcept;
  bool operator!=(DumpConfiguration const &other) const noexcept;

  // Validation methods
  bool isValid() const noexcept;
  std::string getValidationError() const;
};

/**
 * @class DumpFactory
 * @brief Factory class for creating and configuring crash dump generators
 *
 * This factory class implements the Factory design pattern to create
 * and configure different types of crash dump generators based on the
 * DumpType enumeration. It provides a centralized way to create
 * platform-appropriate dump configurations and validate dump type
 * support across different operating systems.
 *
 * @details The factory provides:
 * - Configuration creation for all supported dump types
 * - Platform-specific configuration optimization
 * - Dump type validation and support checking
 * - Human-readable descriptions and size estimates
 * - Cross-platform compatibility mapping
 *
 * @note This class is stateless and thread-safe. All methods are
 * static and can be called concurrently from multiple threads.
 *
 * @thread_safety All public methods are thread-safe and can be called
 * concurrently without external synchronization.
 *
 * @since Version 1.0
 * @see DumpType
 * @see DumpConfiguration
 * @see CoreDumpGenerator
 */
class DumpFactory
{
public:
  /**
   * @brief Create a dump configuration for the specified type
   * @param type The dump type to create configuration for
   * @return DumpConfiguration object configured for the specified type
   */
  static DumpConfiguration createConfiguration(DumpType type);

  /**
   * @brief Get the default dump type for the current platform
   * @return Default DumpType for the current platform
   */
  static DumpType getDefaultDumpType() noexcept;

  /**
   * @brief Check if a dump type is supported on the current platform
   * @param type The dump type to check
   * @return true if supported, false otherwise
   */
  static bool isSupported(DumpType type);

  /**
   * @brief Get a human-readable description of the dump type
   * @param type The dump type to describe
   * @return String description of the dump type
   */
  static std::string getDescription(DumpType type);

  /**
   * @brief Get the estimated size of a dump type
   * @param type The dump type to estimate
   * @return Estimated size in bytes (0 if unknown)
   */
  static size_t getEstimatedSize(DumpType type);

private:
  // Platform-specific configuration creators
  static DumpConfiguration createWindowsConfiguration(DumpType type);
  static DumpConfiguration createUnixConfiguration(DumpType type);

  // Registry of dump type descriptions
  static std::map<DumpType, std::string> const s_descriptions;
  static std::map<DumpType, size_t> const s_estimatedSizes;
};

/**
 * @class CoreDumpGenerator
 * @brief Cross-platform crash dump handler with comprehensive debugging support
 *
 * This class provides a robust, thread-safe crash dump handler that automatically
 * captures memory dumps when crashes occur on both Windows and UNIX platforms.
 * It supports multiple dump types through the DumpFactory pattern and provides
 * comprehensive debugging information for post-mortem analysis.
 *
 * @details The CoreDumpGenerator offers:
 * - Automatic crash detection and dump generation
 * - Cross-platform compatibility (Windows/UNIX)
 * - Multiple dump types and configurations
 * - Thread-safe singleton pattern implementation
 * - Comprehensive error handling and logging
 * - Security-focused path validation
 * - Atomic file operations to prevent race conditions
 *
 * @invariant The singleton instance is created only once and is thread-safe
 * @invariant All file operations are atomic to prevent TOCTOU race conditions
 * @invariant All input paths are validated for security vulnerabilities
 * @invariant Platform-specific handlers are properly initialized
 *
 * @thread_safety This class is thread-safe. Multiple threads can call
 * public methods concurrently without external synchronization.
 *
 * @exception_safety All public methods provide strong exception safety
 * guarantee unless otherwise specified. Methods marked noexcept provide
 * no-throw guarantee.
 *
 * @since Version 1.0
 * @see DumpType
 * @see DumpConfiguration
 * @see DumpFactory
 *
 * @example
 * ```cpp
 * // Initialize the crash dump handler
 * CoreDumpGenerator::initialize("/path/to/dumps", DumpType::MINI_DUMP_WITH_FULL_MEMORY);
 *
 * // Generate a manual dump
 * CoreDumpGenerator::generateDump("Manual dump for testing");
 *
 * // Get the singleton instance
 * auto& generator = CoreDumpGenerator::instance();
 * ```
 */
class CoreDumpGenerator
{
public:
  static constexpr size_t const KB_32                     = 32ULL * 1024ULL;   // 32KB
  static constexpr size_t const KB_64                     = 64ULL * 1024ULL;   // 64KB
  static constexpr size_t const KB_128                    = 128ULL * 1024ULL;  // 128KB
  static constexpr size_t const KB_256                    = 256ULL * 1024ULL;  // 256KB
  static constexpr size_t const KB_512                    = 512ULL * 1024ULL;  // 512KB
  static constexpr size_t const MB_1                      = 1024ULL * 1024ULL; // 1MB

  CoreDumpGenerator(CoreDumpGenerator const &)            = delete;
  CoreDumpGenerator &operator=(CoreDumpGenerator const &) = delete;
  CoreDumpGenerator(CoreDumpGenerator &&)                 = delete;
  CoreDumpGenerator &operator=(CoreDumpGenerator &&)      = delete;
  ~CoreDumpGenerator() noexcept                           = default;

  /**
   * @brief Initialize the crash dump handler
   *
   * Sets up platform-specific crash handlers with default configuration.
   * Also sets up C++ exception handling.
   *
   * @param dumpDirectory Optional directory path for dump files.
   *                      If empty, uses executable directory.
   * @param dumpType Type of dump to generate (defaults to platform-specific)
   * @param handleExceptions Whether to handle unhandled C++ exceptions (default: true)
   * @throws std::runtime_error if initialization fails
   * @throws std::system_error if filesystem operations fail
   * @throws std::invalid_argument if configuration is invalid
   *
   * @complexity O(1) for basic initialization, O(n) where n is directory depth for recursive creation
   * @thread_safety This function is thread-safe and may be called concurrently
   * @exception_safety Strong guarantee: if an exception is thrown, the object remains in a valid state
   */
  static void initialize(std::string const &dumpDirectory = "", DumpType dumpType = DumpType::DEFAULT_AUTO,
                         bool handleExceptions = true);

  /**
   * @brief Initialize the crash dump handler with full configuration
   *
   * Sets up platform-specific crash handlers with custom configuration.
   * Also sets up C++ exception handling.
   *
   * @param config Complete dump configuration
   * @param handleExceptions Whether to handle unhandled C++ exceptions (default: true)
   * @throws std::runtime_error if initialization fails
   * @throws std::system_error if filesystem operations fail
   */
  static void initialize(DumpConfiguration const &config, bool handleExceptions = true);

  /**
   * @brief Check if the dump creator is initialized
   *
   * @return true if initialized, false otherwise
   */
  static bool isInitialized() noexcept;

  /**
   * @brief Manually trigger a dump generation
   *
   * @param reason Optional reason for the dump generation
   * @param dumpType Type of dump to generate (uses current config if DEFAULT_AUTO)
   * @return true if dump was generated successfully, false otherwise
   * @throws std::runtime_error if not initialized
   * @throws std::system_error if filesystem operations fail
   * @throws std::invalid_argument if dump type is not supported
   *
   * @complexity O(1) for basic operations, O(n) where n is memory size for full dumps
   * @thread_safety This function is thread-safe and may be called concurrently
   * @exception_safety Basic guarantee: if an exception is thrown, the system remains in a valid state
   */
  static bool generateDump(std::string const &reason = "Manual dump", DumpType dumpType = DumpType::DEFAULT_AUTO);

  /**
   * @brief Manually trigger a dump generation with error code
   *
   * Non-throwing alternative to generateDump() that returns error information
   * via std::error_code instead of throwing exceptions.
   *
   * @param reason Optional reason for the dump generation
   * @param dumpType Type of dump to generate (uses current config if DEFAULT_AUTO)
   * @param ec Error code to be set on failure
   * @return true if dump was generated successfully, false otherwise
   *
   * @complexity O(1) for basic operations, O(n) where n is memory size for full dumps
   * @thread_safety This function is thread-safe and may be called concurrently
   * @exception_safety No-throw guarantee
   */
  static bool generateDump(std::string const &reason, DumpType dumpType, std::error_code &errorCode) noexcept;

  /**
   * @brief Generate a dump file with custom configuration
   *
   * @param config Complete dump configuration
   * @param reason Reason for generating the dump
   * @return true if successful, false otherwise
   * @throws std::runtime_error if not initialized
   */
  static bool generateDump(DumpConfiguration const &config, std::string const &reason = "Manual dump");

  /**
   * @brief Get the singleton instance
   *
   * @return Reference to the singleton instance
   * @note This method is thread-safe
   * @throws std::runtime_error if not initialized
   */
  static CoreDumpGenerator &instance();

  /**
   * @brief Get the current dump directory
   *
   * @return Current dump directory path
   */
  static std::string getDumpDirectory() noexcept;

  /**
   * @brief Get the current dump configuration
   *
   * @return Current dump configuration
   */
  static DumpConfiguration getCurrentConfiguration() noexcept;

  /**
   * @brief Set the dump type for future dumps
   *
   * @param dumpType New dump type to use
   * @return true if successfully set, false if not supported
   * @throws std::runtime_error if not initialized
   */
  static bool setDumpType(DumpType dumpType);

  /**
   * @brief Get the current dump type
   *
   * @return Current dump type
   */
  static DumpType getCurrentDumpType() noexcept;

private:
  // Private constructor for singleton pattern
  CoreDumpGenerator() = default;

  // Member variables
  static std::unique_ptr<CoreDumpGenerator> s_instance;
  static std::mutex s_mutex;
#if CPP11_OR_GREATER
  static std::once_flag s_initFlag;
#endif
  static std::string s_dumpDirectory;
#if CPP11_OR_GREATER
  static std::atomic<bool> s_initialized;
#else
  static bool s_initialized;
#endif
  static DumpConfiguration s_currentConfig;

  // Platform-specific initialization
  static void _platformInitialize();

  // Exception handling
  static void _setupExceptionHandling();
  static void _unhandledExceptionHandler();

#if DUMP_CREATOR_WINDOWS
  /**
   * @brief Setup Windows handlers
   */
  static void _setupWindowsHandlers();

  /**
   * @brief Create Windows dump with specific type
   */
  static bool _createWindowsDump(std::string const &filename, DumpConfiguration const &config);
#endif

#if DUMP_CREATOR_WINDOWS
  /**
   * @brief Windows-specific unhandled exception filter
   */
  static LONG WINAPI _windowsExceptionHandler(EXCEPTION_POINTERS *pExInfo) noexcept;

  /**
   * @brief Redirected exception filter to prevent removal
   */
  static LONG WINAPI _redirectedSetUnhandledExceptionFilter(EXCEPTION_POINTERS * /*ExceptionInfo*/) noexcept;
#endif

#if DUMP_CREATOR_UNIX
  /**
   * @brief UNIX-specific signal handler
   */
  static void _unixSignalHandler(int signum) noexcept;

  /**
   * @brief Generate core dump on UNIX
   */
  static void _generateCoreDump();

  /**
   * @brief Log core dump file size (UNIX helper)
   * @param filename The path to the core dump file
   */
  static void _logCoreDumpSize(std::string const &filename) noexcept;

  /**
   * @brief Setup signal handlers
   */
  static void _setupSignalHandlers();

  /**
   * @brief Setup core dump settings
   */
  static void _setupCoreDumpSettings();
#endif

  // Utility functions
  static std::string _generateDumpFilename(std::string const &prefix);
  static std::string _generateDumpFilename(DumpType dumpType);
  static std::string _getExecutableDirectory() noexcept;
  static void _logMessage(std::string const &message, bool isError = false);

  // Helper functions to reduce code duplication
  static std::string _convertWideStringToNarrow(std::wstring const &wideStr) noexcept;
  static void _logDumpCreationSuccess(std::string const &filename, size_t size, DumpType dumpType) noexcept;

  // Atomic file operations to prevent TOCTOU race conditions
  static bool _createFileAtomically(std::string const &filename, std::string const &content) noexcept;
  static bool _createDirectoryAtomically(std::string const &path) noexcept;

#if DUMP_CREATOR_WINDOWS
  /**
   * @brief Convert DumpType enum to MINIDUMP_TYPE flags
   * @param type The dump type to convert
   * @return Corresponding MINIDUMP_TYPE flags
   */
  static MINIDUMP_TYPE _getMinidumpType(DumpType type) noexcept;

  /**
   * @brief Validate MINIDUMP_TYPE flags combination
   * @param flags The MINIDUMP_TYPE flags to validate
   * @return true if flags combination is valid, false otherwise
   */
  static bool _isValidMinidumpType(MINIDUMP_TYPE flags) noexcept;
#endif

  // Security and validation functions
#if HAS_STRING_VIEW
  static bool _validateDirectory(std::string_view path) noexcept;
  static bool _validateFilename(std::string_view filename) noexcept;
  static std::string _sanitizePath(std::string_view path) noexcept;
#else
  static bool _validateDirectory(std::string const &path) noexcept;
  static bool _validateFilename(std::string const &filename) noexcept;
  static std::string _sanitizePath(std::string const &path) noexcept;
#endif

  // Platform-specific filesystem utilities (C++11 compatible)
  static bool _createDirectoryRecursive(std::string const &path) noexcept;

  // Security helper functions
  static std::string _generateSecureRandomComponent() noexcept;
  static std::string _generateFallbackRandomComponent() noexcept;
  static std::string _sanitizeFilenameComponent(std::string const &component) noexcept;
  static std::string _sanitizeLogMessage(std::string const &message) noexcept;
};

#endif // !CORE_DUMP_GENERATOR_HPP
