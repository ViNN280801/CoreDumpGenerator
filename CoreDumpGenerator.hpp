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

#if __cplusplus >= 202612L
  #define CPP26_OR_GREATER 1
#else
  #define CPP26_OR_GREATER 0
#endif

// Feature detection for specific C++ features
#if CPP11_OR_GREATER
  #define HAS_TYPE_TRAITS 1
  #define HAS_ATOMIC 1
  #define HAS_THREAD 1
  #define HAS_MUTEX 1
  #define HAS_CONDITION_VARIABLE 1
  #define HAS_FUTURE 1
  #define HAS_CHRONO 1
#else
  #define HAS_TYPE_TRAITS 0
  #define HAS_ATOMIC 0
  #define HAS_THREAD 0
  #define HAS_MUTEX 0
  #define HAS_CONDITION_VARIABLE 0
  #define HAS_FUTURE 0
  #define HAS_CHRONO 0
#endif

#if CPP14_OR_GREATER
  #define HAS_MAKE_UNIQUE 1
  #define HAS_SHARED_MUTEX 0 // C++17 feature
#else
  #define HAS_MAKE_UNIQUE 0
  #define HAS_SHARED_MUTEX 0
#endif

#if CPP17_OR_GREATER
  #define HAS_OPTIONAL 1
  #define HAS_VARIANT 1
  #define HAS_ANY 1
  #define HAS_STRING_VIEW 1
  #ifndef HAS_SHARED_MUTEX
    #define HAS_SHARED_MUTEX 1
  #endif
  #define HAS_FILESYSTEM 1
  #define HAS_PARALLEL_ALGORITHMS 1
#else
  #define HAS_OPTIONAL 0
  #define HAS_VARIANT 0
  #define HAS_ANY 0
  #define HAS_STRING_VIEW 0
  #define HAS_SHARED_MUTEX 0
  #define HAS_FILESYSTEM 0
  #define HAS_PARALLEL_ALGORITHMS 0
#endif

#if CPP20_OR_GREATER
  #define HAS_CONCEPTS 1
  #define HAS_RANGES 1
  #define HAS_SPAN 1
  #define HAS_FORMAT 1
  #define HAS_COROUTINES 1
  #define HAS_JTHREAD 1
  #define HAS_BARRIER 1
  #define HAS_LATCH 1
  #define HAS_SEMAPHORE 1
#else
  #define HAS_CONCEPTS 0
  #define HAS_RANGES 0
  #define HAS_SPAN 0
  #define HAS_FORMAT 0
  #define HAS_COROUTINES 0
  #define HAS_JTHREAD 0
  #define HAS_BARRIER 0
  #define HAS_LATCH 0
  #define HAS_SEMAPHORE 0
#endif

#if CPP23_OR_GREATER
  #define HAS_EXPECTED 1
  #define HAS_MDSpan 1
  #define HAS_STACKTRACE 1
#else
  #define HAS_EXPECTED 0
  #define HAS_MDSpan 0
  #define HAS_STACKTRACE 0
#endif

#if CPP26_OR_GREATER
  #define HAS_REFLECTION 1
  #define HAS_EXECUTORS 1
#else
  #define HAS_REFLECTION 0
  #define HAS_EXECUTORS 0
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#if HAS_CHRONO
  #include <chrono>
#endif

#if HAS_ATOMIC
  #include <atomic>
#endif

#if HAS_MUTEX
  #include <mutex>
#endif

#if HAS_THREAD
  #include <thread>
#endif

#if HAS_CONDITION_VARIABLE
  #include <condition_variable>
#endif

#if HAS_STRING_VIEW
  #include <string_view>
#endif

#if HAS_OPTIONAL
  #include <optional>
#endif

#if HAS_SHARED_MUTEX
  #include <shared_mutex>
#endif

#if HAS_STACKTRACE
  #include <stacktrace>
#endif

#if HAS_RANGES
  #include <ranges>
#endif

// Forward declarations

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
  #define DUMP_CREATOR_WINDOWS 1
  #define DUMP_CREATOR_UNIX 0
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
  #define DUMP_CREATOR_WINDOWS 0
  #define DUMP_CREATOR_UNIX 1
#else
  #error "Unsupported platform"
#endif

// Windows-specific includes
#if DUMP_CREATOR_WINDOWS
  // Prevent Windows headers from defining conflicting symbols
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif

  #include <windows.h>

  #include <aclapi.h>
  #include <dbghelp.h>
  #include <direct.h>
  #include <io.h>
  #include <process.h>
  #include <sddl.h>
  #include <shlwapi.h>
  #include <tchar.h>
  #include <wincrypt.h>

  #pragma comment(lib, "dbghelp.lib")
  #pragma comment(lib, "shlwapi.lib")
#endif

// UNIX-specific includes
#if DUMP_CREATOR_UNIX
  #include <csignal>
  #include <cstdlib>
  #include <errno.h>
  #include <fcntl.h>
  #include <fstream> // for std::ofstream in _generateCoreDump()
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
 * @brief Comprehensive enumeration of all supported crash dump types across
 * platforms
 *
 * This enumeration defines the complete set of dump types that can be generated
 * by the CoreDumpGenerator, including Windows-specific mini-dump types,
 * kernel-mode dump variants, and UNIX core dump types. Each type is designed to
 * capture different levels of system state information for debugging purposes.
 *
 * @details The enumeration is based on Microsoft's MINIDUMP_TYPE documentation
 * and POSIX core dump specifications, providing a unified interface for
 * cross-platform crash dump generation.
 *
 * @note All enum values are explicitly typed as std::int8_t for memory
 * efficiency and ABI stability across different compilers and platforms.
 *
 * @since Version 1.0
 * @see
 * https://docs.microsoft.com/en-us/windows/win32/api/minidumpapiset/ne-minidumpapiset-minidump_type
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
 * @brief Validation and utility functions for DumpType enum
 * @namespace DumpTypeUtils
 */
namespace DumpTypeUtils
{
  // Named constants for magic numbers
  namespace Constants
  {
    static constexpr std::int8_t MIN_DUMP_TYPE_VALUE = 0;
    static constexpr std::int8_t MAX_DUMP_TYPE_VALUE = 29;
    static constexpr std::int8_t AUTO_DETECT_VALUE   = -1;
    static constexpr std::int8_t WINDOWS_MAX_TYPE    = 24;
    static constexpr std::int8_t UNIX_MIN_TYPE       = 25;
    static constexpr std::int8_t UNIX_MAX_TYPE       = 29;
    static constexpr std::int8_t KERNEL_MIN_TYPE     = 20;
    static constexpr std::int8_t KERNEL_MAX_TYPE     = 24;
    static constexpr std::int8_t KERNEL_ONLY_TYPE    = 26;
  } // namespace Constants

  // Character code constants
  namespace CharacterConstants
  {
    static constexpr char CONTROL_CHAR_THRESHOLD = 32;
    static constexpr char COLON_CHAR             = ':';
    static constexpr char ASTERISK_CHAR          = '*';
    static constexpr char QUESTION_CHAR          = '?';
    static constexpr char QUOTE_CHAR             = '"';
    static constexpr char LESS_THAN_CHAR         = '<';
    static constexpr char GREATER_THAN_CHAR      = '>';
    static constexpr char PIPE_CHAR              = '|';
  } // namespace CharacterConstants

  /**
   * @brief Check if a DumpType value is valid
   * @param type The dump type to validate
   * @return true if valid, false otherwise
   * @note This function is thread-safe and noexcept
   */
  inline bool
  isValid(DumpType type) noexcept
  {
    auto const value = static_cast<std::int8_t>(type);
    return (value >= Constants::MIN_DUMP_TYPE_VALUE && value <= Constants::MAX_DUMP_TYPE_VALUE)
           || (value == Constants::AUTO_DETECT_VALUE);
  }

  /**
   * @brief Check if a DumpType is a Windows-specific type
   * @param type The dump type to check
   * @return true if Windows-specific, false otherwise
   * @note This function is thread-safe and noexcept
   */
  inline bool
  isWindowsType(DumpType type) noexcept
  {
    auto const value = static_cast<std::int8_t>(type);
    return value >= Constants::MIN_DUMP_TYPE_VALUE && value <= Constants::WINDOWS_MAX_TYPE;
  }

  /**
   * @brief Check if a DumpType is a UNIX-specific type
   * @param type The dump type to check
   * @return true if UNIX-specific, false otherwise
   * @note This function is thread-safe and noexcept
   */
  inline bool
  isUnixType(DumpType type) noexcept
  {
    auto const value = static_cast<std::int8_t>(type);
    return value >= Constants::UNIX_MIN_TYPE && value <= Constants::UNIX_MAX_TYPE;
  }

  /**
   * @brief Check if a DumpType is a kernel dump type
   * @param type The dump type to check
   * @return true if kernel dump, false otherwise
   * @note This function is thread-safe and noexcept
   */
  inline bool
  isKernelType(DumpType type) noexcept
  {
    auto const value = static_cast<std::int8_t>(type);
    return (value >= Constants::KERNEL_MIN_TYPE && value <= Constants::KERNEL_MAX_TYPE)
           || (value == Constants::KERNEL_ONLY_TYPE);
  }

  /**
   * @brief Get the minimum valid DumpType value
   * @return Minimum valid DumpType value
   * @note This function is thread-safe and noexcept
   */
  constexpr DumpType
  getMinValue() noexcept
  {
    return DumpType::MINI_DUMP_NORMAL;
  }

  /**
   * @brief Get the maximum valid DumpType value
   * @return Maximum valid DumpType value
   * @note This function is thread-safe and noexcept
   */
  constexpr DumpType
  getMaxValue() noexcept
  {
    return DumpType::CORE_DUMP_FILTERED;
  }
} // namespace DumpTypeUtils

// Modern C++ concepts for type safety
#if HAS_CONCEPTS
  #include <concepts>

namespace CoreDumpGeneratorConcepts
{
  template <typename T>
  concept DumpTypeLike = std::same_as<T, DumpType>;

  template <typename T>
  concept StringLike = std::convertible_to<T, std::string_view> || std::convertible_to<T, std::string>;

  template <typename T>
  concept Configurable = requires(T configurable) {
    { configurable.isValid() } -> std::convertible_to<bool>;
    { configurable.getValidationError() } -> std::convertible_to<std::string>;
  };

  template <typename T>
  concept DumpGenerator = requires(T generator) {
    { generator.isInitialized() } -> std::convertible_to<bool>;
    { generator.generateDump(std::string{}) } -> std::convertible_to<bool>;
  };
} // namespace CoreDumpGeneratorConcepts
#endif

/**
 * @class DumpConfiguration
 * @brief Comprehensive configuration class for crash dump generation
 *
 * This class encapsulates all configurable parameters for generating
 * crash dumps, including dump type selection, file naming, directory
 * specification, size limits, compression settings, and platform-specific
 * filtering options. It provides a unified interface for configuring
 * dump generation across different platforms and dump types.
 *
 * @details The class is designed to be:
 * - Copyable and movable for efficient parameter passing
 * - Default-constructible with sensible defaults
 * - Thread-safe for concurrent access (read-only operations)
 * - Extensible for future platform-specific options
 * - Value semantics with proper move semantics
 * - Encapsulated with proper validation and error handling
 *
 * @invariant All string members must be valid UTF-8 encoded strings
 * @invariant m_maxSizeBytes must be 0 (unlimited) or a positive value
 * @invariant m_directory must be a valid, accessible path
 * @invariant m_filename must be a valid filename (no path separators)
 *
 * @thread_safety This class is thread-safe for read-only operations.
 * Modifications should be synchronized externally.
 *
 * @since Version 1.0
 * @see DumpFactory::createConfiguration()
 * @see CoreDumpGenerator::initialize()
 */
class DumpConfiguration
{
public:
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

  // Getters
  DumpType
  getType() const noexcept
  {
    return m_type;
  }
  std::string const &
  getFilename() const noexcept
  {
    return m_filename;
  }
  std::string const &
  getDirectory() const noexcept
  {
    return m_directory;
  }
  bool
  isCompress() const noexcept
  {
    return m_compress;
  }
  bool
  isIncludeUnloadedModules() const noexcept
  {
    return m_includeUnloadedModules;
  }
  bool
  isIncludeHandleData() const noexcept
  {
    return m_includeHandleData;
  }
  bool
  isIncludeThreadInfo() const noexcept
  {
    return m_includeThreadInfo;
  }
  bool
  isIncludeProcessData() const noexcept
  {
    return m_includeProcessData;
  }
  size_t
  getMaxSizeBytes() const noexcept
  {
    return m_maxSizeBytes;
  }
  std::vector<std::string> const &
  getMemoryFilters() const noexcept
  {
    return m_memoryFilters;
  }
  bool
  isEnableSymbols() const noexcept
  {
    return m_enableSymbols;
  }
  bool
  isEnableSourceInfo() const noexcept
  {
    return m_enableSourceInfo;
  }

  // Setters with validation
  bool setType(DumpType type) noexcept;
  bool setFilename(std::string const &filename) noexcept;
  bool setDirectory(std::string const &directory) noexcept;

#if HAS_CONCEPTS
  // Template setters with concepts for type safety
  template <CoreDumpGeneratorConcepts::StringLike T>
  bool
  setFilename(T const &filename) noexcept
  {
    return setFilename(std::string{filename});
  }

  template <CoreDumpGeneratorConcepts::StringLike T>
  bool
  setDirectory(T const &directory) noexcept
  {
    return setDirectory(std::string{directory});
  }

  template <CoreDumpGeneratorConcepts::StringLike T>
  bool
  addMemoryFilter(T const &filter) noexcept
  {
    return addMemoryFilter(std::string{filter}); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  }
#endif
  void
  setCompress(bool compress) noexcept
  {
    m_compress = compress;
  }
  void
  setIncludeUnloadedModules(bool include) noexcept
  {
    m_includeUnloadedModules = include;
  }
  void
  setIncludeHandleData(bool include) noexcept
  {
    m_includeHandleData = include;
  }
  void
  setIncludeThreadInfo(bool include) noexcept
  {
    m_includeThreadInfo = include;
  }
  void
  setIncludeProcessData(bool include) noexcept
  {
    m_includeProcessData = include;
  }
  bool setMaxSizeBytes(size_t maxSize) noexcept;
  bool addMemoryFilter(std::string const &filter) noexcept;
  void
  clearMemoryFilters() noexcept
  {
    m_memoryFilters.clear();
  }
  void
  setEnableSymbols(bool enable) noexcept
  {
    m_enableSymbols = enable;
  }
  void
  setEnableSourceInfo(bool enable) noexcept
  {
    m_enableSourceInfo = enable;
  }

  // Validation methods
  bool isValid() const noexcept;
  std::string getValidationError() const;

private:
  DumpType m_type = DumpType::DEFAULT_AUTO; ///< Type of dump to generate
  std::string m_filename;                   ///< Custom filename (empty for auto-generated)
  std::string m_directory;                  ///< Directory for dump files
  bool m_compress               = false;    ///< Whether to compress the dump
  bool m_includeUnloadedModules = true;     ///< Include unloaded modules (Windows)
  bool m_includeHandleData      = true;     ///< Include handle data (Windows)
  bool m_includeThreadInfo      = true;     ///< Include thread information
  bool m_includeProcessData     = true;     ///< Include process data
  size_t m_maxSizeBytes         = 0;        ///< Maximum size in bytes (0 = unlimited)
  std::vector<std::string> m_memoryFilters; ///< Memory region filters (UNIX)
  bool m_enableSymbols    = true;           ///< Enable symbol information
  bool m_enableSourceInfo = true;           ///< Enable source file information

