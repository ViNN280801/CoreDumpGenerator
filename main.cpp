// NOLINTBEGIN

#include <iostream>
#include <thread>
#include <vector>

#include "CoreDumpGenerator.hpp"

namespace
{
  void
  baz()
  {
    double value1            = 3.14159;
    double value2            = 2.71828;
    double *null_pointer     = nullptr;

    std::vector<double> data = {1.1, 2.2, 3.3, 4.4, 5.5};

    std::string error_msg    = "Critical error occurred!";

    struct Point {
      double x, y, z;
      Point(double x, double y, double z) : x(x), y(y), z(z) {}
    };
    Point point(10.5, 20.3, 30.7);

    (void)null_pointer;
    (void)value1;
    (void)value2;
    (void)data;
    (void)error_msg;
    (void)point;

    throw std::runtime_error("This is an unhandled exception that should trigger CoreDumpGenerator!");
  }

  void
  bar()
  {
    class Foo
    {
    private:
      std::string m_name;
      std::vector<int> m_data;
      double m_value{};
      int m_counter{};

    public:
      Foo() : m_name("Foo"), m_data({1, 2, 3, 4, 5}), m_value(42.0), m_counter(100)
      {
        std::cout << "Foo constructor\n";
        std::cout << "m_name: " << m_name << "\n";
        std::cout << "m_data: " << m_data.size() << "\n";
        std::cout << "m_value: " << m_value << "\n";
        std::cout << "m_counter: " << m_counter << "\n";
      }
    };

    Foo foo;

    int local_var              = 999;
    std::string local_str      = "Hello from bar()";
    std::vector<int> local_vec = {10, 20, 30, 40, 50};

    (void)local_var;
    (void)local_str;
    (void)local_vec;

    baz();
  }

  void
  foo()
  {
    bar();
  }
}

