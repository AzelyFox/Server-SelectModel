#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <list>
#include <iostream>
#include <iomanip>
#include <crtdbg.h>

#include <Windows.h>

#define __FILENAME__ (wcsrchr(__FILEW__, '/') ? wcsrchr(__FILEW__, '/') + 1 : __FILEW__)

//#define LOG_DISABLE

using namespace std;

namespace azely {

	class SimpleLogger
	{
	public:
		enum LogConstants
		{
			TAG_SIZE_BYTES_MAX = 32,
			MESSAGE_SIZE_BYTES_MAX = 256,
			TIME_SIZE_BYTES_MAX = 64
		};

		enum LogSaveMode
		{
			MODE_NONE = 0,
			MODE_DEBUG = 1,
			MODE_CONSOLE = 2,
			MODE_FILE = 4
		};

		enum LogLevel
		{
			LEVEL_DEBUG = 1,
			LEVEL_INFO = 2,
			LEVEL_WARNING = 3,
			LEVEL_ERROR = 4,
			LEVEL_CRITICAL = 9,
			LEVEL_NONE = 10
		};


		enum LogColor {
			BLACK,  	//0
			DARKBLUE,	//1
			DARKGreen,	//2
			DARKSKYBLUE,    //3
			DARKRED,  	//4
			DARKPURPLE,	//5
			DARKYELLOW,	//6
			GRAY,		//7
			DARKGRAY,	//8
			BLUE,		//9
			GREEN,		//10
			SKYBLUE,	//11
			RED,		//12
			PURPLE,		//13
			YELLOW,		//14
			WHITE		//15
		};

		struct Log
		{
			/**
			 * \brief LogSaveLevel
			 */
			INT32 logLevel;
			/**
			 * \brief WHO MADE THIS LOG
			 */
			PWSTR logTag;
			/**
			 * \brief LOG MESSAGE
			 */
			PWSTR logMessage;
			/**
			 * \brief LOG MESSAGE SUB
			 */
			PWSTR logMessageSub;
			/**
			 * \brief LOG CREATED TIMESTAMP
			 */
			PWSTR logTime;
		};

		static SimpleLogger *GetInstance()
		{
			return &instance_;
		}
		/**
		 * \brief CLEAR SAVED LOGS
		 * \param level if value is 0, every log will be cleared
		 */
		VOID ClearLogs(INT32 level = 0);

		/**
		 * \brief SAVE DEBUG LOG
		 * \param tag Log.logTag
		 * \param message Log.logMessage
		 * \param windowsErrorCode if value is not 0, Windows error message will be added to Log.logMessageSub
		 */
		VOID SaveLogDebug(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode = 0);

		/**
		 * \brief SAVE INFO LOG
		 * \param tag Log.logTag
		 * \param message Log.logMessage
		 * \param windowsErrorCode if value is not 0, Windows error message will be added to Log.logMessageSub
		 */
		VOID SaveLogInfo(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode = 0);

		/**
		 * \brief SAVE WARNING LOG
		 * \param tag Log.logTag
		 * \param message Log.logMessage
		 * \param windowsErrorCode if value is not 0, Windows error message will be added to Log.logMessageSub
		 */
		VOID SaveLogWarning(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode = 0);

		/**
		 * \brief SAVE ERROR LOG
		 * \param tag Log.logTag
		 * \param message Log.logMessage
		 * \param windowsErrorCode if value is not 0, Windows error message will be added to Log.logMessageSub
		 */
		VOID SaveLogError(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode = 0);

		/**
		 * \brief SAVE CRITICAL LOG
		 * \param tag Log.logTag
		 * \param message Log.logMessage
		 * \param windowsErrorCode if value is not 0, Windows error message will be added to Log.logMessageSub
		 */
		VOID SaveLogCritical(PCWSTR tag, PCWSTR message, INT32 windowsErrorCode = 0);

		/**
		 * \brief GET LOG WITH LEVEL FILTER
		 * \param outLogList [out] filtered DoubleLinkedList
		 * \param level if value is 0, result will be not filtered
		 * \return data has successfully inserted to outLogList
		 */
		BOOL GetLogs(list<Log *> *outLogList, INT32 level = 0);

		/**
		 * \brief SET LOG SAVE SETTING
		 * \param saveMode LogSaveMode
		 * \param saveMinLevel LogSaveLevel
		 * \param consoleDefaultColor consoleDefaultColor
		 */
		VOID SetLogSetting(LogLevel pauseMinLevel, int saveMode, LogLevel saveMinLevel, LogColor consoleDefaultColor = WHITE);
	private:
		SimpleLogger();
		~SimpleLogger();

		/**
		 * \brief [INTERNAL METHOD] CREATE LOG
		 * \param outLog [out] created Log
		 * \param tag Log.tag
		 * \param message Log.message
		 * \param messageSub Log.messageSub
		 * \param level Log.level
		 * \return is log successfully created
		 */
		BOOL CreateLog(Log **outLog, PCWSTR tag, PCWSTR message, PCWSTR messageSub, INT32 level) const;

		/**
		 * \brief [INTERNAL METHOD] SAVE SINGLE LOG WITH CUSTOM LEVEL
		 * \param tag Log.logTag
		 * \param message Log.logMessage
		 * \param level Log.logLevel
		 * \param windowsErrorCode if value is not 0, Windows error message will be added to Log.logMessageSub
		 */
		VOID SaveLog(PCWSTR tag, PCWSTR message, INT32 level = LEVEL_DEBUG, INT32 windowsErrorCode = 0);

		static SimpleLogger instance_;
		PWSTR fileName_;
		list<Log *> *logs_;

		LogLevel logPauseMinLevel_;
		int logSaveMode_;
		LogLevel logSaveMinLevel_;
		LogColor logConsoleDefaultColor_;
		HANDLE logConsoleHandle_;

		CRITICAL_SECTION logCriticalSection;
	};

}