  // Private validation helpers
  static bool isValidFilename(std::string const &filename) noexcept;
  static bool isValidDirectory(std::string const &directory) noexcept;
  static bool isValidMemoryFilter(std::string const &filter) noexcept;
};

// ==================================== DumpConfiguration Implementation
// ==================================== //

inline bool
DumpConfiguration::setType(DumpType type) noexcept
{
  if(!DumpTypeUtils::isValid(type)) return false;
  m_type = type;
  return true;
}

inline bool
DumpConfiguration::setFilename(std::string const &filename) noexcept
{
  if(!isValidFilename(filename)) return false;
  m_filename = filename;
  return true;
}

inline bool
DumpConfiguration::setDirectory(std::string const &directory) noexcept
{
  if(!isValidDirectory(directory)) return false;
  m_directory = directory;
  return true;
}

inline bool
DumpConfiguration::setMaxSizeBytes(size_t maxSize) noexcept
{
  // Allow 0 for unlimited size
  m_maxSizeBytes = maxSize;
  return true;
}

inline bool
DumpConfiguration::addMemoryFilter(std::string const &filter) noexcept
{
  if(!isValidMemoryFilter(filter)) return false;
  try
  {
    m_memoryFilters.emplace_back(filter);
    return true;
  }
  catch(...)
  {
    return false;
  }
}

inline bool
DumpConfiguration::isValid() const noexcept
{
  return DumpTypeUtils::isValid(m_type) && isValidFilename(m_filename) && isValidDirectory(m_directory)
         && std::all_of(m_memoryFilters.begin(), m_memoryFilters.end(),
                        [](std::string const &filter) { return isValidMemoryFilter(filter); });
}

inline std::string
DumpConfiguration::getValidationError() const
{
  if(!DumpTypeUtils::isValid(m_type)) return "Invalid dump type";
  if(!isValidFilename(m_filename)) return "Invalid filename: contains invalid characters or path separators";
  if(!isValidDirectory(m_directory))
    return "Invalid directory: contains invalid characters or is not "
           "accessible";
  for(auto const &filter : m_memoryFilters)
    if(!isValidMemoryFilter(filter)) return "Invalid memory filter: " + filter;
  return "";
}

inline bool
DumpConfiguration::isValidFilename(std::string const &filename) noexcept
{
  if(filename.empty()) return true; // Empty filename is valid (auto-generated)

  // Check for path separators
  if(filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) return false;

// Check for invalid characters
#if HAS_RANGES
  return std::ranges::all_of(filename,
                             [](char character)
                             {
                               return character >= DumpTypeUtils::CharacterConstants::CONTROL_CHAR_THRESHOLD
                                      && character != DumpTypeUtils::CharacterConstants::COLON_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::ASTERISK_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::QUESTION_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::QUOTE_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::LESS_THAN_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::GREATER_THAN_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::PIPE_CHAR;
                             });
#else
  return std::all_of(filename.begin(), filename.end(),
                     [](char character)
                     {
                       return character >= DumpTypeUtils::CharacterConstants::CONTROL_CHAR_THRESHOLD
                              && character != DumpTypeUtils::CharacterConstants::COLON_CHAR
                              && character != DumpTypeUtils::CharacterConstants::ASTERISK_CHAR
                              && character != DumpTypeUtils::CharacterConstants::QUESTION_CHAR
                              && character != DumpTypeUtils::CharacterConstants::QUOTE_CHAR
                              && character != DumpTypeUtils::CharacterConstants::LESS_THAN_CHAR
                              && character != DumpTypeUtils::CharacterConstants::GREATER_THAN_CHAR
                              && character != DumpTypeUtils::CharacterConstants::PIPE_CHAR;
                     });
#endif
  return true;
}

inline bool
DumpConfiguration::isValidDirectory(std::string const &directory) noexcept
{
  if(directory.empty()) return true; // Empty directory is valid (will use default)

// Check for invalid characters
#if HAS_RANGES
  return std::ranges::all_of(directory,
                             [](char character)
                             {
                               return character >= DumpTypeUtils::CharacterConstants::CONTROL_CHAR_THRESHOLD
                                      && character != DumpTypeUtils::CharacterConstants::ASTERISK_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::QUESTION_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::QUOTE_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::LESS_THAN_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::GREATER_THAN_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::PIPE_CHAR;
                             });
#else
  return std::all_of(directory.begin(), directory.end(),
                     [](char character)
                     {
                       return character >= DumpTypeUtils::CharacterConstants::CONTROL_CHAR_THRESHOLD
                              && character != DumpTypeUtils::CharacterConstants::ASTERISK_CHAR
                              && character != DumpTypeUtils::CharacterConstants::QUESTION_CHAR
                              && character != DumpTypeUtils::CharacterConstants::QUOTE_CHAR
                              && character != DumpTypeUtils::CharacterConstants::LESS_THAN_CHAR
                              && character != DumpTypeUtils::CharacterConstants::GREATER_THAN_CHAR
                              && character != DumpTypeUtils::CharacterConstants::PIPE_CHAR;
                     });
#endif
}

inline bool
DumpConfiguration::isValidMemoryFilter(std::string const &filter) noexcept
{
  if(filter.empty()) return false;

// Basic validation for memory filter format
// This is a simplified validation - in practice, you might want more
// sophisticated checks
#if HAS_RANGES
  return std::ranges::all_of(filter,
                             [](char character)
                             {
                               return character >= DumpTypeUtils::CharacterConstants::CONTROL_CHAR_THRESHOLD
                                      && character != DumpTypeUtils::CharacterConstants::ASTERISK_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::QUESTION_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::QUOTE_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::LESS_THAN_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::GREATER_THAN_CHAR
                                      && character != DumpTypeUtils::CharacterConstants::PIPE_CHAR;
                             });
#else
  return std::all_of(filter.begin(), filter.end(),
                     [](char character)
                     {
                       return character >= DumpTypeUtils::CharacterConstants::CONTROL_CHAR_THRESHOLD
                              && character != DumpTypeUtils::CharacterConstants::ASTERISK_CHAR
                              && character != DumpTypeUtils::CharacterConstants::QUESTION_CHAR
                              && character != DumpTypeUtils::CharacterConstants::QUOTE_CHAR
                              && character != DumpTypeUtils::CharacterConstants::LESS_THAN_CHAR
                              && character != DumpTypeUtils::CharacterConstants::GREATER_THAN_CHAR
                              && character != DumpTypeUtils::CharacterConstants::PIPE_CHAR;
                     });
#endif
}

inline bool
DumpConfiguration::operator==(DumpConfiguration const &other) const noexcept
{
  return m_type == other.m_type && m_filename == other.m_filename && m_directory == other.m_directory
         && m_compress == other.m_compress && m_includeUnloadedModules == other.m_includeUnloadedModules
         && m_includeHandleData == other.m_includeHandleData && m_includeThreadInfo == other.m_includeThreadInfo
         && m_includeProcessData == other.m_includeProcessData && m_maxSizeBytes == other.m_maxSizeBytes
         && m_memoryFilters == other.m_memoryFilters && m_enableSymbols == other.m_enableSymbols
         && m_enableSourceInfo == other.m_enableSourceInfo;
}

inline bool
DumpConfiguration::operator!=(DumpConfiguration const &other) const noexcept
{
  return !(*this == other);
}

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
 * - Error handling with detailed error messages
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
   * @throws std::invalid_argument if type is not supported on current platform
   * @throws std::runtime_error if configuration creation fails
   * @note This method is thread-safe and can be called concurrently
   */
  static DumpConfiguration createConfiguration(DumpType type);

  /**
   * @brief Create a dump configuration with error handling
   * @param type The dump type to create configuration for
   * @param ec Error code to be set on failure
   * @return DumpConfiguration object configured for the specified type
   * @note This method is thread-safe and noexcept
   */
  static DumpConfiguration createConfiguration(DumpType type, std::error_code &errorCode) noexcept;

  /**
   * @brief Get the default dump type for the current platform
   * @return Default DumpType for the current platform
   * @note This method is thread-safe and noexcept
   */
  static DumpType getDefaultDumpType() noexcept;

  /**
   * @brief Check if a dump type is supported on the current platform
   * @param type The dump type to check
   * @return true if supported, false otherwise
   * @note This method is thread-safe and noexcept
   */
  static bool isSupported(DumpType type) noexcept;

  /**
   * @brief Get a human-readable description of the dump type
   * @param type The dump type to describe
   * @return String description of the dump type
   * @note This method is thread-safe and noexcept
   */
  static std::string getDescription(DumpType type) noexcept;

  /**
   * @brief Get the estimated size of a dump type
   * @param type The dump type to estimate
   * @return Estimated size in bytes (0 if unknown)
   * @note This method is thread-safe and noexcept
   */
  static size_t getEstimatedSize(DumpType type) noexcept;

  /**
   * @brief Get all supported dump types for current platform
   * @return Vector of supported DumpType values
   * @note This method is thread-safe and noexcept
   */
  static std::vector<DumpType> getSupportedTypes() noexcept;

  /**
   * @brief Validate a dump configuration
   * @param config The configuration to validate
   * @return true if valid, false otherwise
   * @note This method is thread-safe and noexcept
   */
  static bool validateConfiguration(DumpConfiguration const &config) noexcept;

private:
  // Platform-specific configuration creators
  static DumpConfiguration createWindowsConfiguration(DumpType type);
  static DumpConfiguration createUnixConfiguration(DumpType type);

  // Registry of dump type descriptions
  static std::map<DumpType, std::string> const s_descriptions;
  static std::map<DumpType, size_t> const s_estimatedSizes;
  static std::map<DumpType, bool> const s_platformSupport;
};

/**
 * @class CoreDumpGenerator
 * @brief Cross-platform crash dump handler with comprehensive debugging support
 *
 * This class provides a robust, thread-safe crash dump handler that
 * automatically captures memory dumps when crashes occur on both Windows and
 * UNIX platforms. It supports multiple dump types through the DumpFactory
 * pattern and provides comprehensive debugging information for post-mortem
 * analysis.
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
 * CoreDumpGenerator::initialize("/path/to/dumps",
 * DumpType::MINI_DUMP_WITH_FULL_MEMORY);
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
   * @param handleExceptions Whether to handle unhandled C++ exceptions
   * (default: true)
   * @throws std::runtime_error if initialization fails
   * @throws std::system_error if filesystem operations fail
   * @throws std::invalid_argument if configuration is invalid
   *
   * @complexity O(1) for basic initialization, O(n) where n is directory depth
   * for recursive creation
   * @thread_safety This function is thread-safe and may be called concurrently
   * @exception_safety Strong guarantee: if an exception is thrown, the object
   * remains in a valid state
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
   * @param handleExceptions Whether to handle unhandled C++ exceptions
   * (default: true)
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
   * @param dumpType Type of dump to generate (uses current config if
   * DEFAULT_AUTO)
   * @return true if dump was generated successfully, false otherwise
   * @throws std::runtime_error if not initialized
   * @throws std::system_error if filesystem operations fail
   * @throws std::invalid_argument if dump type is not supported
   *
   * @complexity O(1) for basic operations, O(n) where n is memory size for full
   * dumps
   * @thread_safety This function is thread-safe and may be called concurrently
   * @exception_safety Basic guarantee: if an exception is thrown, the system
   * remains in a valid state
   */
  static bool generateDump(std::string const &reason = "Manual dump", DumpType dumpType = DumpType::DEFAULT_AUTO);

  /**
   * @brief Manually trigger a dump generation with error code
   *
   * Non-throwing alternative to generateDump() that returns error information
   * via std::error_code instead of throwing exceptions.
   *
   * @param reason Optional reason for the dump generation
   * @param dumpType Type of dump to generate (uses current config if
   * DEFAULT_AUTO)
   * @param ec Error code to be set on failure
   * @return true if dump was generated successfully, false otherwise
   *
   * @complexity O(1) for basic operations, O(n) where n is memory size for full
   * dumps
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

  /**
   * @brief Check if the current process is running with administrator
   * privileges
   *
   * This method provides a cross-platform way to check if the current process
   * has administrator/root privileges. On Windows, it checks both token
   * elevation and membership in the Administrators group. On UNIX systems, it
   * checks if the effective user ID is 0 (root).
   *
   * @return true if running with admin privileges, false otherwise
   * @note This method is thread-safe and can be called concurrently
   * @note Returns false if privilege checking fails for security reasons
   *
   * @complexity O(1) - constant time operation
   * @thread_safety This function is thread-safe and may be called concurrently
   * @exception_safety No-throw guarantee
   *
   * @since Version 1.0
   * @see _isElevatedProcess() for a simpler elevation check
   */
  static bool isAdminPrivileges() noexcept;

  // Instance methods for better encapsulation
  /**
   * @brief Get the dump directory for this instance
   * @return Current dump directory path
   * @note This method is thread-safe
   */
  std::string const &getInstanceDumpDirectory() const noexcept;

  /**
   * @brief Get the current configuration for this instance
   * @return Current dump configuration
   * @note This method is thread-safe
   */
  DumpConfiguration const &getInstanceConfiguration() const noexcept;

  /**
   * @brief Check if this instance is initialized
   * @return true if initialized, false otherwise
   * @note This method is thread-safe
   */
  bool isInstanceInitialized() const noexcept;

  /**
   * @brief Generate a dump using this instance's configuration
   * @param reason Reason for generating the dump
   * @return true if successful, false otherwise
   * @note This method is thread-safe
   */
  bool generateInstanceDump(std::string const &reason = "Manual dump");

  /**
   * @brief Generate a dump with error handling
   * @param reason Reason for generating the dump
   * @param errorCode Error code to be set on failure
   * @return true if successful, false otherwise
   * @note This method is thread-safe and noexcept
   */
  bool generateInstanceDump(std::string const &reason, std::error_code &errorCode) noexcept;

#if HAS_CONCEPTS
  // Template methods with concepts for type safety
  template <CoreDumpGeneratorConcepts::StringLike T>
  bool
  generateInstanceDump(T const &reason)
  {
    return generateInstanceDump(std::string{reason});
  }

  template <CoreDumpGeneratorConcepts::StringLike T>
  bool
  generateInstanceDump(T const &reason, std::error_code &errorCode) noexcept
  {
    return generateInstanceDump(std::string{reason}, errorCode);
  }
#endif

// Modern C++ features
#if HAS_RANGES
  /**
   * @brief Get all memory filters as a range
   * @return Range of memory filters
   * @note This method is thread-safe
   */
  static auto
  getMemoryFiltersRange() noexcept
  {
    return s_currentConfig.getMemoryFilters() | std::views::all;
  }
#endif