int
main()
{
  std::cout << "=== CoreDumpGenerator Crash Demonstration ===\n";
  std::cout << "Initializing CoreDumpGenerator...\n";

  CoreDumpGenerator::initialize("", DumpType::MINI_DUMP_WITH_FULL_MEMORY, true);

  if(!CoreDumpGenerator::isInitialized())
  {
    std::cerr << "Failed to initialize CoreDumpGenerator!\n";
    return 1;
  }

  std::cout << "CoreDumpGenerator initialized successfully!\n";
  std::cout << "Dump directory: " << CoreDumpGenerator::getDumpDirectory() << "\n";
  std::cout << "Current dump type: " << DumpFactory::getDescription(CoreDumpGenerator::getCurrentDumpType()) << "\n";

  std::cout << "\n=== Available Dump Types ===\n";
  std::cout << "=== Basic Mini Dumps (64KB) ===\n";
  std::cout << "1. MINI_DUMP_NORMAL\n";
  std::cout << "2. MINI_DUMP_WITHOUT_OPTIONAL_DATA\n";
  std::cout << "3. MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY\n";
  std::cout << "4. MINI_DUMP_FILTER_MEMORY\n";
  std::cout << "5. MINI_DUMP_SCAN_MEMORY\n";
  std::cout << "6. MINI_DUMP_FILTER_MODULE_PATHS\n";
  std::cout << "7. MINI_DUMP_WITHOUT_AUXILIARY_STATE\n";

  std::cout << "\n=== Medium Mini Dumps (128-512KB) ===\n";
  std::cout << "8. MINI_DUMP_WITH_DATA_SEGS\n";
  std::cout << "9. MINI_DUMP_WITH_HANDLE_DATA\n";
  std::cout << "10. MINI_DUMP_WITH_UNLOADED_MODULES\n";
  std::cout << "11. MINI_DUMP_WITH_THREAD_INFO\n";
  std::cout << "12. MINI_DUMP_WITH_CODE_SEGMENTS\n";
  std::cout << "13. MINI_DUMP_WITH_TOKEN_INFORMATION\n";

  std::cout << "\n=== Large Mini Dumps (1MB+) ===\n";
  std::cout << "14. MINI_DUMP_WITH_PROCESS_THREAD_DATA\n";
  std::cout << "15. MINI_DUMP_WITH_FULL_AUXILIARY_STATE\n";

  std::cout << "\n=== Full Memory Dumps (Variable Size) ===\n";
  std::cout << "16. MINI_DUMP_WITH_FULL_MEMORY\n";
  std::cout << "17. MINI_DUMP_WITH_FULL_MEMORY_INFO\n";
  std::cout << "18. MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY\n";
  std::cout << "19. MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY\n";
  std::cout << "20. MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY\n";

  std::cout << "\n=== Kernel Dumps ===\n";
  std::cout << "21. KERNEL_FULL_DUMP\n";
  std::cout << "22. KERNEL_KERNEL_DUMP\n";
  std::cout << "23. KERNEL_SMALL_DUMP\n";
  std::cout << "24. KERNEL_AUTOMATIC_DUMP\n";
  std::cout << "25. KERNEL_ACTIVE_DUMP\n";

  std::cout << "0. Exit\n";
  std::cout << "\nSelect option: ";

  int choice = 0;
  std::cin >> choice;

  if(choice == 0)
  {
    std::cout << "Exiting...\n";
    return 0;
  }

  if(choice >= 1 && choice <= 25)
  {
    std::vector<DumpType> dumpTypes = {
      DumpType::MINI_DUMP_NORMAL,                            // 1
      DumpType::MINI_DUMP_WITHOUT_OPTIONAL_DATA,             // 2
      DumpType::MINI_DUMP_IGNORE_INACCESSIBLE_MEMORY,        // 3
      DumpType::MINI_DUMP_FILTER_MEMORY,                     // 4
      DumpType::MINI_DUMP_SCAN_MEMORY,                       // 5
      DumpType::MINI_DUMP_FILTER_MODULE_PATHS,               // 6
      DumpType::MINI_DUMP_WITHOUT_AUXILIARY_STATE,           // 7
      DumpType::MINI_DUMP_WITH_DATA_SEGS,                    // 8
      DumpType::MINI_DUMP_WITH_HANDLE_DATA,                  // 9
      DumpType::MINI_DUMP_WITH_UNLOADED_MODULES,             // 10
      DumpType::MINI_DUMP_WITH_THREAD_INFO,                  // 11
      DumpType::MINI_DUMP_WITH_CODE_SEGMENTS,                // 12
      DumpType::MINI_DUMP_WITH_TOKEN_INFORMATION,            // 13
      DumpType::MINI_DUMP_WITH_PROCESS_THREAD_DATA,          // 14
      DumpType::MINI_DUMP_WITH_FULL_AUXILIARY_STATE,         // 15
      DumpType::MINI_DUMP_WITH_FULL_MEMORY,                  // 16
      DumpType::MINI_DUMP_WITH_FULL_MEMORY_INFO,             // 17
      DumpType::MINI_DUMP_WITH_INDIRECTLY_REFERENCED_MEMORY, // 18
      DumpType::MINI_DUMP_WITH_PRIVATE_READ_WRITE_MEMORY,    // 19
      DumpType::MINI_DUMP_WITH_PRIVATE_WRITE_COPY_MEMORY,    // 20
      DumpType::KERNEL_FULL_DUMP,                            // 21
      DumpType::KERNEL_KERNEL_DUMP,                          // 22
      DumpType::KERNEL_SMALL_DUMP,                           // 23
      DumpType::KERNEL_AUTOMATIC_DUMP,                       // 24
      DumpType::KERNEL_ACTIVE_DUMP                           // 25
    };

    CoreDumpGenerator::setDumpType(dumpTypes[choice - 1]);
    std::cout << "Selected dump type: " << DumpFactory::getDescription(CoreDumpGenerator::getCurrentDumpType()) << "\n";
    std::cout << "Estimated size: " << DumpFactory::getEstimatedSize(CoreDumpGenerator::getCurrentDumpType())
              << " bytes\n";
    std::cout << "Triggering crash in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Trigger crash
    // char *null_pointer = nullptr;
    // *null_pointer      = CRASH_VALUE; // This will crash

    foo();
  }

  std::cout << "This should not be printed!\n";
  return 0;
}

// NOLINTEND
