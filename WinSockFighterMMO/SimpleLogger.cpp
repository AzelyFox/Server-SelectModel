#include "stdafx.h"
#include "SimpleLogger.h"

namespace azely {

	SimpleLogger SimpleLogger::instance_;

	SimpleLogger::SimpleLogger()
	{
		logs_ = new list<Log *>;
		logSaveMode_ = MODE_NONE;
		logConsoleHandle_ = GetStdHandle(STD_OUTPUT_HANDLE);
		logConsoleDefaultColor_ = WHITE;
		logPauseMinLevel_ = LEVEL_ERROR;
		logSaveMinLevel_ = LEVEL_DEBUG;
		fileName_ = new WCHAR[64];
		ZeroMemory(fileName_, 64 * 2);
		time_t now = time(NULL);
		tm ltm;
		localtime_s(&ltm, &now);
		wcsftime(fileName_, 64, L"log_%Y-%m-%d_%H_%M_%S.txt", &ltm);
		setlocale(LC_ALL, "");
		InitializeCriticalSection(&logCriticalSection);
	}

	SimpleLogger::~SimpleLogger()
	{
		delete logs_;
	}

	VOID SimpleLogger::SaveLog(PCWSTR tag, PCWSTR message, INT32 level, INT32 windowsErrorCode)
	{
#ifdef LOG_DISABLE
		return;
#endif
		if (message == nullptr) return;

		Log *log;
		PVOID messageFromWindows = nullptr;

		if (windowsErrorCode != 0)
		{
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, windowsErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (PWSTR)(&messageFromWindows), 0, nullptr
			);
		}

		/*
		 * CRITICAL SECTION ON
		 */
		EnterCriticalSection(&logCriticalSection);

		BOOL logCreationResult = CreateLog(&log, tag, message, static_cast<PWSTR>(messageFromWindows), level);
		if (!logCreationResult || log == nullptr) return;
		logs_->push_back(log);

		if (messageFromWindows != nullptr) LocalFree(messageFromWindows);

		LeaveCriticalSection(&logCriticalSection);
		/*
		 * CRITICAL SECTION OFF
		 */

		int logLevelColor = WHITE;
		WCHAR logLevelStr[10];
		ZeroMemory(logLevelStr, 10 * 2);
		if (level == LEVEL_DEBUG)
		{
			PCWSTR levelStr = L"DEBUG";
			logLevelColor = GRAY;
			memcpy_s(logLevelStr, 10 * 2, levelStr, wcslen(levelStr) * 2);
		}
		if (level == LEVEL_INFO)
		{
			PCWSTR levelStr = L"INFO";
			logLevelColor = SKYBLUE;
			memcpy_s(logLevelStr, 10 * 2, levelStr, wcslen(levelStr) * 2);
		}
		if (level == LEVEL_WARNING)
		{
			PCWSTR levelStr = L"WARN";
			logLevelColor = YELLOW;
			memcpy_s(logLevelStr, 10 * 2, levelStr, wcslen(levelStr) * 2);
		}
		if (level == LEVEL_ERROR)
		{
			PCWSTR levelStr = L"ERROR";
			logLevelColor = RED;
			memcpy_s(logLevelStr, 10 * 2, levelStr, wcslen(levelStr) * 2);
		}
		if (level == LEVEL_CRITICAL)
		{
			PCWSTR levelStr = L"CRIT";
			logLevelColor = DARKRED;
			memcpy_s(logLevelStr, 10 * 2, levelStr, wcslen(levelStr) * 2);
		}

		if (logSaveMode_ == MODE_NONE) return;
		if (logSaveMode_ & MODE_DEBUG)
		{
			_RPTW1(0, L"[%s\t%s]\t%s :: %s (%d)\n", logLevelStr, log->logTime, log->logTag, log->logMessage, windowsErrorCode);
		}
		if (logSaveMode_ & MODE_CONSOLE)
		{
			if (logConsoleHandle_ == INVALID_HANDLE_VALUE) return;
			SetConsoleTextAttribute(logConsoleHandle_, logLevelColor);
			wcout << L"[" << logLevelStr << L"\t" << log->logTime << L"]\t";
			SetConsoleTextAttribute(logConsoleHandle_, logConsoleDefaultColor_);
			wcout << left << std::setw(TAG_SIZE_BYTES_MAX / 2) << log->logTag << L" :: ";
			wcout << log->logMessage << endl;
			if (windowsErrorCode != 0 && log->logMessageSub != nullptr) {
				wcout << L"\t(" << windowsErrorCode << L")\t" << log->logMessageSub << endl;
			}
		}
		if (logSaveMode_ & MODE_FILE)
		{
			FILE *file;
			errno_t err = _wfopen_s(&file, fileName_, L"a");
			if (err != 0 || file == NULL) {
				return;
			}
			fwprintf_s(file, L"[%s\t%s]\t%s :: %s\n", logLevelStr, log->logTime, log->logTag, log->logMessage);
			if (windowsErrorCode != 0 && log->logMessageSub != nullptr) {
				fwprintf_s(file, L"%s\n", log->logMessageSub);
			}
			fclose(file);
		}