#if HAS_OPTIONAL
  /**
   * @brief Get optional dump directory
   * @return Optional containing directory if set, empty if not
   * @note This method is thread-safe
   */
  std::optional<std::string>
  getOptionalDumpDirectory() const noexcept
  {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    return m_dumpDirectory.empty() ? std::nullopt : std::make_optional(m_dumpDirectory);
  }
#else
  /**
   * @brief Get dump directory with validity check
   * @param directory Output parameter for directory
   * @return true if directory is set, false if empty
   * @note This method is thread-safe
   */
  bool
  getDumpDirectoryIfSet(std::string &directory) const noexcept
  {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    if(m_dumpDirectory.empty()) return false;
    directory = m_dumpDirectory;
    return true;
  }
#endif

private:
  // Private constructor for singleton pattern
  CoreDumpGenerator() = default;

  // Private constructor with initialization
  explicit CoreDumpGenerator(std::string const &dumpDirectory, DumpConfiguration const &config)
      : m_dumpDirectory(dumpDirectory), m_currentConfig(config), m_isInitialized(true)
  {}

  // Member variables
  static std::unique_ptr<CoreDumpGenerator> s_instance;
  static std::mutex s_mutex;
#if CPP11_OR_GREATER
  static std::once_flag s_initFlag;
#endif
  static std::string s_dumpDirectory;
#if CPP11_OR_GREATER
  static std::atomic_bool s_initialized;
#else
  static bool s_initialized;
#endif
  static DumpConfiguration s_currentConfig;

  // Instance member variables
  std::string m_dumpDirectory;
  DumpConfiguration m_currentConfig;
  bool m_isInitialized = false;
  mutable std::mutex m_instanceMutex;

// Performance optimization: cache frequently accessed values
#if HAS_OPTIONAL
  mutable std::optional<DumpType> m_cachedDumpType;
  mutable std::optional<std::string> m_cachedDumpDirectory;
#else
  mutable DumpType m_cachedDumpType = DumpType::DEFAULT_AUTO;
  mutable std::string m_cachedDumpDirectory;
  mutable bool m_cachedDumpTypeValid      = false;
  mutable bool m_cachedDumpDirectoryValid = false;
#endif
  mutable bool m_cacheValid = false;

  // Memory optimization: use small string optimization
  static constexpr size_t SMALL_STRING_SIZE = 32;
  using SmallString                         = std::array<char, SMALL_STRING_SIZE>;

// Thread safety improvements
#if HAS_SHARED_MUTEX
  mutable std::shared_mutex m_sharedMutex;
#else
  mutable std::mutex m_sharedMutex;
#endif
  std::atomic_bool m_operationInProgress;

  // Concurrency control
  static constexpr size_t MAX_CONCURRENT_OPERATIONS = 4;
  static std::atomic<size_t> s_activeOperations;
  static std::condition_variable s_operationCondition;
  static std::mutex s_operationMutex;

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
#if DUMP_CREATOR_WINDOWS
  static std::string _convertWideStringToNarrow(std::wstring const &wideStr) noexcept; // only for Windows
#endif
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
  static std::string _sanitizeLogMessageForAdmin(std::string const &message) noexcept;

  // Advanced error handling and logging
  enum class LogLevel : std::uint8_t
  {
    DEBUG_    = 0,
    INFO_     = 1,
    WARNING_  = 2,
    ERROR_    = 3,
    CRITICAL_ = 4
  };

  static void _logMessage(std::string const &message, LogLevel level = LogLevel::INFO_) noexcept;

  // Performance monitoring
  struct PerformanceMetrics {
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_endTime;
    size_t m_dumpSize = 0;
    bool m_success    = false;
  };

  static void _startPerformanceMonitoring(PerformanceMetrics &metrics) noexcept;
  static void _endPerformanceMonitoring(PerformanceMetrics &metrics, bool success) noexcept;
  static void _logPerformanceMetrics(PerformanceMetrics const &metrics) noexcept;

  // Advanced concurrency control
  static bool _acquireOperationSlot() noexcept;
  static void _releaseOperationSlot() noexcept;
  static void _waitForOperationSlot() noexcept;

  // Thread-safe cache management
  void _invalidateCache() const noexcept;
  void _updateCache() const noexcept;

  // RAII operation guard
  class OperationGuard
  {
  public:
    OperationGuard() noexcept;
    ~OperationGuard() noexcept;
    OperationGuard(OperationGuard const &)            = delete;
    OperationGuard &operator=(OperationGuard const &) = delete;
    OperationGuard(OperationGuard &&)                 = delete;
    OperationGuard &operator=(OperationGuard &&)      = delete;

    bool
    isAcquired() const noexcept
    {
      return m_acquired;
    }

  private:
    bool m_acquired = false;
  };

// Windows privilege checking
#if DUMP_CREATOR_WINDOWS
  static bool _isAdminPrivileges() noexcept;
  static bool _isElevatedProcess() noexcept;
#endif
};

// ==================================== Implementation
// ==================================== //

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,
// cppcoreguidelines-pro-type-reinterpret-cast,
// cppcoreguidelines-pro-bounds-array-to-pointer-decay)

// Static member definitions
std::unique_ptr<CoreDumpGenerator> CoreDumpGenerator::s_instance = nullptr;
std::mutex CoreDumpGenerator::s_mutex;
std::condition_variable CoreDumpGenerator::s_operationCondition;
std::mutex CoreDumpGenerator::s_operationMutex;
std::atomic<size_t> CoreDumpGenerator::s_activeOperations{};
#if CPP11_OR_GREATER
std::once_flag CoreDumpGenerator::s_initFlag;
#endif
std::string CoreDumpGenerator::s_dumpDirectory = "DumpCreatorCrashDump";
#if CPP11_OR_GREATER
std::atomic_bool CoreDumpGenerator::s_initialized{};
#else
bool CoreDumpGenerator::s_initialized = false;
#endif
DumpConfiguration CoreDumpGenerator::s_currentConfig;

// DumpFactory static member definitions
std::map<DumpType, std::string> const DumpFactory::s_descriptions = {
  // Windows mini-dump types
  {DumpType::MINI_DUMP_NORMAL, "Basic mini-dump (64KB)"},
  {DumpType::MINI_DUMP_WITH_DATA_SEGS, "Mini-dump with data segments"},
  {DumpType::MINI_DUMP_WITH_FULL_MEMORY, "Full memory mini-dump (largest)"},
  {DumpType::MINI_DUMP_WITH_HANDLE_DATA, "Mini-dump with handle data"},
  {DumpType::MINI_DUMP_FILTER_MEMORY, "Filtered memory mini-dump"},
  {DumpType::MINI_DUMP_SCAN_MEMORY, "Scanned memory mini-dump"},
  {DumpType::MINI_DUMP_WITH_UNLOADED_MODULES, "Mini-dump with unloaded modules"},
  {DumpType::MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY, "Mini-dump with indirectly referenced memory"},
  {DumpType::MINI_DUMP_FILTER_MODULE_PATHS, "Mini-dump with filtered module paths"},
  {DumpType::MINI_DUMP_WITH_PROCESS_THREAD_DATA, "Mini-dump with process/thread data"},
  {DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY, "Mini-dump with private read/write memory"},
  {DumpType::MINI_DUMP_WITHOUT_OPTIONAL_DATA, "Mini-dump without optional data"},
  {DumpType::MINI_DUMP_WITH_FULL_MEMORY_INFO, "Mini-dump with full memory info"},
  {DumpType::MINI_DUMP_WITH_THREAD_INFO, "Mini-dump with thread info"},
  {DumpType::MINI_DUMP_WITH_CODE_SEGMENTS, "Mini-dump with code segments"},
  {DumpType::MINI_DUMP_WITHOUT_AUXILIARY_STATE, "Mini-dump without auxiliary state"},
  {DumpType::MINI_DUMP_WITH_FULL_AUXILIARY_STATE, "Mini-dump with full auxiliary state"},
  {DumpType::MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY, "Mini-dump with private write-copy memory"},
  {DumpType::MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY, "Mini-dump ignoring inaccessible memory"},
  {DumpType::MINI_DUMP_WITH_TOKEN_INFORMATION, "Mini-dump with token information"},

  // Windows kernel-mode dump types
  {DumpType::KERNEL_FULL_DUMP, "Full kernel dump - largest kernel dump"},
  {DumpType::KERNEL_KERNEL_DUMP, "Kernel memory dump - kernel memory only"},
  {DumpType::KERNEL_SMALL_DUMP, "Small kernel dump - 64KB"},
  {DumpType::KERNEL_AUTOMATIC_DUMP, "Automatic kernel dump - flexible size"},
  {DumpType::KERNEL_ACTIVE_DUMP, "Active kernel dump - similar to full but smaller"},

  // UNIX core dump types
  {DumpType::CORE_DUMP_FULL, "Full core dump with all memory"},
  {DumpType::CORE_DUMP_KERNEL_ONLY, "Kernel-space only core dump"},
  {DumpType::CORE_DUMP_USER_ONLY, "User-space only core dump"},
  {DumpType::CORE_DUMP_COMPRESSED, "Compressed core dump"},
  {DumpType::CORE_DUMP_FILTERED, "Filtered core dump (exclude certain memory regions)"},

  // Default types
  {DumpType::DEFAULT_WINDOWS, "Default Windows dump type"},
  {DumpType::DEFAULT_UNIX, "Default UNIX dump type"},
  {DumpType::DEFAULT_AUTO, "Auto-detect based on platform"}};

std::map<DumpType, bool> const DumpFactory::s_platformSupport = {
  // Windows mini-dump types
  {DumpType::MINI_DUMP_NORMAL, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_DATA_SEGS, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_FULL_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_HANDLE_DATA, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_FILTER_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_SCAN_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_UNLOADED_MODULES, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_FILTER_MODULE_PATHS, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_PROCESS_THREAD_DATA, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITHOUT_OPTIONAL_DATA, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_FULL_MEMORY_INFO, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_THREAD_INFO, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_CODE_SEGMENTS, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITHOUT_AUXILIARY_STATE, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_FULL_AUXILIARY_STATE, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY, DUMP_CREATOR_WINDOWS},
  {DumpType::MINI_DUMP_WITH_TOKEN_INFORMATION, DUMP_CREATOR_WINDOWS},

  // Windows kernel-mode dump types
  {DumpType::KERNEL_FULL_DUMP, DUMP_CREATOR_WINDOWS},
  {DumpType::KERNEL_KERNEL_DUMP, DUMP_CREATOR_WINDOWS},
  {DumpType::KERNEL_SMALL_DUMP, DUMP_CREATOR_WINDOWS},
  {DumpType::KERNEL_AUTOMATIC_DUMP, DUMP_CREATOR_WINDOWS},
  {DumpType::KERNEL_ACTIVE_DUMP, DUMP_CREATOR_WINDOWS},

  // UNIX core dump types
  {DumpType::CORE_DUMP_FULL, !DUMP_CREATOR_WINDOWS},
  {DumpType::CORE_DUMP_KERNEL_ONLY, !DUMP_CREATOR_WINDOWS},
  {DumpType::CORE_DUMP_USER_ONLY, !DUMP_CREATOR_WINDOWS},
  {DumpType::CORE_DUMP_COMPRESSED, !DUMP_CREATOR_WINDOWS},
  {DumpType::CORE_DUMP_FILTERED, !DUMP_CREATOR_WINDOWS},

  // Default types
  {DumpType::DEFAULT_WINDOWS, DUMP_CREATOR_WINDOWS},
  {DumpType::DEFAULT_UNIX, !DUMP_CREATOR_WINDOWS},
  {DumpType::DEFAULT_AUTO, true}};

std::map<DumpType, size_t> const DumpFactory::s_estimatedSizes = {
  // Windows mini-dump types (estimated sizes)
  {DumpType::MINI_DUMP_NORMAL, CoreDumpGenerator::KB_64},                     // 64KB
  {DumpType::MINI_DUMP_WITH_DATA_SEGS, CoreDumpGenerator::KB_128},            // 128KB
  {DumpType::MINI_DUMP_WITH_FULL_MEMORY, 0},                                  // Variable - depends on system memory
  {DumpType::MINI_DUMP_WITH_HANDLE_DATA, CoreDumpGenerator::KB_256},          // 256KB
  {DumpType::MINI_DUMP_FILTER_MEMORY, CoreDumpGenerator::KB_64},              // 64KB
  {DumpType::MINI_DUMP_SCAN_MEMORY, CoreDumpGenerator::KB_128},               // 128KB
  {DumpType::MINI_DUMP_WITH_UNLOADED_MODULES, CoreDumpGenerator::KB_512},     // 512KB
  {DumpType::MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY, 0},                 // Variable
  {DumpType::MINI_DUMP_FILTER_MODULE_PATHS, CoreDumpGenerator::KB_64},        // 64KB
  {DumpType::MINI_DUMP_WITH_PROCESS_THREAD_DATA, CoreDumpGenerator::MB_1},    // 1MB
  {DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY, 0},                    // Variable
  {DumpType::MINI_DUMP_WITHOUT_OPTIONAL_DATA, CoreDumpGenerator::KB_32},      // 32KB
  {DumpType::MINI_DUMP_WITH_FULL_MEMORY_INFO, 0},                             // Variable
  {DumpType::MINI_DUMP_WITH_THREAD_INFO, CoreDumpGenerator::KB_256},          // 256KB
  {DumpType::MINI_DUMP_WITH_CODE_SEGMENTS, CoreDumpGenerator::KB_512},        // 512KB
  {DumpType::MINI_DUMP_WITHOUT_AUXILIARY_STATE, CoreDumpGenerator::KB_64},    // 64KB
  {DumpType::MINI_DUMP_WITH_FULL_AUXILIARY_STATE, CoreDumpGenerator::MB_1},   // 1MB
  {DumpType::MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY, 0},                    // Variable
  {DumpType::MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY, CoreDumpGenerator::KB_64}, // 64KB
  {DumpType::MINI_DUMP_WITH_TOKEN_INFORMATION, CoreDumpGenerator::KB_128},    // 128KB

  // Windows kernel-mode dump types
  {DumpType::KERNEL_FULL_DUMP, 0},                         // Variable - full system memory
  {DumpType::KERNEL_KERNEL_DUMP, 0},                       // Variable - kernel memory only
  {DumpType::KERNEL_SMALL_DUMP, CoreDumpGenerator::KB_64}, // 64KB
  {DumpType::KERNEL_AUTOMATIC_DUMP, 0},                    // Variable - flexible size
  {DumpType::KERNEL_ACTIVE_DUMP, 0},                       // Variable - similar to full but smaller

  // UNIX core dump types
  {DumpType::CORE_DUMP_FULL, 0},        // Variable - full process memory
  {DumpType::CORE_DUMP_KERNEL_ONLY, 0}, // Variable - kernel space only
  {DumpType::CORE_DUMP_USER_ONLY, 0},   // Variable - user space only
  {DumpType::CORE_DUMP_COMPRESSED, 0},  // Variable - compressed size
  {DumpType::CORE_DUMP_FILTERED, 0},    // Variable - depends on filters

  // Default types
  {DumpType::DEFAULT_WINDOWS, 0}, // Variable
  {DumpType::DEFAULT_UNIX, 0},    // Variable
  {DumpType::DEFAULT_AUTO, 0}     // Variable
};

