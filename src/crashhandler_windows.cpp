#if !(defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
#error "crashhandler_windows.cpp is used on a non-Windows platform"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fmt/format.h>
#include <map>
#include <csignal>
#include "crashhandler.hpp"
#include "CLogger.hpp"


namespace
{
	LPTOP_LEVEL_EXCEPTION_FILTER PreviousUnhandledExceptionHandler = nullptr;
	void *VectorExceptionHandler = nullptr;

#define __CH_WIN32_STATUS_PAIR(stat) {STATUS_##stat, #stat}

	const std::map<crashhandler::Signal, std::string> KnownExceptionsMap{
		__CH_WIN32_STATUS_PAIR(ACCESS_VIOLATION),
		__CH_WIN32_STATUS_PAIR(DATATYPE_MISALIGNMENT),
		__CH_WIN32_STATUS_PAIR(BREAKPOINT),
		__CH_WIN32_STATUS_PAIR(SINGLE_STEP),
		__CH_WIN32_STATUS_PAIR(ARRAY_BOUNDS_EXCEEDED),
		__CH_WIN32_STATUS_PAIR(FLOAT_DENORMAL_OPERAND),
		__CH_WIN32_STATUS_PAIR(FLOAT_DIVIDE_BY_ZERO),
		__CH_WIN32_STATUS_PAIR(FLOAT_INEXACT_RESULT),
		__CH_WIN32_STATUS_PAIR(FLOAT_INVALID_OPERATION),
		__CH_WIN32_STATUS_PAIR(FLOAT_OVERFLOW),
		__CH_WIN32_STATUS_PAIR(FLOAT_STACK_CHECK),
		__CH_WIN32_STATUS_PAIR(FLOAT_UNDERFLOW),
		__CH_WIN32_STATUS_PAIR(INTEGER_DIVIDE_BY_ZERO),
		__CH_WIN32_STATUS_PAIR(INTEGER_OVERFLOW),
		__CH_WIN32_STATUS_PAIR(PRIVILEGED_INSTRUCTION),
		__CH_WIN32_STATUS_PAIR(IN_PAGE_ERROR),
		__CH_WIN32_STATUS_PAIR(ILLEGAL_INSTRUCTION),
		__CH_WIN32_STATUS_PAIR(NONCONTINUABLE_EXCEPTION),
		__CH_WIN32_STATUS_PAIR(STACK_OVERFLOW),
		__CH_WIN32_STATUS_PAIR(INVALID_DISPOSITION)
	};


	void ReverseToOriginalFatalHandling()
	{
		SetUnhandledExceptionFilter(PreviousUnhandledExceptionHandler);
		RemoveVectoredExceptionHandler(VectorExceptionHandler);
	}

	LONG WINAPI GeneralExceptionHandler(LPEXCEPTION_POINTERS info, const char *handler)
	{
		const crashhandler::Signal fatal_signal = info->ExceptionRecord->ExceptionCode;
		auto it = KnownExceptionsMap.find(fatal_signal);
		const std::string signal_str = (it != KnownExceptionsMap.end()) ? it->second : "<unknown>";
		const std::string err_msg = fmt::format(
			"exception {:#X} ({:s}) from {:s} catched; shutting log-core down",
			fatal_signal, signal_str, handler ? handler : "invalid");

		CLogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
			"log-core", LogLevel::ERROR, err_msg, 0, "", "")));
		CLogManager::Get()->Destroy();

		// FATAL Exception: It doesn't necessarily stop here we pass on continue search
		// if no one else will catch that then it's goodbye anyhow.
		// The RISK here is if someone is cathing this and returning "EXCEPTION_EXECUTE_HANDLER"
		// but does not shutdown then the software will be running with g3log shutdown.
		// .... However... this must be seen as a bug from standard handling of fatal exceptions
		// https://msdn.microsoft.com/en-us/library/6wxdsc38.aspx
		return EXCEPTION_CONTINUE_SEARCH;
	}

	LONG WINAPI UnhandledExceptionHandler(LPEXCEPTION_POINTERS info)
	{
		ReverseToOriginalFatalHandling();
		return GeneralExceptionHandler(info, "Unhandled Exception Handler");
	}

	LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS p)
	{
		const crashhandler::Signal exc_code = p->ExceptionRecord->ExceptionCode;
		if (KnownExceptionsMap.find(exc_code) == KnownExceptionsMap.end())
			return EXCEPTION_CONTINUE_SEARCH; //not an exception we care for

		ReverseToOriginalFatalHandling();
		return GeneralExceptionHandler(p, "Vectored Exception Handler");
	}

	BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
	{
		if (dwCtrlType == CTRL_CLOSE_EVENT)
		{
			CLogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
				"log-core", LogLevel::INFO, "received Windows console close event; shutting log-core down", 0, "", "")));
			CLogManager::Get()->Destroy();
		}
		return FALSE; //let other handlers have a chance to clean stuff up
	}
}


namespace crashhandler
{
	void Install()
	{
		PreviousUnhandledExceptionHandler = SetUnhandledExceptionFilter(UnhandledExceptionHandler);
		VectorExceptionHandler = AddVectoredExceptionHandler(0, VectoredExceptionHandler);
		SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	}
}
