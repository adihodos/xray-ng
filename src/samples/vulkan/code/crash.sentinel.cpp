#include "crash.dump.common.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstdint>

#include <optional>
#include <charconv>
#include <system_error>
#include <string>

#include <fmt/format.h>

#include <Dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void make_minidump(const xray::crash_handler::CrashData* d) {
	// prepare file name
	char name[MAX_PATH];
	{
		SYSTEMTIME t;
		GetSystemTime(&t);
		wsprintfA(
				  name, "crash_dump_%4d%02d%02d_%02d%02d%02d.dmp", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond
				  );
	}

	auto dump_file = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (dump_file == INVALID_HANDLE_VALUE) return;

	EXCEPTION_POINTERS excp = {};

	auto proc_handle   = OpenProcess(PROCESS_ALL_ACCESS, TRUE, d->proc_id);
	auto thread_handle = OpenThread(THREAD_ALL_ACCESS, TRUE, d->thread_id);

	auto mdt = (MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData |
							   MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId			= d->thread_id;
	exceptionInfo.ExceptionPointers = d->except_ptrs;
	exceptionInfo.ClientPointers	= TRUE;

	auto dumped = MiniDumpWriteDump(proc_handle, d->proc_id, dump_file, mdt, &exceptionInfo, nullptr, nullptr);

	CloseHandle(dump_file);
	return;
}

int main(int argc, char* argv[]) {
	FILE* log_file = fopen("crash.handler.log", "wt");
	if (!log_file) {
		log_file = stderr;
	}
	
	if (argc != 4) {
		fmt::println(log_file, "usage: crash-sentinel.exe crash_event_name PID memory_mapped_file_name");
		return EXIT_FAILURE;
	}

	// open shared memory
	auto map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, argv[3]);
	if (!map_file) {
		fmt::println(log_file, "Crash sentinel: could not open memory mapped file {}, error {}", argv[3], GetLastError());
		return EXIT_FAILURE;
	}
	const std::byte* mmf = reinterpret_cast<const std::byte*>(MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0, 1024));

	uint32_t process_id{};
	const auto [ptr, err_code] = std::from_chars(argv[2], argv[2] + strlen(argv[2]), process_id);
	if (err_code != std::errc{}) {
		fmt::println(log_file, "Invalid process id specified {}", argv[2]);
		return EXIT_FAILURE;
	}

	fmt::println(stdout, "Sentinel started, waiting for crash/normal exit event, PID: {}", process_id);
	HANDLE proc_handle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, process_id);
	if (proc_handle == nullptr) {
		fmt::println(log_file, "Failed to open watched process with PID {}, error {}", process_id, GetLastError());
		return EXIT_FAILURE;
	}

	// open crash event
	HANDLE all_events[2] = {
		OpenEvent(EVENT_ALL_ACCESS, false, argv[1]),
		proc_handle,
	};

	if (all_events[0] == nullptr) {
		fmt::println(log_file, "Failed to open crash event {}, error {}", argv[1], GetLastError());
		return EXIT_FAILURE;
	}
	
	// ... and wait
	const DWORD signaled_event = WaitForMultipleObjects(2, all_events, false, INFINITE);
	
	switch (signaled_event) {
	case WAIT_OBJECT_0: {
		fmt::println(log_file, "Crash detected. Creating dump...");
		const auto data = reinterpret_cast<const xray::crash_handler::CrashData*>(mmf);
		make_minidump(data);
		SetEvent(all_events[0]);
	} break;

	case (WAIT_OBJECT_0 + 1): {
		fmt::println(log_file, "Normal exit for process {}", process_id);
	} break;

	default: {
		fmt::println(log_file, "WaitForMultipleObjects failure {}, error {}", signaled_event, GetLastError());
	}
		break;
	}

	return 0;
}