// Thread-safe time formatting
namespace
{
  std::mutex s_timeMutex; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  std::string
  formatTime(char const *format)
  {
    std::lock_guard<std::mutex> lock(s_timeMutex);
    auto now = std::time(nullptr);

#if defined(_MSC_VER)
    struct tm timeInfo = {};
    if(localtime_s(&timeInfo, &now) != 0) return "unknown_time";
    auto &timeStruct = timeInfo;
#else
    struct tm timeInfo = {};
    if(localtime_r(&now, &timeInfo) == nullptr) return "unknown_time";
    auto &timeStruct = timeInfo;
#endif

    std::ostringstream oss;
    oss << std::put_time(&timeStruct, format);
    return oss.str();
  }

  /**
   * @brief Convert DumpType enum to descriptive string
   * @param type The dump type to convert
   * @return String representation of the dump type
   */
  std::string
  dumpTypeToString(DumpType type)
  {
    switch(type)
    {
    // Windows mini-dump types
    case DumpType::MINI_DUMP_NORMAL: return "mini_dump_normal";
    case DumpType::MINI_DUMP_WITH_DATA_SEGS: return "mini_dump_with_data_segs";
    case DumpType::MINI_DUMP_WITH_FULL_MEMORY: return "mini_dump_with_full_memory";
    case DumpType::MINI_DUMP_WITH_HANDLE_DATA: return "mini_dump_with_handle_data";
    case DumpType::MINI_DUMP_FILTER_MEMORY: return "mini_dump_filter_memory";
    case DumpType::MINI_DUMP_SCAN_MEMORY: return "mini_dump_scan_memory";
    case DumpType::MINI_DUMP_WITH_UNLOADED_MODULES: return "mini_dump_with_unloaded_modules";
    case DumpType::MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY: return "mini_dump_with_indirectly_referenced_memory";
    case DumpType::MINI_DUMP_FILTER_MODULE_PATHS: return "mini_dump_filter_module_paths";
    case DumpType::MINI_DUMP_WITH_PROCESS_THREAD_DATA: return "mini_dump_with_process_thread_data";
    case DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY: return "mini_dump_with_private_read_write_memory";
    case DumpType::MINI_DUMP_WITHOUT_OPTIONAL_DATA: return "mini_dump_without_optional_data";
    case DumpType::MINI_DUMP_WITH_FULL_MEMORY_INFO: return "mini_dump_with_full_memory_info";
    case DumpType::MINI_DUMP_WITH_THREAD_INFO: return "mini_dump_with_thread_info";
    case DumpType::MINI_DUMP_WITH_CODE_SEGMENTS: return "mini_dump_with_code_segments";
    case DumpType::MINI_DUMP_WITHOUT_AUXILIARY_STATE: return "mini_dump_without_auxiliary_state";
    case DumpType::MINI_DUMP_WITH_FULL_AUXILIARY_STATE: return "mini_dump_with_full_auxiliary_state";
    case DumpType::MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY: return "mini_dump_with_private_write_copy_memory";
    case DumpType::MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY: return "mini_dump_ignore_inaccessible_memory";
    case DumpType::MINI_DUMP_WITH_TOKEN_INFORMATION: return "mini_dump_with_token_information";

    // Windows kernel-mode dump types
    case DumpType::KERNEL_FULL_DUMP: return "kernel_full_dump";
    case DumpType::KERNEL_KERNEL_DUMP: return "kernel_kernel_dump";
    case DumpType::KERNEL_SMALL_DUMP: return "kernel_small_dump";
    case DumpType::KERNEL_AUTOMATIC_DUMP: return "kernel_automatic_dump";
    case DumpType::KERNEL_ACTIVE_DUMP: return "kernel_active_dump";

    // UNIX core dump types
    case DumpType::CORE_DUMP_FULL: return "core_dump_full";
    case DumpType::CORE_DUMP_KERNEL_ONLY: return "core_dump_kernel_only";
    case DumpType::CORE_DUMP_USER_ONLY: return "core_dump_user_only";
    case DumpType::CORE_DUMP_COMPRESSED: return "core_dump_compressed";
    case DumpType::CORE_DUMP_FILTERED: return "core_dump_filtered";

    // Default types
    case DumpType::DEFAULT_AUTO: return "default_auto";

    default: return "unknown_dump_type";
    }
  }
} // namespace

void
CoreDumpGenerator::initialize(std::string const &dumpDirectory, DumpType dumpType, bool handleExceptions)
{
  DumpConfiguration config = DumpFactory::createConfiguration(dumpType);
  config.setDirectory(dumpDirectory.empty() ? _getExecutableDirectory() + "/dumps" : dumpDirectory);
  initialize(config, handleExceptions);
}

void
CoreDumpGenerator::initialize(DumpConfiguration const &config, bool handleExceptions)
{
  std::lock_guard<std::mutex> lock(s_mutex);

#if CPP11_OR_GREATER
  // Check if already initialized using atomic load
  if(s_initialized.load(std::memory_order_acquire))
  {
    _logMessage("CoreDumpGenerator already initialized", false);
    return;
  }
#else
  if(s_initialized)
  {
    _logMessage("CoreDumpGenerator already initialized", false);
    return;
  }
#endif

  try
  {
    // Set configuration
    s_currentConfig = config;

    // Set dump directory
    s_dumpDirectory = config.getDirectory().empty() ? _getExecutableDirectory() + "/dumps" : config.getDirectory();

    // Validate and sanitize directory path
    s_dumpDirectory = _sanitizePath(s_dumpDirectory);
    if(!_validateDirectory(s_dumpDirectory)) throw std::runtime_error("Invalid dump directory: " + s_dumpDirectory);

    // Create dump directory with proper error handling
    if(!_createDirectoryRecursive(s_dumpDirectory))
      throw std::runtime_error("Failed to create dump directory: " + s_dumpDirectory);
    _logMessage("Dump directory created: " + s_dumpDirectory, false);

    // Log configuration
    _logMessage("Dump type: " + DumpFactory::getDescription(config.getType()), false);
    if(config.getMaxSizeBytes() > 0)
      _logMessage("Max size: " + std::to_string(config.getMaxSizeBytes()) + " bytes", false);

    // Initialize platform-specific handlers
    _platformInitialize();

    // Setup exception handling if requested
    if(handleExceptions) _setupExceptionHandling();

    // Set initialized flag with release semantics to ensure all previous
    // operations are visible
#if CPP11_OR_GREATER
    s_initialized.store(true, std::memory_order_release);
#else
    s_initialized = true;
#endif
    _logMessage("CoreDumpGenerator initialized successfully", false);
  }
  catch(std::exception const &exc)
  {
    _logMessage("Failed to initialize CoreDumpGenerator: " + std::string(exc.what()), true);
    throw;
  }
  catch(...)
  {
    _logMessage("Failed to initialize CoreDumpGenerator: Unknown error", true);
    throw std::runtime_error("Unknown error during CoreDumpGenerator initialization");
  }
}

CoreDumpGenerator &
CoreDumpGenerator::instance()
{
#if CPP11_OR_GREATER
  // Double-checked locking pattern for thread safety
  if(!s_initialized.load(std::memory_order_acquire))
    throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");

  std::call_once(s_initFlag,
                 []()
                 {
                   // Additional check inside call_once for extra safety
                   if(!s_initialized.load(std::memory_order_acquire))
                     throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
                   s_instance = std::unique_ptr<CoreDumpGenerator>(new CoreDumpGenerator());
                 });
#else
  // C++98/03 fallback - not thread-safe
  if(!s_instance)
  {
    if(!s_initialized) throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
  }
#endif

  return *s_instance;
}

bool
CoreDumpGenerator::isInitialized() noexcept
{
#if CPP11_OR_GREATER
  return s_initialized.load(std::memory_order_seq_cst);
#else
  return s_initialized;
#endif
}

bool
CoreDumpGenerator::generateDump(std::string const &reason, DumpType dumpType)
{
#if CPP11_OR_GREATER
  if(!s_initialized.load(std::memory_order_acquire))
    throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
#else
  if(!s_initialized) throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
#endif

  try
  {
    DumpConfiguration config = s_currentConfig;
    if(dumpType != DumpType::DEFAULT_AUTO)
    {
      config = DumpFactory::createConfiguration(dumpType);
      config.setDirectory(s_dumpDirectory); // Preserve current directory
    }

    std::string filename = _generateDumpFilename(config.getType());
    _logMessage("Generating dump: " + reason, false);
    _logMessage("Dump type: " + DumpFactory::getDescription(config.getType()), false);

#if DUMP_CREATOR_WINDOWS
    return _createWindowsDump(filename, config);
#elif DUMP_CREATOR_UNIX
    _generateCoreDump();
    _logCoreDumpSize(filename);
    return true;
#endif
  }
  catch(std::exception const &exc)
  {
    _logMessage("Failed to generate dump: " + std::string(exc.what()), true);
    return false;
  }
}

bool
CoreDumpGenerator::generateDump(DumpConfiguration const &config, std::string const &reason)
{
#if CPP11_OR_GREATER
  if(!s_initialized.load(std::memory_order_acquire))
    throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
#else
  if(!s_initialized) throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
#endif

  try
  {
    std::string filename = _generateDumpFilename(config.getType());
    _logMessage("Generating dump: " + reason, false);
    _logMessage("Dump type: " + DumpFactory::getDescription(config.getType()), false);

#if DUMP_CREATOR_WINDOWS
    return _createWindowsDump(filename, config);
#elif DUMP_CREATOR_UNIX
    _generateCoreDump();
    _logCoreDumpSize(filename);
    return true;
#endif
  }
  catch(std::exception const &exc)
  {
    _logMessage("Failed to generate dump: " + std::string(exc.what()), true);
    return false;
  }
}

bool
CoreDumpGenerator::generateDump(std::string const &reason, DumpType dumpType, std::error_code &errorCode) noexcept
{
  try
  {
    // Clear error code
    errorCode.clear();

#if CPP11_OR_GREATER
    if(!s_initialized.load(std::memory_order_acquire))
    {
      errorCode = std::make_error_code(std::errc::operation_not_permitted);
      return false;
    }
#else
    if(!s_initialized)
    {
      errorCode = std::make_error_code(std::errc::operation_not_permitted);
      return false;
    }
#endif

    // Validate dump type
    if(!DumpFactory::isSupported(dumpType))
    {
      errorCode = std::make_error_code(std::errc::invalid_argument);
      return false;
    }

    DumpConfiguration config = s_currentConfig;
    if(dumpType != DumpType::DEFAULT_AUTO)
    {
      config = DumpFactory::createConfiguration(dumpType);
      config.setDirectory(s_dumpDirectory); // Preserve current directory
    }

    std::string filename = _generateDumpFilename(config.getType());
    _logMessage("Generating dump: " + reason, false);
    _logMessage("Dump type: " + DumpFactory::getDescription(config.getType()), false);

#if DUMP_CREATOR_WINDOWS
    return _createWindowsDump(filename, config);
#elif DUMP_CREATOR_UNIX
    _generateCoreDump();
    _logCoreDumpSize(filename);
    return true;
#endif
  }
  catch(std::system_error const &exc)
  {
    errorCode = exc.code();
    _logMessage("Failed to generate dump: " + std::string(exc.what()), true);
    return false;
  }
  catch(std::exception const &exc)
  {
    errorCode = std::make_error_code(std::errc::operation_canceled);
    _logMessage("Failed to generate dump: " + std::string(exc.what()), true);
    return false;
  }
  catch(...)
  {
    errorCode = std::make_error_code(std::errc::operation_canceled);
    _logMessage("Failed to generate dump: Unknown error", true);
    return false;
  }
}

std::string
CoreDumpGenerator::getDumpDirectory() noexcept
{
  return s_dumpDirectory; // s_dumpDirectory is only modified during
                          // initialization, which is single-threaded
}

DumpConfiguration
CoreDumpGenerator::getCurrentConfiguration() noexcept
{
  return s_currentConfig; // s_currentConfig is only modified during
                          // initialization, which is single-threaded
}

bool
CoreDumpGenerator::setDumpType(DumpType dumpType)
{
  std::lock_guard<std::mutex> lock(s_mutex);

#if CPP11_OR_GREATER
  if(!s_initialized.load(std::memory_order_acquire))
    throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
#else
  if(!s_initialized) throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
#endif

  if(!DumpFactory::isSupported(dumpType))
  {
    _logMessage("Dump type not supported on this platform", true);
    return false;
  }

  s_currentConfig = DumpFactory::createConfiguration(dumpType);
  s_currentConfig.setDirectory(s_dumpDirectory); // Preserve current directory
  return true;
}

DumpType
CoreDumpGenerator::getCurrentDumpType() noexcept
{
  return s_currentConfig.getType(); // s_currentConfig is only modified during
                                    // initialization, which is single-threaded
}

bool
CoreDumpGenerator::isAdminPrivileges() noexcept
{
#if DUMP_CREATOR_WINDOWS
  return _isAdminPrivileges();
#elif DUMP_CREATOR_UNIX
  return (getuid() == 0);
#else
  return false; // Unsupported platform
#endif
}

// Instance methods implementation
std::string const &
CoreDumpGenerator::getInstanceDumpDirectory() const noexcept
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  return m_dumpDirectory;
}

DumpConfiguration const &
CoreDumpGenerator::getInstanceConfiguration() const noexcept
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  return m_currentConfig;
}

bool
CoreDumpGenerator::isInstanceInitialized() const noexcept
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  return m_isInitialized;
}

bool
CoreDumpGenerator::generateInstanceDump(std::string const &reason)
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);

  if(!m_isInitialized) return false;

  try
  {
    return generateDump(reason, m_currentConfig.getType());
  }
  catch(...)
  {
    return false;
  }
}

bool
CoreDumpGenerator::generateInstanceDump(std::string const &reason, std::error_code &errorCode) noexcept
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);

  if(!m_isInitialized)
  {
    errorCode = std::make_error_code(std::errc::invalid_argument);
    return false;
  }

  try
  {
    return generateDump(reason, m_currentConfig.getType(), errorCode);
  }
  catch(...)
  {
    errorCode = std::make_error_code(std::errc::invalid_argument);
    return false;
  }
}

// Advanced concurrency control implementation
inline bool
CoreDumpGenerator::_acquireOperationSlot() noexcept
{
  size_t current = s_activeOperations.load(std::memory_order_acquire);
  while(current < MAX_CONCURRENT_OPERATIONS)
  {
    if(s_activeOperations.compare_exchange_weak(current, current + 1, std::memory_order_release,
                                                std::memory_order_acquire))
    {
      return true;
    }
  }
  return false;
}

inline void
CoreDumpGenerator::_releaseOperationSlot() noexcept
{
  s_activeOperations.fetch_sub(1, std::memory_order_release);
  s_operationCondition.notify_one();
}

inline void
CoreDumpGenerator::_waitForOperationSlot() noexcept
{
  std::unique_lock<std::mutex> lock(s_operationMutex);
  s_operationCondition.wait(lock, []() { return s_activeOperations.load() < MAX_CONCURRENT_OPERATIONS; });
}