		if (logPauseMinLevel_ <= level)
		{
			system("pause");
		}
	}

	VOID SimpleLogger::SaveLogDebug(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode)
	{
		if (logSaveMinLevel_ > LEVEL_DEBUG) return;
		return SaveLog(tag, message, LEVEL_DEBUG, windowsErrorCode);
	}

	VOID SimpleLogger::SaveLogInfo(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode)
	{
		if (logSaveMinLevel_ > LEVEL_INFO) return;
		return SaveLog(tag, message, LEVEL_INFO, windowsErrorCode);
	}

	VOID SimpleLogger::SaveLogWarning(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode)
	{
		if (logSaveMinLevel_ > LEVEL_WARNING) return;
		return SaveLog(tag, message, LEVEL_WARNING, windowsErrorCode);
	}

	VOID SimpleLogger::SaveLogError(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode)
	{
		if (logSaveMinLevel_ > LEVEL_ERROR) return;
		return SaveLog(tag, message, LEVEL_ERROR, windowsErrorCode);
	}

	VOID SimpleLogger::SaveLogCritical(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode)
	{
		if (logSaveMinLevel_ > LEVEL_CRITICAL) return;
		return SaveLog(tag, message, LEVEL_CRITICAL, windowsErrorCode);
	}

	VOID SimpleLogger::ClearLogs(INT32 level)
	{
		/*
		 * CRITICAL SECTION ON
		 */
		EnterCriticalSection(&logCriticalSection);

		if (level == 0)
		{
			logs_->clear();
			return;
		}

		list<Log *>::iterator iterator = logs_->begin();
		while (iterator != logs_->end())
		{
			if ((*iterator)->logLevel == level)
			{
				iterator = logs_->erase(iterator);
			}
			else
			{
				++iterator;
			}
		}

		LeaveCriticalSection(&logCriticalSection);
		/*
		 * CRITICAL SECTION OFF
		 */
	}

	BOOL SimpleLogger::GetLogs(list<Log *> *outLogList, INT32 level)
	{
		if (outLogList == nullptr) return false;

		/*
		 * CRITICAL SECTION ON
		 */
		EnterCriticalSection(&logCriticalSection);

		list<Log *>::iterator iterator = logs_->begin();
		while (iterator != logs_->end())
		{
			if (level == 0 || (*iterator)->logLevel == level)
			{
				outLogList->push_back(*iterator);
			}
			++iterator;
		}

		LeaveCriticalSection(&logCriticalSection);
		/*
		 * CRITICAL SECTION OFF
		 */

		return true;
	}

	VOID SimpleLogger::SetLogSetting(LogLevel pauseMinLevel, int saveMode, LogLevel saveMinLevel, LogColor consoleDefaultColor)
	{
		logPauseMinLevel_ = pauseMinLevel;
		logSaveMode_ = saveMode;
		logSaveMinLevel_ = saveMinLevel;
		logConsoleDefaultColor_ = consoleDefaultColor;
	}

	BOOL SimpleLogger::CreateLog(Log **outLog, PCWSTR tag, PCWSTR message, PCWSTR messageSub, INT32 level) const
	{
		if (tag == nullptr || message == nullptr) return false;

		SIZE_T tagLength = min(wcslen(tag) + 1, TAG_SIZE_BYTES_MAX / sizeof(WCHAR));
		SIZE_T messageLength = min(wcslen(message) + 1, MESSAGE_SIZE_BYTES_MAX / sizeof(WCHAR));
		SIZE_T messageSubLength = 0;
		SIZE_T timeLength = TIME_SIZE_BYTES_MAX / sizeof(WCHAR);
		if (messageSub != nullptr) messageSubLength = min(wcslen(messageSub) + 1, MESSAGE_SIZE_BYTES_MAX / sizeof(WCHAR));

		*outLog = new Log;

		(*outLog)->logLevel = level;

		(*outLog)->logTag = new WCHAR[tagLength];
		//wcsncpy_s((*outLog)->logTag, tagLength, tag, tagLength);
		memcpy_s((*outLog)->logTag, tagLength * sizeof(WCHAR), tag, tagLength * sizeof(WCHAR));
		memset(((*outLog)->logTag + tagLength - 1), 0, 2);

		(*outLog)->logMessage = new WCHAR[messageLength];
		//wcsncpy_s((*outLog)->logMessage, messageLength, message, messageLength);
		memcpy_s((*outLog)->logMessage, messageLength * sizeof(WCHAR), message, messageLength * sizeof(WCHAR));
		memset(((*outLog)->logMessage + messageLength - 1), 0, 2);

		(*outLog)->logMessageSub = nullptr;
		if (messageSub != nullptr) {
			(*outLog)->logMessageSub = new WCHAR[messageSubLength];
			//wcsncpy_s((*outLog)->logMessageSub, messageSubLength, messageSub, messageSubLength);
			memcpy_s((*outLog)->logMessageSub, messageSubLength * sizeof(WCHAR), messageSub, messageSubLength * sizeof(WCHAR));
			memset(((*outLog)->logMessageSub + messageSubLength - 1), 0, 2);
		}

		(*outLog)->logTime = new WCHAR[timeLength];
		memset((*outLog)->logTime, 0, timeLength * sizeof(WCHAR));

		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);
		wsprintfW((*outLog)->logTime,
			L"%4d-%02d-%02d %02u:%02u:%02u.%03u",
			systemTime.wYear,
			systemTime.wMonth,
			systemTime.wDay,
			systemTime.wHour,
			systemTime.wMinute,
			systemTime.wSecond,
			systemTime.wMilliseconds);

		return true;
	}

}