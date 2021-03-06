#pragma once
#ifndef INC_SAMPLOG_PLUGINLOGGER_H
#define INC_SAMPLOG_PLUGINLOGGER_H


#include "Logger.h"
#include "DebugInfo.h"


//NOTE: Passing "-fvisibility=hidden" as a compiler option to GCC is advised!
#if defined _WIN32 || defined __CYGWIN__
# ifdef __GNUC__
#  define DLL_PUBLIC __attribute__ ((dllimport))
# else
#  define DLL_PUBLIC __declspec(dllimport)
# endif
#else
# if __GNUC__ >= 4
#  define DLL_PUBLIC __attribute__ ((visibility ("default")))
# else
#  define DLL_PUBLIC
# endif
#endif


typedef struct tagAMX AMX;


extern "C" DLL_PUBLIC bool samplog_LogNativeCall(
	const char *module, AMX * const amx,
	const char *name, const char *params_format);


#ifdef __cplusplus

#include <string>

namespace samplog
{
	inline bool LogNativeCall(const char *module, AMX * const amx, 
		const char *name, const char *params_format)
	{
		return samplog_LogNativeCall(module, amx, name, params_format);
	}

	class CPluginLogger : public CLogger
	{
	public:
		CPluginLogger(std::string pluginname) :
			CLogger("plugins/" + pluginname)
		{ }
		~CPluginLogger() = default;
		CPluginLogger(CPluginLogger const &rhs) = delete;
		CPluginLogger& operator=(CPluginLogger const &rhs) = delete;

		CPluginLogger(CPluginLogger &&other) = delete;
		CPluginLogger& operator=(CPluginLogger &&other) = delete;

	public:
		inline bool Log(LogLevel level, const char *msg)
		{
			return CLogger::Log(level, msg, 0, "", "");
		}
		bool Log(AMX * const amx, const LogLevel level, const char *msg)
		{
			if (!CLogger::IsLogLevel(level))
				return false;

			const char
				*func = "",
				*file = "";
			int line = 0;

			if (samplog::GetLastAmxLine(amx, line))
			{
				samplog::GetLastAmxFile(amx, file);
				samplog::GetLastAmxFunction(amx, func);
			}

			return CLogger::Log(level, msg, line, file, func);
		}
		inline bool LogNativeCall(AMX * const amx, const char *name, const char *params_format)
		{
			return samplog::LogNativeCall(m_Module.c_str(), amx, name, params_format);
		}
	};

	typedef CPluginLogger PluginLogger_t;

}

#endif /* __cplusplus */

#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_PLUGINLOGGER_H */