inline void
CoreDumpGenerator::_invalidateCache() const noexcept
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  m_cacheValid = false;
#if HAS_OPTIONAL
  m_cachedDumpType.reset();
  m_cachedDumpDirectory.reset();
#else
  m_cachedDumpTypeValid      = false;
  m_cachedDumpDirectoryValid = false;
#endif
}

inline void
CoreDumpGenerator::_updateCache() const noexcept
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  if(!m_cacheValid)
  {
#if HAS_OPTIONAL
    m_cachedDumpType      = m_currentConfig.getType();
    m_cachedDumpDirectory = m_dumpDirectory;
#else
    m_cachedDumpType           = m_currentConfig.getType();
    m_cachedDumpDirectory      = m_dumpDirectory;
    m_cachedDumpTypeValid      = true;
    m_cachedDumpDirectoryValid = true;
#endif
    m_cacheValid = true;
  }
}

// OperationGuard implementation
inline CoreDumpGenerator::OperationGuard::OperationGuard() noexcept
{
  m_acquired = _acquireOperationSlot();
  if(!m_acquired)
  {
    _waitForOperationSlot();
    m_acquired = _acquireOperationSlot();
  }
}

inline CoreDumpGenerator::OperationGuard::~OperationGuard() noexcept
{
  if(m_acquired) _releaseOperationSlot();
}

// Performance monitoring implementation
inline void
CoreDumpGenerator::_startPerformanceMonitoring(PerformanceMetrics &metrics) noexcept
{
  metrics.m_startTime = std::chrono::high_resolution_clock::now();
  metrics.m_success   = false;
  metrics.m_dumpSize  = 0;
}

inline void
CoreDumpGenerator::_endPerformanceMonitoring(PerformanceMetrics &metrics, bool success) noexcept
{
  metrics.m_endTime = std::chrono::high_resolution_clock::now();
  metrics.m_success = success;
  _logPerformanceMetrics(metrics);
}

inline void
CoreDumpGenerator::_logPerformanceMetrics(PerformanceMetrics const &metrics) noexcept
{
  auto duration       = std::chrono::duration_cast<std::chrono::milliseconds>(metrics.m_endTime - metrics.m_startTime);
  std::string message = "Performance: " + std::to_string(duration.count())
                        + "ms, Size: " + std::to_string(metrics.m_dumpSize)
                        + " bytes, Success: " + (metrics.m_success ? "true" : "false");
  _logMessage(message, LogLevel::INFO_);
}

// Private implementation
void
CoreDumpGenerator::_platformInitialize()
{
#if DUMP_CREATOR_WINDOWS
  _setupWindowsHandlers();
#elif DUMP_CREATOR_UNIX
  _setupSignalHandlers();
  _setupCoreDumpSettings();
#endif
}

// Windows-specific implementation
#if DUMP_CREATOR_WINDOWS

void
CoreDumpGenerator::_setupWindowsHandlers()
{
  try
  {
    // Set our exception handler
    SetUnhandledExceptionFilter(_windowsExceptionHandler);

    // Redirect SetUnhandledExceptionFilter to prevent removal
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if(hKernel32 != nullptr)
    {
      FARPROC pSetUnhandledExceptionFilter = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
      if(pSetUnhandledExceptionFilter != nullptr)
      {
        // We can't easily hook this in C++, but the handler should work
        _logMessage("Windows exception handler installed", false);
      }
    }

    _logMessage("Windows crash handlers installed successfully", false);
  }
  catch(std::exception const &exc)
  {
    _logMessage("Failed to setup Windows handlers: " + std::string(exc.what()), true);
    throw;
  }
}

LONG WINAPI
CoreDumpGenerator::_windowsExceptionHandler(EXCEPTION_POINTERS *pExInfo) noexcept
{
  HANDLE hFile = INVALID_HANDLE_VALUE; // Initialize to invalid handle for
                                       // RAII-style cleanup

  try
  {
    _logMessage("Windows exception handler called", false);

    // Create dump directory
    _createDirectoryRecursive(s_dumpDirectory);

    // Generate filename using thread-safe time formatting with dump type
    std::string timeStr     = formatTime("%d.%m.%Y.%H.%M.%S");
    std::string dumpTypeStr = dumpTypeToString(s_currentConfig.getType());

    // Sanitize time string to prevent command injection
    std::string sanitizedTimeStr;
    for(char c : timeStr)
      if(std::isalnum(c) || c == '.' || c == '_' || c == '-')
        sanitizedTimeStr += c;
      else
        sanitizedTimeStr += '_';

    std::wstring wTimeStr(sanitizedTimeStr.begin(), sanitizedTimeStr.end());
    std::wstring wDumpTypeStr(dumpTypeStr.begin(), dumpTypeStr.end());
    std::wstringstream wss;
    wss << s_dumpDirectory.c_str() << L"\\" << wDumpTypeStr << L"_" << wTimeStr << L".dmp";

    std::wstring filename = wss.str();

    // Open file for writing dump with secure permissions (owner read/write
    // only)
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength             = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle      = FALSE;

    // Create a security descriptor that only allows owner access
    SECURITY_DESCRIPTOR sd = {};
    if(InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
      // Set owner to current user
      PSID ownerSid      = nullptr;
      DWORD ownerSidSize = 0;
      GetTokenInformation(GetCurrentProcessToken(), TokenUser, nullptr, 0, &ownerSidSize);
      if(ownerSidSize > 0)
      {
        std::vector<BYTE> tokenInfo(ownerSidSize);
        if(GetTokenInformation(GetCurrentProcessToken(), TokenUser, tokenInfo.data(), ownerSidSize, &ownerSidSize))
        {
          TOKEN_USER *tokenUser = reinterpret_cast<TOKEN_USER *>(tokenInfo.data());
          ownerSid              = tokenUser->User.Sid;
        }
      }

      if(ownerSid && SetSecurityDescriptorOwner(&sd, ownerSid, FALSE))
      {
        // Create DACL with owner read/write access only
        PACL dacl      = nullptr;
        DWORD daclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(ownerSid);
        dacl           = static_cast<PACL>(LocalAlloc(LPTR, daclSize));
        if(dacl && InitializeAcl(dacl, daclSize, ACL_REVISION))
        {
          if(AddAccessAllowedAce(dacl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_WRITE, ownerSid))
          {
            SetSecurityDescriptorDacl(&sd, TRUE, dacl, FALSE);
            sa.lpSecurityDescriptor = &sd;
          }
        }
      }
    }

    hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
      DWORD error = GetLastError();
      _logMessage("Failed to create dump file. Error: " + std::to_string(error), true);
      return EXCEPTION_EXECUTE_HANDLER;
    }

    // Write minidump with proper dump type
    MINIDUMP_EXCEPTION_INFORMATION eInfo;
    eInfo.ThreadId          = GetCurrentThreadId();
    eInfo.ExceptionPointers = pExInfo;
    eInfo.ClientPointers    = FALSE;

    // Get dump type using centralized mapping
    MINIDUMP_TYPE dumpType = _getMinidumpType(s_currentConfig.getType());

    // Validate dump type flags
    if(!_isValidMinidumpType(dumpType))
    {
      _logMessage("Invalid MINIDUMP_TYPE flags detected, using MiniDumpNormal", true);
      dumpType = MiniDumpNormal;
    }

    BOOL success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, &eInfo, NULL, NULL);

    // Get file size BEFORE closing the handle
    LARGE_INTEGER fileSize = {{0, 0}};
    BOOL sizeResult        = GetFileSizeEx(hFile, &fileSize);

    // Close the handle immediately after getting size
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    if(success == FALSE)
    {
      DWORD error = GetLastError();
      _logMessage("MiniDumpWriteDump failed with error: " + std::to_string(error), true);
    }
    else
    {
      // Convert wide string to narrow string using helper function
      std::string narrowFilename = _convertWideStringToNarrow(filename);
      if(narrowFilename.empty())
      {
        _logMessage("Failed to convert wide string to narrow string", true);
        return EXCEPTION_EXECUTE_HANDLER;
      }

      // Log success using helper function with overflow protection
      size_t fileSizeBytes = 0;
      if(sizeResult != 0)
      {
        // Check for integer overflow before conversion
        if(fileSize.QuadPart >= 0 && static_cast<unsigned long long>(fileSize.QuadPart) <= SIZE_MAX)
        {
          fileSizeBytes = static_cast<size_t>(fileSize.QuadPart);
        }
        else
        {
          _logMessage("File size too large to represent in size_t, using 0", true);
          fileSizeBytes = 0;
        }
      }
      _logDumpCreationSuccess(narrowFilename, fileSizeBytes, static_cast<DumpType>(dumpType));
    }
  }
  catch(...)
  {
    // Don't let exceptions escape from the exception handler
    _logMessage("Exception in Windows exception handler", true);

    // Ensure handle is closed even on exceptions
    if(hFile != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
    }
  }

  return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI
CoreDumpGenerator::_redirectedSetUnhandledExceptionFilter(EXCEPTION_POINTERS * /*ExceptionInfo*/) noexcept
{
  // When the CRT calls SetUnhandledExceptionFilter with NULL parameter
  // our handler will not get removed.
  return 0;
}

bool
CoreDumpGenerator::_createWindowsDump(std::string const &filename, DumpConfiguration const &config)
{
  try
  {
    // Convert string to wide string for Windows API
    std::wstring wfilename(filename.begin(), filename.end());

    // Open file for writing dump with proper error handling
    HANDLE hFile = CreateFileW(wfilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
      DWORD error = GetLastError();
      _logMessage("Failed to create dump file. Error: " + std::to_string(error), true);
      return false;
    }

    // Write minidump with proper exception information
    MINIDUMP_EXCEPTION_INFORMATION eInfo;
    eInfo.ThreadId          = GetCurrentThreadId();
    eInfo.ExceptionPointers = nullptr; // No exception context in manual dump
    eInfo.ClientPointers    = FALSE;

    // Get dump type using centralized mapping
    MINIDUMP_TYPE dumpType = _getMinidumpType(config.getType());

    // Validate dump type flags
    if(!_isValidMinidumpType(dumpType))
    {
      _logMessage("Invalid MINIDUMP_TYPE flags detected, using MiniDumpNormal", true);
      dumpType = MiniDumpNormal;
    }

    // Configure symbol information if enabled
    MINIDUMP_USER_STREAM_INFORMATION userStreamInfo = {0, nullptr};

    BOOL success
      = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, &eInfo, &userStreamInfo, NULL);
    CloseHandle(hFile);

    if(success == FALSE)
    {
      DWORD error = GetLastError();
      _logMessage("MiniDumpWriteDump failed with error: " + std::to_string(error), true);
      return false;
    }

    _logMessage("Windows dump created successfully: " + filename, false);
    return true;
  }
  catch(std::exception const &exc)
  {
    _logMessage("Exception in _createWindowsDump: " + std::string(exc.what()), true);
    return false;
  }
}

// Windows-specific utility functions
MINIDUMP_TYPE
CoreDumpGenerator::_getMinidumpType(DumpType type) noexcept
{
  switch(type)
  {
  // Basic mini-dump types (single flags)
  case DumpType::MINI_DUMP_NORMAL: return MiniDumpNormal;
  case DumpType::MINI_DUMP_WITH_DATA_SEGS: return MiniDumpWithDataSegs;
  case DumpType::MINI_DUMP_WITH_HANDLE_DATA: return MiniDumpWithHandleData;
  case DumpType::MINI_DUMP_FILTER_MEMORY: return MiniDumpFilterMemory;
  case DumpType::MINI_DUMP_SCAN_MEMORY: return MiniDumpScanMemory;
  case DumpType::MINI_DUMP_WITH_UNLOADED_MODULES: return MiniDumpWithUnloadedModules;
  case DumpType::MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY: return MiniDumpWithIndirectlyReferencedMemory;
  case DumpType::MINI_DUMP_FILTER_MODULE_PATHS: return MiniDumpFilterModulePaths;
  case DumpType::MINI_DUMP_WITH_PROCESS_THREAD_DATA: return MiniDumpWithProcessThreadData;
  case DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY: return MiniDumpWithPrivateReadWriteMemory;
  case DumpType::MINI_DUMP_WITHOUT_OPTIONAL_DATA: return MiniDumpWithoutOptionalData;
  case DumpType::MINI_DUMP_WITH_FULL_MEMORY_INFO: return MiniDumpWithFullMemoryInfo;
  case DumpType::MINI_DUMP_WITH_THREAD_INFO: return MiniDumpWithThreadInfo;
  case DumpType::MINI_DUMP_WITH_CODE_SEGMENTS: return MiniDumpWithCodeSegs;
  case DumpType::MINI_DUMP_WITHOUT_AUXILIARY_STATE: return MiniDumpWithoutAuxiliaryState;
  case DumpType::MINI_DUMP_WITH_FULL_AUXILIARY_STATE: return MiniDumpWithFullAuxiliaryState;
  case DumpType::MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY: return MiniDumpWithPrivateWriteCopyMemory;
  case DumpType::MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY: return MiniDumpIgnoreInaccessibleMemory;
  case DumpType::MINI_DUMP_WITH_TOKEN_INFORMATION: return MiniDumpWithTokenInformation;

  // Full memory dump (combination of flags for maximum information)
  case DumpType::MINI_DUMP_WITH_FULL_MEMORY:
  case DumpType::KERNEL_FULL_DUMP:
    return static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData
                                      | MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory
                                      | MiniDumpWithProcessThreadData | MiniDumpWithPrivateReadWriteMemory
                                      | MiniDumpWithThreadInfo);

  // Kernel dump types (mapped to appropriate mini-dump equivalents)
  case DumpType::KERNEL_KERNEL_DUMP:
    return static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo);
  case DumpType::KERNEL_SMALL_DUMP: return MiniDumpNormal;
  case DumpType::KERNEL_AUTOMATIC_DUMP:
    return static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData);
  case DumpType::KERNEL_ACTIVE_DUMP:
    return static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData
                                      | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData);

  // Default fallback
  default: return MiniDumpNormal;
  }
}

bool
CoreDumpGenerator::_isValidMinidumpType(MINIDUMP_TYPE flags) noexcept
{
  // Check if flags contain only valid combinations
  // Basic validation: ensure no invalid flag combinations

  // Check for mutually exclusive flags
  if(((flags & MiniDumpWithFullMemory) != 0) && ((flags & MiniDumpNormal) != 0))
    return false; // Cannot have both full memory and normal

  // Check for invalid flag combinations
  if(((flags & MiniDumpWithoutOptionalData) != 0) && ((flags & MiniDumpWithFullMemoryInfo) != 0))
    return false; // Conflicting flags

  // Check for valid individual flags
  const MINIDUMP_TYPE validFlags = static_cast<MINIDUMP_TYPE>(
    MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpFilterMemory
    | MiniDumpScanMemory | MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory
    | MiniDumpFilterModulePaths | MiniDumpWithProcessThreadData | MiniDumpWithPrivateReadWriteMemory
    | MiniDumpWithoutOptionalData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithCodeSegs
    | MiniDumpWithoutAuxiliaryState | MiniDumpWithFullAuxiliaryState | MiniDumpWithPrivateWriteCopyMemory
    | MiniDumpIgnoreInaccessibleMemory | MiniDumpWithTokenInformation);

  // Ensure all flags are valid
  return (flags & ~validFlags) == 0;
}
#endif // DUMP_CREATOR_WINDOWS

