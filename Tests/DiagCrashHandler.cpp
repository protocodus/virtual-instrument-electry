// TEMPORARY DIAGNOSTIC (not for merge): print the faulting stack when the
// processor tests crash on Windows CI.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

#include <cstdio>
#include <cstring>

#pragma comment(lib, "dbghelp.lib")

extern const char* volatile electryDiagCurrentTest;

namespace
{
void printFrame (HANDLE process, DWORD64 address, int index)
{
    char modulePath[MAX_PATH] = "?";
    HMODULE module = nullptr;
    DWORD64 moduleOffset = 0;
    if (GetModuleHandleExA (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR> (address), &module))
    {
        GetModuleFileNameA (module, modulePath, MAX_PATH);
        moduleOffset = address - reinterpret_cast<DWORD64> (module);
    }
    const char* moduleName = std::strrchr (modulePath, '\\');
    moduleName = moduleName != nullptr ? moduleName + 1 : modulePath;

    alignas (SYMBOL_INFO) char buffer[sizeof (SYMBOL_INFO) + 1024] {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*> (buffer);
    symbol->SizeOfStruct = sizeof (SYMBOL_INFO);
    symbol->MaxNameLen = 1023;
    DWORD64 displacement = 0;
    const bool hasSymbol = SymFromAddr (process, address, &displacement, symbol) != FALSE;

    IMAGEHLP_LINE64 line {};
    line.SizeOfStruct = sizeof (line);
    DWORD lineDisplacement = 0;
    const bool hasLine = SymGetLineFromAddr64 (process, address, &lineDisplacement, &line) != FALSE;

    std::fprintf (stderr, "  #%02d %s+0x%llx  %s+0x%llx", index, moduleName,
                  static_cast<unsigned long long> (moduleOffset),
                  hasSymbol ? symbol->Name : "?",
                  static_cast<unsigned long long> (displacement));
    if (hasLine)
        std::fprintf (stderr, "  (%s:%lu)", line.FileName, line.LineNumber);
    std::fprintf (stderr, "\n");
}

LONG WINAPI onUnhandledException (EXCEPTION_POINTERS* info)
{
    const auto* record = info->ExceptionRecord;
    const char* const during = electryDiagCurrentTest;
    std::fprintf (stderr, "\n*** Unhandled exception 0x%08lx at %p on thread %lu during %s\n",
                  record->ExceptionCode, record->ExceptionAddress, GetCurrentThreadId(),
                  during != nullptr ? during : "(before the first test)");
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
        std::fprintf (stderr, "    %s of address 0x%llx\n",
                      record->ExceptionInformation[0] == 0 ? "read"
                          : record->ExceptionInformation[0] == 1 ? "write" : "execute",
                      static_cast<unsigned long long> (record->ExceptionInformation[1]));
    std::fflush (stderr);
    if (record->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;

    const auto process = GetCurrentProcess();
    SymSetOptions (SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize (process, nullptr, TRUE);

    // Unwind from the exception's own context so the faulting frames are
    // reported rather than the exception-dispatch frames above this filter.
    CONTEXT context = *info->ContextRecord;
    for (int index = 0; index < 80 && context.Rip != 0; ++index)
    {
        printFrame (process, context.Rip, index);
        DWORD64 imageBase = 0;
        if (auto* function = RtlLookupFunctionEntry (context.Rip, &imageBase, nullptr))
        {
            void* handlerData = nullptr;
            DWORD64 establisherFrame = 0;
            RtlVirtualUnwind (UNW_FLAG_NHANDLER, imageBase, context.Rip, function,
                              &context, &handlerData, &establisherFrame, nullptr);
        }
        else
        {
            context.Rip = *reinterpret_cast<const DWORD64*> (context.Rsp);
            context.Rsp += 8;
        }
    }
    std::fflush (stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

const auto previousFilter = SetUnhandledExceptionFilter (onUnhandledException);
} // namespace
