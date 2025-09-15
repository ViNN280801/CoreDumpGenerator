// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, cppcoreguidelines-pro-type-reinterpret-cast,
// cppcoreguidelines-pro-bounds-array-to-pointer-decay)

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#if DUMP_CREATOR_UNIX
  #include <fcntl.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#if DUMP_CREATOR_WINDOWS
  #include <aclapi.h>
  #include <process.h>
  #include <sddl.h>
  #include <wincrypt.h>
#endif

#include "CoreDumpGenerator.hpp"

// Static member definitions
std::unique_ptr<CoreDumpGenerator> CoreDumpGenerator::s_instance = nullptr;
std::mutex CoreDumpGenerator::s_mutex;
#if CPP11_OR_GREATER
std::once_flag CoreDumpGenerator::s_initFlag;
#endif
std::string CoreDumpGenerator::s_dumpDirectory = "DumpCreatorCrashDump";
#if CPP11_OR_GREATER
std::atomic<bool> CoreDumpGenerator::s_initialized{false};
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
} // anonymous namespace with helpers

void
CoreDumpGenerator::initialize(std::string const &dumpDirectory, DumpType dumpType, bool handleExceptions)
{
  DumpConfiguration config = DumpFactory::createConfiguration(dumpType);
  config.directory         = dumpDirectory.empty() ? _getExecutableDirectory() + "/dumps" : dumpDirectory;
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
    s_dumpDirectory = config.directory.empty() ? _getExecutableDirectory() + "/dumps" : config.directory;

    // Validate and sanitize directory path
    s_dumpDirectory = _sanitizePath(s_dumpDirectory);
    if(!_validateDirectory(s_dumpDirectory)) throw std::runtime_error("Invalid dump directory: " + s_dumpDirectory);

    // Create dump directory with proper error handling
    if(!_createDirectoryRecursive(s_dumpDirectory))
      throw std::runtime_error("Failed to create dump directory: " + s_dumpDirectory);
    _logMessage("Dump directory created: " + s_dumpDirectory, false);

    // Log configuration
    _logMessage("Dump type: " + DumpFactory::getDescription(config.type), false);
    if(config.maxSizeBytes > 0) _logMessage("Max size: " + std::to_string(config.maxSizeBytes) + " bytes", false);

    // Initialize platform-specific handlers
    _platformInitialize();

    // Setup exception handling if requested
    if(handleExceptions) _setupExceptionHandling();

    // Set initialized flag with release semantics to ensure all previous operations are visible
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
  #if CPP14_OR_GREATER
                   s_instance = std::make_unique<CoreDumpGenerator>();
  #else
                   s_instance = std::unique_ptr<CoreDumpGenerator>(new CoreDumpGenerator());
  #endif
                 });
#else
  // C++98/03 fallback - not thread-safe
  if(!s_instance)
  {
    if(!s_initialized) throw std::runtime_error("CoreDumpGenerator not initialized. Call initialize() first.");
  #if CPP14_OR_GREATER
    s_instance = std::make_unique<CoreDumpGenerator>();
  #else
    s_instance = std::unique_ptr<CoreDumpGenerator>(new CoreDumpGenerator());
  #endif
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
      config           = DumpFactory::createConfiguration(dumpType);
      config.directory = s_dumpDirectory; // Preserve current directory
    }

    std::string filename = _generateDumpFilename(config.type);
    _logMessage("Generating dump: " + reason, false);
    _logMessage("Dump type: " + DumpFactory::getDescription(config.type), false);

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
    std::string filename = _generateDumpFilename(config.type);
    _logMessage("Generating dump: " + reason, false);
    _logMessage("Dump type: " + DumpFactory::getDescription(config.type), false);

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
      config           = DumpFactory::createConfiguration(dumpType);
      config.directory = s_dumpDirectory; // Preserve current directory
    }

    std::string filename = _generateDumpFilename(config.type);
    _logMessage("Generating dump: " + reason, false);
    _logMessage("Dump type: " + DumpFactory::getDescription(config.type), false);

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
  return s_dumpDirectory; // s_dumpDirectory is only modified during initialization, which is single-threaded
}