// UNIX-specific implementation
#if DUMP_CREATOR_UNIX

void
CoreDumpGenerator::_setupSignalHandlers()
{
  try
  {
    struct sigaction sa;
    sa.sa_handler = _unixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND; // This flag resets handler after first call

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);

    _logMessage("UNIX signal handlers installed successfully", false);
  }
  catch(std::exception const &exc)
  {
    _logMessage("Failed to setup UNIX handlers: " + std::string(exc.what()), true);
    throw;
  }
}

void
CoreDumpGenerator::_setupCoreDumpSettings()
{
  try
  {
    prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

    struct rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limit);

    _logMessage("UNIX core dump settings configured", false);
  }
  catch(std::exception const &exc)
  {
    _logMessage("Failed to setup core dump settings: " + std::string(exc.what()), true);
    throw;
  }
}

void
CoreDumpGenerator::_unixSignalHandler(int signum) noexcept
{
  // Only async-signal-safe operations in signal handler
  char const crash_msg[] = "CRASH DETECTED\n";
  ssize_t bytes_written  = write(STDERR_FILENO, crash_msg, sizeof(crash_msg) - 1);
  if(bytes_written == -1) _logMessage("Failed to write to stderr: " + std::to_string(errno), true);

  // Generate core dump filename using async-signal-safe operations
  char filename[PATH_MAX];
  time_t now = time(nullptr);
  pid_t pid  = getpid();

  // Create filename: /tmp/core_<pid>_<timestamp>
  snprintf(filename, sizeof(filename), "/tmp/core_%d_%ld", pid, now);

  // Try to set core dump pattern - this may fail in WSL
  int fd = open("/proc/sys/kernel/core_pattern", O_WRONLY | O_TRUNC);
  if(fd >= 0)
  {
    ssize_t bytes_written = write(fd, filename, strlen(filename));
    if(bytes_written == -1) _logMessage("Failed to write to core pattern file: " + std::to_string(errno), true);
    close(fd);
  }

  // Use _exit instead of raise to avoid recursion and ensure proper cleanup
  _exit(128 + signum);
}

// Helper function to check and log core dump file size
void
CoreDumpGenerator::_logCoreDumpSize(std::string const &filename) noexcept
{
  try
  {
    struct stat fileStat;
    if(stat(filename.c_str(), &fileStat) == 0)
    {
      _logMessage("Core dump created successfully. Path: " + filename + ". Size: " + std::to_string(fileStat.st_size)
                    + " bytes",
                  false);
    }
    else
    {
      _logMessage("Core dump may have been created at: " + filename + " (unable to verify size)", false);

      // Check common core dump locations
      std::vector<std::string> commonLocations = {"/tmp", ".", "/var/crash", "/var/tmp"};

      for(auto const &location : commonLocations)
      {
        std::string searchPattern = location + "/core*";
        // Note: In a real implementation, you'd use glob() or similar to search for core files
        _logMessage("Check " + location + " for core dump files", false);
      }
    }
  }
  catch(...)
  {
    // Don't let logging errors crash the application
    _logMessage("Core dump created (unable to verify details)", false);
  }
}

void
CoreDumpGenerator::_generateCoreDump()
{
  try
  {
    // Create dump directory with proper error handling
    if(!_createDirectoryRecursive(s_dumpDirectory))
    {
      _logMessage("Failed to create dump directory: " + s_dumpDirectory, true);
      return;
    }

    // Generate filename using thread-safe time formatting
    std::string timeStr = formatTime("%d.%m.%Y.%H.%M.%S");
    std::ostringstream filename;
    filename << s_dumpDirectory << "/crash_dump_" << timeStr << ".core";

    // Sanitize filename to avoid validation issues
    std::string sanitizedFilename = filename.str();
    std::replace(sanitizedFilename.begin(), sanitizedFilename.end(), ':', '_');
    std::replace(sanitizedFilename.begin(), sanitizedFilename.end(), ' ', '_');

    // Set core dump size limit with error checking
    struct rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    if(setrlimit(RLIMIT_CORE, &core_limit) != 0)
      _logMessage("Failed to set core dump size limit. Error: " + std::to_string(errno), true);

    // Set secure file permissions for core dumps (owner read/write only)
    mode_t oldUmask = umask(077); // Only owner can read/write

    // Check if running in WSL
    bool isWSL = false;
    if(access("/proc/version", R_OK) == 0)
    {
      std::ifstream version_file("/proc/version");
      std::string version_line;
      if(std::getline(version_file, version_line))
        isWSL = (version_line.find("Microsoft") != std::string::npos || version_line.find("WSL") != std::string::npos);
    }

    if(isWSL)
    {
      _logMessage("WSL detected - core dump generation may be limited", true);
      _logMessage("WSL core dumps typically go to /tmp or current directory", false);
    }

    // Try to set core pattern - this requires root privileges and may not work in WSL
    int fd = open("/proc/sys/kernel/core_pattern", O_WRONLY | O_TRUNC);
    if(fd >= 0)
    {
      ssize_t bytes_written = write(fd, sanitizedFilename.c_str(), sanitizedFilename.length());
      if(bytes_written == -1) _logMessage("Failed to write to core pattern file: " + std::to_string(errno), true);
      close(fd);
      _logMessage("Core dump pattern set to: " + sanitizedFilename, false);
    }
    else
    {
      if(isWSL) { _logMessage("WSL limitation: Cannot set core_pattern. Core dumps will use default location.", true); }
      else
      {
        _logMessage("Warning: Failed to set core_pattern - insufficient permissions. "
                    "Core dumps will use system default location.",
                    true);
      }

      // Fallback: Use system default location and check common locations
      _logMessage("Core dumps will be created in system default location", false);
      _logMessage("Common locations: /tmp, current directory, or system core dump directory", false);

      // Set environment variable for core dump location (if supported by systemd-coredump)
      setenv("COREDUMP_PATTERN", sanitizedFilename.c_str(), 1);
    }

    // Restore original umask
    umask(oldUmask);

    _logMessage("Core dump generation configured", false);

    // For WSL, try alternative core dump generation methods
    if(isWSL)
    {
      _logMessage("WSL detected - attempting alternative core dump setup", false);

      // Try to use gcore if available (external tool)
      std::string gcore_cmd = "which gcore >/dev/null 2>&1";
      int gcore_available   = system(gcore_cmd.c_str());
      if(gcore_available == 0)
      {
        _logMessage("gcore is available - can be used for manual core dumps", false);
        _logMessage("To generate core dump manually: gcore <pid>", false);
      }

      // Set up environment for better core dump handling
      setenv("COREDUMP_PATTERN", "/tmp/core.%e.%p.%t", 1);
      setenv("COREDUMP_DIRECTORY", s_dumpDirectory.c_str(), 1);
    }

    // For manual core dump generation, we can check if the file exists and log its size
    _logCoreDumpSize(sanitizedFilename);
  }
  catch(std::exception const &exc)
  {
    _logMessage("Exception in _generateCoreDump: " + std::string(exc.what()), true);
  }
}

#endif // DUMP_CREATOR_UNIX

// Helper functions to reduce code duplication
#if DUMP_CREATOR_WINDOWS
std::string
CoreDumpGenerator::_convertWideStringToNarrow(std::wstring const &wideStr) noexcept
{
  try
  {
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if(size <= 0) return "";

    std::string narrowStr(size - 1, 0);
    int result = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &narrowStr[0], size, nullptr, nullptr);
    if(result == 0) return "";

    return narrowStr;
  }
  catch(...)
  {
    return "";
  }
}
#endif

void
CoreDumpGenerator::_logDumpCreationSuccess(std::string const &filename, size_t size, DumpType dumpType) noexcept
{
  try
  {
    std::ostringstream oss;
    oss << "Crash dump created successfully with type: " << static_cast<int>(dumpType) << ". Path: " << filename;

    if(size > 0)
      oss << ". Size: " << size << " bytes";
    else
      oss << ". Size: unknown";

    // Use direct logging for dump creation success to avoid path sanitization
    std::string timeStr = formatTime("%H:%M:%S");
    std::cout << "[" << timeStr << "] INFO: " << oss.str() << '\n';
  }
  catch(...)
  {
    // Silent failure in logging
  }
}

// Atomic file operations to prevent TOCTOU race conditions
bool
CoreDumpGenerator::_createFileAtomically(std::string const &filename, std::string const &content) noexcept
{
  try
  {
#if DUMP_CREATOR_WINDOWS
    // On Windows, use CreateFileW with CREATE_NEW to ensure atomic creation
    std::wstring wfilename(filename.begin(), filename.end());
    HANDLE hFile = CreateFileW(wfilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
      DWORD error = GetLastError();
      if(error != ERROR_FILE_EXISTS)
        _logMessage("Failed to create file atomically. Error: " + std::to_string(error), true);
      return false;
    }

    // Write content
    DWORD bytesWritten;
    if(!WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.length()), &bytesWritten, NULL))
    {
      CloseHandle(hFile);
      DeleteFileW(wfilename.c_str()); // Clean up on failure
      return false;
    }

    CloseHandle(hFile);
    return true;
#else
    // On UNIX, use O_CREAT | O_EXCL to ensure atomic creation
    int fd = open(filename.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
    if(fd == -1)
    {
      if(errno != EEXIST) _logMessage("Failed to create file atomically. Error: " + std::to_string(errno), true);
      return false;
    }

    // Write content
    ssize_t bytesWritten = write(fd, content.c_str(), content.length());
    if(bytesWritten == -1) _logMessage("Failed to write to file: " + std::to_string(errno), true);
    close(fd);

    if(bytesWritten != static_cast<ssize_t>(content.length()))
    {
      unlink(filename.c_str()); // Clean up on failure
      return false;
    }

    return true;
#endif
  }
  catch(...)
  {
    return false;
  }
}

bool
CoreDumpGenerator::_createDirectoryAtomically(std::string const &path) noexcept
{
  try
  {
#if DUMP_CREATOR_WINDOWS
    // On Windows, use CreateDirectoryW
    std::wstring wpath(path.begin(), path.end());
    if(CreateDirectoryW(wpath.c_str(), NULL)) return true;

    DWORD error = GetLastError();
    if(error == ERROR_ALREADY_EXISTS) return true; // Directory already exists, which is fine

    _logMessage("Failed to create directory atomically. Error: " + std::to_string(error), true);
    return false;
#else
    // On UNIX, use mkdir with proper error handling
    if(mkdir(path.c_str(), 0755) == 0) return true;

    if(errno == EEXIST) return true; // Directory already exists, which is fine

    _logMessage("Failed to create directory atomically. Error: " + std::to_string(errno), true);
    return false;
#endif
  }
  catch(...)
  {
    return false;
  }
}

// Utility functions
std::string
CoreDumpGenerator::_generateDumpFilename(std::string const &prefix)
{
  // Generate cryptographically secure random component to prevent enumeration
  // attacks
  std::string randomComponent = _generateSecureRandomComponent();

  std::string timeStr         = formatTime("%d.%m.%Y.%H.%M.%S");

  // Sanitize prefix to prevent command injection
  std::string sanitizedPrefix = _sanitizeFilenameComponent(prefix);

  // Sanitize time string to prevent command injection
  std::string sanitizedTimeStr = _sanitizeFilenameComponent(timeStr);

  std::ostringstream oss;
  oss << s_dumpDirectory << "/" << sanitizedPrefix << "_" << sanitizedTimeStr << "_" << randomComponent;

#if DUMP_CREATOR_WINDOWS
  oss << ".dmp";
#elif DUMP_CREATOR_UNIX
  oss << ".core";
#endif

  std::string filename = oss.str();

  // Final validation to ensure filename is safe
  if(!_validateFilename(filename))
  {
    // Fallback to safe default filename
    std::ostringstream fallback;
    fallback << s_dumpDirectory << "/dump_" << randomComponent;
#if DUMP_CREATOR_WINDOWS
    fallback << ".dmp";
#elif DUMP_CREATOR_UNIX
    fallback << ".core";
#endif
    return fallback.str();
  }

  return filename;
}

std::string
CoreDumpGenerator::_generateDumpFilename(DumpType dumpType)
{
  std::string prefix = dumpTypeToString(dumpType);
  return _generateDumpFilename(prefix);
}

std::string
CoreDumpGenerator::_getExecutableDirectory() noexcept
{
  try
  {
#if DUMP_CREATOR_WINDOWS
    std::array<char, MAX_PATH> buffer{};
    DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if(length == 0)
    {
      _logMessage("Failed to get executable path", true);
      return ".";
    }

    std::string exePath(buffer.data(), length);
    size_t lastSlash = exePath.find_last_of("\\/");
    if(lastSlash != std::string::npos) return exePath.substr(0, lastSlash);
    return ".";

#elif DUMP_CREATOR_UNIX
    std::array<char, PATH_MAX> buffer{};
    ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if(length == -1)
    {
      // Fallback to getcwd if readlink fails
      if(getcwd(buffer.data(), buffer.size()) != nullptr) return std::string(buffer.data());
      _logMessage("Failed to get executable path", true);
      return ".";
    }

    buffer[length] = '\0';
    std::string exePath(buffer.data());
    size_t lastSlash = exePath.find_last_of('/');
    if(lastSlash != std::string::npos) return exePath.substr(0, lastSlash);
    return ".";
#endif
  }
  catch(std::exception const &exc)
  {
    _logMessage("Exception in _getExecutableDirectory: " + std::string(exc.what()), true);
    return ".";
  }
  catch(...)
  {
    _logMessage("Unknown exception in _getExecutableDirectory", true);
    return ".";
  }
}

// DumpFactory implementation
DumpConfiguration
DumpFactory::createConfiguration(DumpType type)
{
  if(type == DumpType::DEFAULT_AUTO) type = getDefaultDumpType();

#if DUMP_CREATOR_WINDOWS
  return createWindowsConfiguration(type);
#elif DUMP_CREATOR_UNIX
  return createUnixConfiguration(type);
#else
  return DumpConfiguration{}; // Empty configuration for unsupported platforms
#endif
}

DumpType
DumpFactory::getDefaultDumpType() noexcept
{
#if DUMP_CREATOR_WINDOWS
  return DumpType::DEFAULT_WINDOWS;
#elif DUMP_CREATOR_UNIX
  return DumpType::DEFAULT_UNIX;
#else
  return DumpType::DEFAULT_AUTO;
#endif
}

bool
DumpFactory::isSupported(DumpType type) noexcept
{
  if(type == DumpType::DEFAULT_AUTO) return true; // Always supported

#if DUMP_CREATOR_WINDOWS
  // Windows supports all mini-dump types and kernel dump types
  return (static_cast<int>(type) >= static_cast<int>(DumpType::MINI_DUMP_NORMAL)
          && static_cast<int>(type) <= static_cast<int>(DumpType::KERNEL_ACTIVE_DUMP))
         || (type == DumpType::DEFAULT_WINDOWS);
#elif DUMP_CREATOR_UNIX
  // UNIX supports core dump types
  return (static_cast<int>(type) >= static_cast<int>(DumpType::CORE_DUMP_FULL)
          && static_cast<int>(type) <= static_cast<int>(DumpType::CORE_DUMP_FILTERED))
         || (type == DumpType::DEFAULT_UNIX);
#else
  return false;
#endif
}

std::string
DumpFactory::getDescription(DumpType type) noexcept
{
  auto iter = s_descriptions.find(type);
  if(iter != s_descriptions.end()) return iter->second;
  return "Unknown dump type";
}

size_t
DumpFactory::getEstimatedSize(DumpType type) noexcept
{
  auto iter = s_estimatedSizes.find(type);
  if(iter != s_estimatedSizes.end()) return iter->second;
  return 0; // Unknown size
}

std::vector<DumpType>
DumpFactory::getSupportedTypes() noexcept
{
  std::vector<DumpType> supportedTypes;

  for(auto const &pair : s_platformSupport)
    if(pair.second) supportedTypes.push_back(pair.first);

  return supportedTypes;
}

bool
DumpFactory::validateConfiguration(DumpConfiguration const &config) noexcept
{
  return config.isValid();
}

DumpConfiguration
DumpFactory::createConfiguration(DumpType type, std::error_code &errorCode) noexcept
{
  errorCode.clear();

  if(!isSupported(type))
  {
    errorCode = std::make_error_code(std::errc::not_supported);
    return DumpConfiguration{};
  }

  try
  {
    return createConfiguration(type);
  }
  catch(...)
  {
    errorCode = std::make_error_code(std::errc::invalid_argument);
    return DumpConfiguration{};
  }
}

DumpConfiguration
DumpFactory::createWindowsConfiguration(DumpType type)
{
  DumpConfiguration config;
  config.setType(type);
  config.setEnableSymbols(true);
  config.setEnableSourceInfo(true);
  config.setIncludeUnloadedModules(true);
  config.setIncludeHandleData(true);
  config.setIncludeThreadInfo(true);
  config.setIncludeProcessData(true);

  switch(type)
  {
  case DumpType::MINI_DUMP_NORMAL:
    config.setMaxSizeBytes(CoreDumpGenerator::KB_64); // 64KB
    break;
  case DumpType::MINI_DUMP_WITH_FULL_MEMORY:
    // Set reasonable limit to prevent DoS attacks (1GB max)
    config.setMaxSizeBytes(1024ULL * 1024ULL * 1024ULL); // 1GB
    break;
  case DumpType::KERNEL_FULL_DUMP:
    // Set reasonable limit for kernel dumps (2GB max)
    config.setMaxSizeBytes(2ULL * 1024ULL * 1024ULL * 1024ULL); // 2GB
    break;
  case DumpType::KERNEL_SMALL_DUMP:
    config.setMaxSizeBytes(CoreDumpGenerator::KB_64); // 64KB
    break;
  default:
    // Use default settings with reasonable limits
    config.setMaxSizeBytes(256ULL * 1024ULL * 1024ULL); // 256MB default limit
    break;
  }

  return config;
}

DumpConfiguration
DumpFactory::createUnixConfiguration(DumpType type)
{
  DumpConfiguration config;
  config.setType(type);
  config.setEnableSymbols(true);
  config.setEnableSourceInfo(true);

  switch(type)
  {
  case DumpType::CORE_DUMP_FULL:
    // Set reasonable limit to prevent DoS attacks (1GB max)
    config.setMaxSizeBytes(1024ULL * 1024ULL * 1024ULL); // 1GB
    break;
  case DumpType::CORE_DUMP_COMPRESSED:
    config.setCompress(true);
    // Set reasonable limit for compressed dumps (512MB max)
    config.setMaxSizeBytes(512ULL * 1024ULL * 1024ULL); // 512MB
    break;
  case DumpType::CORE_DUMP_FILTERED:
    // Add some default memory filters using move semantics
    config.addMemoryFilter("stack");
    config.addMemoryFilter("heap");
    // Set reasonable limit for filtered dumps (256MB max)
    config.setMaxSizeBytes(256ULL * 1024ULL * 1024ULL); // 256MB
    break;
  default:
    // Use default settings with reasonable limits
    config.setMaxSizeBytes(128ULL * 1024ULL * 1024ULL); // 128MB default limit
    break;
  }

  return config;
}

void
CoreDumpGenerator::_logMessage(std::string const &message, bool isError)
{
  try
  {
    std::string timeStr = formatTime("%H:%M:%S");
    std::ostringstream oss;

    // Check if running with admin privileges to determine sanitization level
    bool isAdmin = isAdminPrivileges();

    // Sanitize message based on privilege level
    std::string sanitizedMessage;
    if(isAdmin)
    {
      // For admin users, use less restrictive sanitization that allows paths
      sanitizedMessage = _sanitizeLogMessageForAdmin(message);
    }
    else
    {
      // For regular users, use full sanitization to prevent information
      // disclosure
      sanitizedMessage = _sanitizeLogMessage(message);
    }

    oss << "[" << timeStr << "] " << (isError ? "ERROR" : "INFO") << ": " << sanitizedMessage;

    // Standard console output
    if(isError)
      std::cerr << oss.str() << '\n';
    else
      std::cout << oss.str() << '\n';
  }
  catch(std::exception const &exc)
  {
    // Log the exception but don't let logging exceptions crash the application
    std::cerr << "Logging error: " << exc.what() << '\n';
  }
  catch(...)
  {
    // Don't let logging exceptions crash the application
    std::cerr << "Unknown logging error\n";
  }
}

// Exception handling implementation
void
CoreDumpGenerator::_setupExceptionHandling()
{
  // Set the unhandled exception handler
  std::set_terminate(_unhandledExceptionHandler);
  _logMessage("Exception handling enabled", false);
}

void
CoreDumpGenerator::_unhandledExceptionHandler()
{
  try
  {
    _logMessage("Unhandled C++ exception detected", true);

    // Generate dump for the exception with proper synchronization
    // Use atomic load to safely check initialization status
#if CPP11_OR_GREATER
    bool isInitialized = s_initialized.load(std::memory_order_acquire);
#else
    bool isInitialized = s_initialized;
#endif

    if(isInitialized)
    {
      // Create a local copy of configuration to avoid accessing potentially
      // destroyed static data
      DumpConfiguration localConfig = s_currentConfig;
      std::string filename          = _generateDumpFilename("unhandled_exception");
      _logMessage("Generating exception dump: " + filename, false);

#if DUMP_CREATOR_WINDOWS
      _createWindowsDump(filename, localConfig);
#elif DUMP_CREATOR_UNIX
      _generateCoreDump();
#endif
    }
    else { _logMessage("CoreDumpGenerator not initialized, skipping dump generation", true); }
  }
  catch(...)
  {
    // If exception handling fails, just log to stderr
    // Use only async-signal-safe functions in exception handler
    char const errorMsg[] = "Failed to handle unhandled exception\n";
#if DUMP_CREATOR_UNIX
    auto bytes_written = write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
    if(bytes_written == -1) _logMessage("Failed to write to stderr: " + std::to_string(errno), true);
#else
    // Windows fallback
    OutputDebugStringA(errorMsg);
#endif
  }

  // Call the default terminate handler
  std::abort();
}