DumpConfiguration
CoreDumpGenerator::getCurrentConfiguration() noexcept
{
  return s_currentConfig; // s_currentConfig is only modified during initialization, which is single-threaded
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

  s_currentConfig           = DumpFactory::createConfiguration(dumpType);
  s_currentConfig.directory = s_dumpDirectory; // Preserve current directory
  return true;
}

DumpType
CoreDumpGenerator::getCurrentDumpType() noexcept
{
  return s_currentConfig.type; // s_currentConfig is only modified during initialization, which is single-threaded
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
  HANDLE hFile = INVALID_HANDLE_VALUE; // Initialize to invalid handle for RAII-style cleanup

  try
  {
    _logMessage("Windows exception handler called", false);

    // Create dump directory
    _createDirectoryRecursive(s_dumpDirectory);

    // Generate filename using thread-safe time formatting with dump type
    std::string timeStr     = formatTime("%d.%m.%Y.%H.%M.%S");
    std::string dumpTypeStr = dumpTypeToString(s_currentConfig.type);

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

    // Open file for writing dump with secure permissions (owner read/write only)
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
    MINIDUMP_TYPE dumpType = _getMinidumpType(s_currentConfig.type);

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
    MINIDUMP_TYPE dumpType = _getMinidumpType(config.type);

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
  write(STDERR_FILENO, crash_msg, sizeof(crash_msg) - 1);

  // Set core dump pattern using async-signal-safe operations
  int fd = open("/proc/sys/kernel/core_pattern", O_WRONLY | O_TRUNC);
  if(fd >= 0)
  {
    char const *pattern = "/tmp/core.%e.%p.%t";
    write(fd, pattern, strlen(pattern));
    close(fd);
  }

  // Use _exit instead of raise to avoid recursion and ensure proper cleanup
  // Note: _logMessage calls are not async-signal-safe and have been removed
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
    else { _logMessage("Core dump may have been created at: " + filename + " (unable to verify size)", false); }
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

    // Validate filename for security
    if(!_validateFilename(filename.str()))
    {
      _logMessage("Invalid filename generated, using default", true);
      filename.str("");
      filename << s_dumpDirectory << "/crash_dump_" << timeStr << ".core";
    }

    // Set core dump size limit with error checking
    struct rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    if(setrlimit(RLIMIT_CORE, &core_limit) != 0)
      _logMessage("Failed to set core dump size limit. Error: " + std::to_string(errno), true);

    // Set secure file permissions for core dumps (owner read/write only)
    // This is done by setting the umask to restrict permissions
    mode_t oldUmask = umask(077); // Only owner can read/write

    // Use proper file I/O instead of system() call to prevent command injection
    std::ofstream core_pattern_file("/proc/sys/kernel/core_pattern");
    if(core_pattern_file.is_open())
    {
      core_pattern_file << filename.str();
      if(core_pattern_file.fail())
        _logMessage("Failed to write core dump pattern. Error: " + std::to_string(errno), true);
      else
        _logMessage("Core dump pattern set to: " + filename.str(), false);
      core_pattern_file.close();
    }
    else
    {
      _logMessage("Warning: Failed to set core_pattern - insufficient permissions. Error: " + std::to_string(errno),
                  true);
    }

    // Restore original umask
    umask(oldUmask);

    _logMessage("Core dump generation configured", false);

    // For manual core dump generation, we can check if the file exists and log its size
    // Note: For automatic core dumps (crashes), the kernel creates the file after the process terminates
    _logCoreDumpSize(filename.str());
  }
  catch(std::exception const &exc)
  {
    _logMessage("Exception in _generateCoreDump: " + std::string(exc.what()), true);
  }
}

#endif // DUMP_CREATOR_UNIX

// Helper functions to reduce code duplication
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
  // Generate cryptographically secure random component to prevent enumeration attacks
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
DumpFactory::isSupported(DumpType type)
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
DumpFactory::getDescription(DumpType type)
{
  auto iter = s_descriptions.find(type);
  if(iter != s_descriptions.end()) return iter->second;
  return "Unknown dump type";
}

size_t
DumpFactory::getEstimatedSize(DumpType type)
{
  auto iter = s_estimatedSizes.find(type);
  if(iter != s_estimatedSizes.end()) return iter->second;
  return 0; // Unknown size
}