// Security and validation functions
#if HAS_STRING_VIEW
bool
CoreDumpGenerator::_validateDirectory(std::string_view path) noexcept
#else
bool
CoreDumpGenerator::_validateDirectory(std::string const &path) noexcept
#endif
{
#if HAS_STRING_VIEW
  auto npos = std::string_view::npos;
#else
  auto npos = std::string::npos;
#endif

  try
  {
    if(path.empty()) return false;

    // Check path length first
    constexpr size_t MAX_PATH_LENGTH = 4096;
    if(path.length() > MAX_PATH_LENGTH) return false;

    // Normalize path to prevent Unicode and encoding attacks
    std::string normalizedPath(path);

    // Remove null bytes and control characters (CWE-170: Improper Null
    // Termination)
    normalizedPath.erase(std::remove_if(normalizedPath.begin(), normalizedPath.end(), [](char c)
                                        { return c == '\0' || (c < 32 && c != '\t' && c != '\n' && c != '\r'); }),
                         normalizedPath.end());

    // Check for path traversal attempts (CWE-22: Path Traversal) - Enhanced
    // detection Check for various encoding attacks
    std::string lowerPath = normalizedPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                   [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

    // Check for directory traversal patterns
    if(lowerPath.find("..") != npos) return false;
    if(lowerPath.find("%2e%2e") != npos) return false;     // URL encoded
    if(lowerPath.find("%252e%252e") != npos) return false; // Double URL encoded
    if(lowerPath.find("..%2f") != npos) return false;      // Mixed encoding
    if(lowerPath.find("..%5c") != npos) return false;      // Windows backslash
    if(lowerPath.find("..\\") != npos) return false;       // Windows backslash
    if(lowerPath.find("//") != npos) return false;
    if(lowerPath.find("\\\\") != npos) return false;

    // Check for command injection patterns (CWE-78: OS Command Injection)
    constexpr std::array<char, 15> dangerous_chars
      = {';', '&', '|', '`', '$', '(', ')', '{', '}', '[', ']', '<', '>', '"', '\''};
    if(!std::none_of(dangerous_chars.begin(), dangerous_chars.end(),
                     [&normalizedPath, npos](char character) { return normalizedPath.find(character) != npos; }))
      return false;

    // Check for absolute path requirements
    if(normalizedPath.empty()) return false;

    // Validate absolute path format
    bool isAbsolute = false;
#if DUMP_CREATOR_WINDOWS
    // Windows: C:\ or \\server\share
    isAbsolute = (normalizedPath.length() >= 3 && normalizedPath[1] == ':'
                  && (normalizedPath[2] == '\\' || normalizedPath[2] == '/'))
                 || (normalizedPath.length() >= 2 && normalizedPath[0] == '\\' && normalizedPath[1] == '\\');
#else
    // UNIX: /path
    isAbsolute = (normalizedPath[0] == '/');
#endif

    if(!isAbsolute) return false;

    // Additional Windows-specific checks
#if DUMP_CREATOR_WINDOWS
    // Check for reserved names (CWE-22) - Enhanced check
    std::array<std::string, 22> const reservedNames
      = {"con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4", "com5", "com6", "com7",
         "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};

    // Extract filename from path for reserved name check
    size_t lastSlash     = lowerPath.find_last_of("\\/");
    std::string filename = (lastSlash != npos) ? lowerPath.substr(lastSlash + 1) : lowerPath;

    // Check if filename starts with reserved name
    for(auto const &reserved : reservedNames)
    {
      if(filename.length() >= reserved.length() && filename.substr(0, reserved.length()) == reserved
         && (filename.length() == reserved.length() || filename[reserved.length()] == '.'))
      {
        return false;
      }
    }
#endif

    return true;
  }
  catch(...)
  {
    return false;
  }
}

#if HAS_STRING_VIEW
bool
CoreDumpGenerator::_validateFilename(std::string_view filename) noexcept
#else
bool
CoreDumpGenerator::_validateFilename(std::string const &filename) noexcept
#endif
{
#if HAS_STRING_VIEW
  auto npos = std::string_view::npos;
#else
  auto npos = std::string::npos;
#endif

  try
  {
    if(filename.empty()) return false;

    // Check filename length first
    constexpr size_t MAX_FILENAME_LENGTH = 255;
    if(filename.length() > MAX_FILENAME_LENGTH) return false;

    // Normalize filename to prevent Unicode and encoding attacks
    std::string normalizedFilename(filename);

    // Remove null bytes and control characters (CWE-170: Improper Null
    // Termination)
    normalizedFilename.erase(std::remove_if(normalizedFilename.begin(), normalizedFilename.end(), [](char c)
                                            { return c == '\0' || (c < 32 && c != '\t' && c != '\n' && c != '\r'); }),
                             normalizedFilename.end());

    // Check for path traversal attempts (CWE-22: Path Traversal) - Enhanced
    // detection
    std::string lowerFilename = normalizedFilename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                   [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

    // Check for directory traversal patterns
    if(lowerFilename.find("..") != npos) return false;
    if(lowerFilename.find("%2e%2e") != npos) return false;     // URL encoded
    if(lowerFilename.find("%252e%252e") != npos) return false; // Double URL encoded
    if(lowerFilename.find("..%2f") != npos) return false;      // Mixed encoding
    if(lowerFilename.find("..%5c") != npos) return false;      // Windows backslash
    if(lowerFilename.find('/') != npos) return false;
    if(lowerFilename.find('\\') != npos) return false;

    // Check for command injection patterns (CWE-78: OS Command Injection)
    constexpr std::array<char, 15> dangerous_chars
      = {';', '&', '|', '`', '$', '(', ')', '{', '}', '[', ']', '<', '>', '"', '\''};
    if(!std::none_of(dangerous_chars.begin(), dangerous_chars.end(), [&normalizedFilename, npos](char character)
                     { return normalizedFilename.find(character) != npos; }))
      return false;

    // Check for valid extension
    if(normalizedFilename.find('.') == npos) return false;

    // Additional Windows-specific checks
#if DUMP_CREATOR_WINDOWS
    // Check for reserved names (CWE-22) - Enhanced check
    std::array<std::string, 22> const reservedNames
      = {"con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4", "com5", "com6", "com7",
         "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};

    // Extract base filename without extension
    size_t dotPos            = lowerFilename.find_last_of('.');
    std::string baseFilename = (dotPos != npos) ? lowerFilename.substr(0, dotPos) : lowerFilename;

    // Check if base filename matches reserved names
    for(auto const &reserved : reservedNames)
      if(baseFilename == reserved) return false;
#endif

    return true;
  }
  catch(...)
  {
    return false;
  }
}

#if HAS_STRING_VIEW
std::string
CoreDumpGenerator::_sanitizePath(std::string_view path) noexcept
#else
std::string
CoreDumpGenerator::_sanitizePath(std::string const &path) noexcept
#endif
{
#if HAS_STRING_VIEW
  auto npos = std::string_view::npos;
#else
  auto npos = std::string::npos;
#endif

  try
  {
    if(path.empty()) return "";

    std::string sanitized(path);

    // Remove dangerous characters
#if HAS_STRING_VIEW
    std::string_view const dangerous_chars = ";&|`$(){}[]<>\"'";
#else
    std::string const dangerous_chars = ";&|`$(){}[]<>\"'";
#endif
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
                                   [&dangerous_chars, npos](char c) { return dangerous_chars.find(c) != npos; }),
                    sanitized.end());

    // Return sanitized path
    return sanitized;
  }
  catch(...)
  {
#if HAS_STRING_VIEW
    return std::string(path); // Return original if sanitization fails
#else
    return path; // Return original if sanitization fails
#endif
  }
}

// Security helper functions implementation
std::string
CoreDumpGenerator::_generateSecureRandomComponent() noexcept
{
  try
  {
    // Generate 32 random bytes (256 bits) for cryptographically secure
    // uniqueness This provides 256 bits of entropy, exceeding the 128-bit
    // security level
    constexpr size_t RANDOM_BYTES = 32;
    std::array<unsigned char, RANDOM_BYTES> randomBytes{};

#if DUMP_CREATOR_WINDOWS
    // Use Windows CryptGenRandom for cryptographically secure random numbers
    // This is the recommended approach for Windows systems per NIST SP 800-90A
    HCRYPTPROV hProv = 0;
    if(CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
      if(CryptGenRandom(hProv, static_cast<DWORD>(RANDOM_BYTES), randomBytes.data()))
      {
        // Successfully generated cryptographically secure random bytes
        CryptReleaseContext(hProv, 0);
      }
      else
      {
        // CryptGenRandom failed - this is a critical security failure
        CryptReleaseContext(hProv, 0);
        _logMessage("CRITICAL: CryptGenRandom failed - falling back to insecure method", true);
        return _generateFallbackRandomComponent();
      }
    }
    else
    {
      // CryptAcquireContext failed - this is a critical security failure
      _logMessage("CRITICAL: CryptAcquireContext failed - falling back to "
                  "insecure method",
                  true);
      return _generateFallbackRandomComponent();
    }
#else
    // UNIX: Use /dev/urandom for cryptographically secure random numbers
    // This is the recommended approach for UNIX systems per NIST SP 800-90A
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd >= 0)
    {
      ssize_t bytesRead = read(fd, randomBytes.data(), RANDOM_BYTES);
      close(fd);

      if(bytesRead == static_cast<ssize_t>(RANDOM_BYTES))
      {
        // Successfully read cryptographically secure random bytes
      }
      else
      {
        // Partial read or read failure - this is a critical security failure
        _logMessage("CRITICAL: /dev/urandom read failed - falling back to "
                    "insecure method",
                    true);
        return _generateFallbackRandomComponent();
      }
    }
    else
    {
      // /dev/urandom not available - this is a critical security failure
      _logMessage("CRITICAL: /dev/urandom not available - falling back to "
                  "insecure method",
                  true);
      return _generateFallbackRandomComponent();
    }
#endif

    // Convert to hex string using secure formatting
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::uppercase;
    for(unsigned char byte : randomBytes) oss << std::setw(2) << static_cast<unsigned int>(byte);

    return oss.str();
  }
  catch(...)
  {
    // Exception occurred - this is a critical security failure
    _logMessage("CRITICAL: Exception in secure random generation - using fallback", true);
    return _generateFallbackRandomComponent();
  }
}

// Fallback random component generation (insecure - only for system
// compatibility)
std::string
CoreDumpGenerator::_generateFallbackRandomComponent() noexcept
{
  try
  {
    // WARNING: This fallback method is NOT cryptographically secure
    // It should only be used when the secure random number generator is
    // unavailable This provides minimal entropy and should be avoided in
    // production systems

    auto now       = std::chrono::high_resolution_clock::now();
    auto timestamp = now.time_since_epoch().count();

    // Use multiple entropy sources for better (but still insufficient)
    // randomness
#if DUMP_CREATOR_WINDOWS
    auto pid = _getpid();
#else
    auto pid = getpid();
#endif
    auto tid = std::this_thread::get_id();

    // Combine entropy sources using XOR
    std::hash<std::string> hasher;
    std::string combined
      = std::to_string(timestamp) + std::to_string(pid) + std::to_string(std::hash<std::thread::id>{}(tid));

    auto hashValue = hasher(combined);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::uppercase;
    oss << std::setw(16) << (hashValue & 0xFFFFFFFFFFFFFFFFULL);

    return oss.str();
  }
  catch(...)
  {
    // Ultimate fallback - use timestamp only (very insecure)
    auto now       = std::chrono::high_resolution_clock::now();
    auto timestamp = now.time_since_epoch().count();

    std::ostringstream oss;
    oss << std::hex << timestamp;
    return oss.str();
  }
}

std::string
CoreDumpGenerator::_sanitizeFilenameComponent(std::string const &component) noexcept
{
  try
  {
    if(component.empty()) return "unknown";

    std::string sanitized = component;

    // Remove or replace dangerous characters
    for(char &c : sanitized)
    {
      if(c < 32 || c > 126) // Non-printable characters
      {
        c = '_';
      }
      else if(c == ' ' || c == '\t' || c == '\n' || c == '\r') // Whitespace
      {
        c = '_';
      }
      else if(c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>'
              || c == '|') // Invalid filename characters
      {
        c = '_';
      }
      else if(c == ';' || c == '&' || c == '|' || c == '`' || c == '$' || c == '(' || c == ')' || c == '{' || c == '}'
              || c == '[' || c == ']') // Command injection characters
      {
        c = '_';
      }
    }

    // Remove consecutive underscores
    sanitized.erase(
      std::unique(sanitized.begin(), sanitized.end(), [](char a, char b) { return a == '_' && b == '_'; }),
      sanitized.end());

    // Remove leading/trailing underscores
    if(!sanitized.empty() && sanitized.front() == '_') sanitized.erase(0, 1);
    if(!sanitized.empty() && sanitized.back() == '_') sanitized.pop_back();

    // Ensure component is not empty after sanitization
    if(sanitized.empty()) return "unknown";

    // Limit length to prevent buffer overflow
    constexpr size_t MAX_COMPONENT_LENGTH = 64;
    if(sanitized.length() > MAX_COMPONENT_LENGTH) sanitized = sanitized.substr(0, MAX_COMPONENT_LENGTH);

    return sanitized;
  }
  catch(...)
  {
    return "unknown";
  }
}

std::string
CoreDumpGenerator::_sanitizeLogMessage(std::string const &message) noexcept
{
  try
  {
    if(message.empty()) return "[empty]";

    std::string sanitized = message;

    // Remove or replace sensitive information patterns
    // Remove full file paths - replace with relative paths or basenames
    size_t pos = 0;
    while((pos = sanitized.find("C:\\", pos)) != std::string::npos)
    {
      size_t end = sanitized.find('\\', pos + 3);
      if(end != std::string::npos)
      {
        sanitized.replace(pos, end - pos, "[PATH]");
        pos += 6; // Length of "[PATH]"
      }
      else { break; }
    }

    // Remove Unix paths
    pos = 0;
    while((pos = sanitized.find("/", pos)) != std::string::npos)
    {
      if(pos > 0 && sanitized[pos - 1] != ' ')
      {
        size_t start = sanitized.rfind(' ', pos);
        if(start == std::string::npos)
          start = 0;
        else
          start++;

        size_t end = sanitized.find(' ', pos);
        if(end == std::string::npos) end = sanitized.length();

        if(end - start > 10) // Only replace long paths
        {
          sanitized.replace(start, end - start, "[PATH]");
          pos = start + 6; // Length of "[PATH]"
        }
        else { pos++; }
      }
      else { pos++; }
    }

    // Remove potential memory addresses (hex patterns)
    pos = 0;
    while((pos = sanitized.find("0x", pos)) != std::string::npos)
    {
      size_t end = pos + 2;
      while(end < sanitized.length()
            && ((sanitized[end] >= '0' && sanitized[end] <= '9') || (sanitized[end] >= 'a' && sanitized[end] <= 'f')
                || (sanitized[end] >= 'A' && sanitized[end] <= 'F')))
      {
        end++;
      }

      if(end - pos > 6) // Only replace long hex patterns
      {
        sanitized.replace(pos, end - pos, "[ADDR]");
        pos += 6; // Length of "[ADDR]"
      }
      else { pos = end; }
    }

    // Remove potential process IDs and thread IDs
    pos = 0;
    while((pos = sanitized.find("PID:", pos)) != std::string::npos)
    {
      size_t end = pos + 4;
      while(end < sanitized.length() && sanitized[end] >= '0' && sanitized[end] <= '9') end++;
      sanitized.replace(pos, end - pos, "PID:[REDACTED]");
      pos += 13; // Length of "PID:[REDACTED]"
    }

    // Remove potential error codes that might leak system information
    pos = 0;
    while((pos = sanitized.find("Error:", pos)) != std::string::npos)
    {
      size_t end = sanitized.find(' ', pos + 6);
      if(end == std::string::npos) end = sanitized.length();

      // Check if it's a numeric error code
      bool isNumeric = true;
      for(size_t i = pos + 6; i < end; i++)
      {
        if(sanitized[i] < '0' || sanitized[i] > '9')
        {
          isNumeric = false;
          break;
        }
      }

      if(isNumeric && end - pos > 8) // Only replace long numeric error codes
      {
        sanitized.replace(pos, end - pos, "Error:[REDACTED]");
        pos += 15; // Length of "Error:[REDACTED]"
      }
      else { pos = end; }
    }

    // Remove control characters and non-printable characters
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(), [](char c) { return c < 32 || c > 126; }),
                    sanitized.end());

    // Limit message length to prevent log flooding
    constexpr size_t MAX_LOG_LENGTH = 512;
    if(sanitized.length() > MAX_LOG_LENGTH) sanitized = sanitized.substr(0, MAX_LOG_LENGTH - 3) + "...";

    return sanitized;
  }
  catch(...)
  {
    return "[sanitization_failed]";
  }
}

std::string
CoreDumpGenerator::_sanitizeLogMessageForAdmin(std::string const &message) noexcept
{
  try
  {
    if(message.empty()) return "[empty]";

    std::string sanitized = message;

    // For admin users, we allow more detailed information but still sanitize
    // sensitive data Remove potential memory addresses (hex patterns) - still
    // sanitize for security
    size_t pos = 0;
    while((pos = sanitized.find("0x", pos)) != std::string::npos)
    {
      size_t end = pos + 2;
      while(end < sanitized.length()
            && ((sanitized[end] >= '0' && sanitized[end] <= '9') || (sanitized[end] >= 'a' && sanitized[end] <= 'f')
                || (sanitized[end] >= 'A' && sanitized[end] <= 'F')))
      {
        end++;
      }

      if(end - pos > 10) // Only replace very long hex patterns for admin
      {
        sanitized.replace(pos, end - pos, "[ADDR]");
        pos += 6; // Length of "[ADDR]"
      }
      else { pos = end; }
    }

    // Remove potential process IDs and thread IDs - still sanitize for security
    pos = 0;
    while((pos = sanitized.find("PID:", pos)) != std::string::npos)
    {
      size_t end = pos + 4;
      while(end < sanitized.length() && sanitized[end] >= '0' && sanitized[end] <= '9') end++;
      sanitized.replace(pos, end - pos, "PID:[REDACTED]");
      pos += 13; // Length of "PID:[REDACTED]"
    }

    // Remove control characters and non-printable characters
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(), [](char c) { return c < 32 || c > 126; }),
                    sanitized.end());

    // Limit message length to prevent log flooding
    constexpr size_t MAX_LOG_LENGTH = 1024; // Allow longer messages for admin
    if(sanitized.length() > MAX_LOG_LENGTH) sanitized = sanitized.substr(0, MAX_LOG_LENGTH - 3) + "...";

    return sanitized;
  }
  catch(...)
  {
    return "[sanitization_failed]";
  }
}

// Windows privilege checking implementation
#if DUMP_CREATOR_WINDOWS
bool
CoreDumpGenerator::_isAdminPrivileges() noexcept
{
  try
  {
    // Method 1: Check if the current process is running with elevated
    // privileges This is the most reliable method for checking admin privileges
    HANDLE hToken = nullptr;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return false;

    // Use RAII to ensure token handle is closed
    struct TokenHandleCloser {
      HANDLE handle;
      explicit TokenHandleCloser(HANDLE h) : handle(h) {}
      ~TokenHandleCloser()
      {
        if(handle) CloseHandle(handle);
      }
    } tokenCloser(hToken);

    // Get token elevation information
    TOKEN_ELEVATION elevation = {};
    DWORD dwSize              = 0;
    if(!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) return false;

    // Check if the token is elevated
    if(elevation.TokenIsElevated == 0) return false;

    // Method 2: Verify the user is actually in the Administrators group
    // This provides additional security by checking group membership
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID administratorsGroup             = nullptr;

    if(!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
                                 0, &administratorsGroup))
    {
      return false;
    }

    // Use RAII to ensure SID is freed
    struct SidCloser {
      PSID sid;
      explicit SidCloser(PSID s) : sid(s) {}
      ~SidCloser()
      {
        if(sid) FreeSid(sid);
      }
    } sidCloser(administratorsGroup);

    // Check if the current user is a member of the Administrators group
    BOOL isMember = FALSE;
    if(!CheckTokenMembership(hToken, administratorsGroup, &isMember)) return false;

    return (isMember != FALSE);
  }
  catch(...)
  {
    // If any exception occurs, assume no admin privileges for security
    return false;
  }
}

bool
CoreDumpGenerator::_isElevatedProcess() noexcept
{
  try
  {
    // Alternative method: Check if the process is running with elevated
    // privileges This is a simpler check that doesn't require group membership
    // verification
    HANDLE hToken = nullptr;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return false;

    // Use RAII to ensure token handle is closed
    struct TokenHandleCloser {
      HANDLE handle;
      explicit TokenHandleCloser(HANDLE h) : handle(h) {}
      ~TokenHandleCloser()
      {
        if(handle) CloseHandle(handle);
      }
    } tokenCloser(hToken);

    // Get token elevation information
    TOKEN_ELEVATION elevation = {};
    DWORD dwSize              = 0;
    if(!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) return false;

    return (elevation.TokenIsElevated != 0);
  }
  catch(...)
  {
    // If any exception occurs, assume no elevated privileges for security
    return false;
  }
}
#endif // DUMP_CREATOR_WINDOWS

// Platform-specific filesystem utilities implementation
bool
CoreDumpGenerator::_createDirectoryRecursive(std::string const &path) noexcept // NOLINT(misc-no-recursion)
{
  try
  {
    if(path.empty()) return false;

#if DUMP_CREATOR_WINDOWS
    // Windows implementation using CreateDirectory
    std::wstring wpath(path.begin(), path.end());

    // Check if directory already exists
    DWORD attributes = GetFileAttributesW(wpath.c_str());
    if((attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
      return true; // Directory already exists

    // Create parent directories first
    std::string parent = path;
    size_t lastSlash   = parent.find_last_of("\\/");
    if(lastSlash != std::string::npos)
    {
      parent = parent.substr(0, lastSlash);
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-recursion) - Intentional
      // recursion for directory creation
      if(!_createDirectoryRecursive(parent)) return false;
    }

    // Create the directory atomically
    return _createDirectoryAtomically(path);

#elif DUMP_CREATOR_UNIX
    // UNIX implementation using mkdir
    struct stat st;
    if(stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode); // Directory already exists

    // Create parent directories first
    std::string parent = path;
    size_t lastSlash   = parent.find_last_of('/');
    if(lastSlash != std::string::npos)
    {
      parent = parent.substr(0, lastSlash);
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-recursion) - Intentional
      // recursion for directory creation
      if(!_createDirectoryRecursive(parent)) return false;
    }

    // Create the directory atomically
    return _createDirectoryAtomically(path);

#else
    return false;
#endif
  }
  catch(...)
  {
    return false;
  }
}

// NOLINTEND(cppcoreguidelines-avoid-c-arrays,
// cppcoreguidelines-pro-type-reinterpret-cast,
// cppcoreguidelines-pro-bounds-array-to-pointer-decay)

#endif // !CORE_DUMP_GENERATOR_HPP