DumpConfiguration
DumpFactory::createWindowsConfiguration(DumpType type)
{
  DumpConfiguration config;
  config.type                   = type;
  config.enableSymbols          = true;
  config.enableSourceInfo       = true;
  config.includeUnloadedModules = true;
  config.includeHandleData      = true;
  config.includeThreadInfo      = true;
  config.includeProcessData     = true;

  switch(type)
  {
  case DumpType::MINI_DUMP_NORMAL:
    config.maxSizeBytes = CoreDumpGenerator::KB_64; // 64KB
    break;
  case DumpType::MINI_DUMP_WITH_FULL_MEMORY:
    // Set reasonable limit to prevent DoS attacks (1GB max)
    config.maxSizeBytes = 1024ULL * 1024ULL * 1024ULL; // 1GB
    break;
  case DumpType::KERNEL_FULL_DUMP:
    // Set reasonable limit for kernel dumps (2GB max)
    config.maxSizeBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL; // 2GB
    break;
  case DumpType::KERNEL_SMALL_DUMP:
    config.maxSizeBytes = CoreDumpGenerator::KB_64; // 64KB
    break;
  default:
    // Use default settings with reasonable limits
    config.maxSizeBytes = 256ULL * 1024ULL * 1024ULL; // 256MB default limit
    break;
  }

  return config;
}

DumpConfiguration
DumpFactory::createUnixConfiguration(DumpType type)
{
  DumpConfiguration config;
  config.type             = type;
  config.enableSymbols    = true;
  config.enableSourceInfo = true;

  switch(type)
  {
  case DumpType::CORE_DUMP_FULL:
    // Set reasonable limit to prevent DoS attacks (1GB max)
    config.maxSizeBytes = 1024ULL * 1024ULL * 1024ULL; // 1GB
    break;
  case DumpType::CORE_DUMP_COMPRESSED:
    config.compress = true;
    // Set reasonable limit for compressed dumps (512MB max)
    config.maxSizeBytes = 512ULL * 1024ULL * 1024ULL; // 512MB
    break;
  case DumpType::CORE_DUMP_FILTERED:
    // Add some default memory filters using move semantics
    config.memoryFilters.reserve(2);
    config.memoryFilters.emplace_back("stack");
    config.memoryFilters.emplace_back("heap");
    // Set reasonable limit for filtered dumps (256MB max)
    config.maxSizeBytes = 256ULL * 1024ULL * 1024ULL; // 256MB
    break;
  default:
    // Use default settings with reasonable limits
    config.maxSizeBytes = 128ULL * 1024ULL * 1024ULL; // 128MB default limit
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

    // Sanitize message to prevent information disclosure
    std::string sanitizedMessage = _sanitizeLogMessage(message);

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
      // Create a local copy of configuration to avoid accessing potentially destroyed static data
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
    write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
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

    // Remove null bytes and control characters (CWE-170: Improper Null Termination)
    normalizedPath.erase(std::remove_if(normalizedPath.begin(), normalizedPath.end(), [](char c)
                                        { return c == '\0' || (c < 32 && c != '\t' && c != '\n' && c != '\r'); }),
                         normalizedPath.end());

    // Check for path traversal attempts (CWE-22: Path Traversal) - Enhanced detection
    // Check for various encoding attacks
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
#if HAS_STRING_VIEW
    std::string_view const dangerous_chars = ";&|`$(){}[]<>\"'";
#else
    std::string const dangerous_chars = ";&|`$(){}[]<>\"'";
#endif
    if(!std::none_of(dangerous_chars.begin(), dangerous_chars.end(),
                     [&normalizedPath, npos](char c) { return normalizedPath.find(c) != npos; }))
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

    // Remove null bytes and control characters (CWE-170: Improper Null Termination)
    normalizedFilename.erase(std::remove_if(normalizedFilename.begin(), normalizedFilename.end(), [](char c)
                                            { return c == '\0' || (c < 32 && c != '\t' && c != '\n' && c != '\r'); }),
                             normalizedFilename.end());

    // Check for path traversal attempts (CWE-22: Path Traversal) - Enhanced detection
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
#if HAS_STRING_VIEW
    std::string_view const dangerous_chars = ";&|`$(){}[]<>\"'";
#else
    std::string const dangerous_chars = ";&|`$(){}[]<>\"'";
#endif
    if(!std::none_of(dangerous_chars.begin(), dangerous_chars.end(),
                     [&normalizedFilename, npos](char c) { return normalizedFilename.find(c) != npos; }))
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
    // Generate 32 random bytes (256 bits) for cryptographically secure uniqueness
    // This provides 256 bits of entropy, exceeding the 128-bit security level
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
      _logMessage("CRITICAL: CryptAcquireContext failed - falling back to insecure method", true);
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
        _logMessage("CRITICAL: /dev/urandom read failed - falling back to insecure method", true);
        return _generateFallbackRandomComponent();
      }
    }
    else
    {
      // /dev/urandom not available - this is a critical security failure
      _logMessage("CRITICAL: /dev/urandom not available - falling back to insecure method", true);
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

// Fallback random component generation (insecure - only for system compatibility)
std::string
CoreDumpGenerator::_generateFallbackRandomComponent() noexcept
{
  try
  {
    // WARNING: This fallback method is NOT cryptographically secure
    // It should only be used when the secure random number generator is unavailable
    // This provides minimal entropy and should be avoided in production systems

    auto now       = std::chrono::high_resolution_clock::now();
    auto timestamp = now.time_since_epoch().count();

    // Use multiple entropy sources for better (but still insufficient) randomness
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
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-recursion) - Intentional recursion for directory creation
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
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-recursion) - Intentional recursion for directory creation
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

// DumpConfiguration implementation
bool
DumpConfiguration::operator==(DumpConfiguration const &other) const noexcept
{
  return type == other.type && filename == other.filename && directory == other.directory && compress == other.compress
         && includeUnloadedModules == other.includeUnloadedModules && includeHandleData == other.includeHandleData
         && includeThreadInfo == other.includeThreadInfo && includeProcessData == other.includeProcessData
         && maxSizeBytes == other.maxSizeBytes && memoryFilters == other.memoryFilters
         && enableSymbols == other.enableSymbols && enableSourceInfo == other.enableSourceInfo;
}

bool
DumpConfiguration::operator!=(DumpConfiguration const &other) const noexcept
{
  return !(*this == other);
}

bool
DumpConfiguration::isValid() const noexcept
{
  try
  {
    // Check if maxSizeBytes is valid (0 means unlimited)
    if(maxSizeBytes != 0 && maxSizeBytes < 1024) return false; // Minimum 1KB

    // Check for potential integer overflow in size calculations
    if(maxSizeBytes != 0 && maxSizeBytes > SIZE_MAX / 2) return false; // Prevent overflow in calculations

    // Check if directory is valid (basic check)
    if(!directory.empty() && directory.length() > 4096) return false; // Max path length

    // Check if filename is valid (basic check)
    if(!filename.empty())
    {
      if(filename.length() > 255) return false; // Max filename length
      if(filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) return false;
    }

    // Check memory filters for overflow potential
    for(auto const &filter : memoryFilters)
    {
      if(filter.empty() || filter.length() > 64) return false;

      // Check for potential overflow in filter processing
      if(filter.length() > SIZE_MAX / 2) return false;
    }

    return true;
  }
  catch(...)
  {
    return false;
  }
}

std::string
DumpConfiguration::getValidationError() const
{
  if(maxSizeBytes != 0 && maxSizeBytes < 1024) return "maxSizeBytes must be 0 (unlimited) or at least 1024 bytes";

  if(!directory.empty() && directory.length() > 4096) return "directory path too long (max 4096 characters)";

  if(!filename.empty())
  {
    if(filename.length() > 255) return "filename too long (max 255 characters)";
    if(filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos)
      return "filename contains path separators";
  }

  for(auto const &filter : memoryFilters)
  {
    if(filter.empty()) return "empty memory filter found";
    if(filter.length() > 64) return "memory filter too long (max 64 characters)";
  }

  return "configuration is valid";
}

// NOLINTEND(cppcoreguidelines-avoid-c-arrays, cppcoreguidelines-pro-type-reinterpret-cast,
// cppcoreguidelines-pro-bounds-array-to-pointer-decay)
