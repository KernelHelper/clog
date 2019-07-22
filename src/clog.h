
#ifndef __CLOG_H__
#define __CLOG_H__

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
// Disable all warning: _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#include <io.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define __off_t		_off_t
#define F_OK		0
#define O_APPEND	_O_APPEND
#define O_CREAT		_O_CREAT
#define O_WRONLY	_O_WRONLY
#define O_CLOEXEC	_O_NOINHERIT
#define DEFFILEMODE 0666
//#define DEFFILEMODE _S_IREAD | _S_IWRITE
#define strcasecmp	stricmp
#define sys_get_tid() ::GetCurrentThreadId()

// For windows implementation of "gettimeofday"
#if defined(_WIN32) || defined(WIN32)
#include <time.h>
#include <windows.h> //I've ommited this line.

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone
{
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};
__inline static 
int gettimeofday(struct timeval* tv, struct timezone* tz)
{
	FILETIME f = { 0 };
	static char _tz = 0x00;
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
	unsigned long long u = 0Ui64;
#else
	unsigned long long u = 0ULL;
#endif
	if (tv)
	{
#ifdef _WIN32_WCE 
		SYSTEMTIME s = { 0 };
		::GetSystemTime(&s);
		::SystemTimeToFileTime(&s, &f);
#else 
		::GetSystemTimeAsFileTime(&f);
#endif 
		u |= f.dwHighDateTime;
		u <<= 32;
		u |= f.dwLowDateTime;
		//convert into microseconds
		u /= 10;  
		//converting file time to unix epoch
		u -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(u / 1000000UL);
		tv->tv_usec = (long)(u % 1000000UL);
	}
	if (tz)
	{
		if (_tz == 0x00)
		{
			_tzset();
			_tz=0x01;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}
	return 0;
}

#endif
#else
#include <stdarg.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>
#define sys_get_tid() syscall(SYS_gettid)
#endif

#pragma pack (1)
struct log_set {
	int level;
	__off_t limit;
	size_t arg_size;
	struct log_arg {
#define data_size 102400
		int fd;
		char fname[256];
		int num;
		int idx;
		char data[data_size];
	}*arg_list;
};
#pragma pack ()

static struct log_set* p_log_set = 0;

enum log_level_type
{
	LOG_NONE = 0,
	LOG_FATAL,
	LOG_ERROR,
	LOG_WARN,
	LOG_INFO,
	LOG_DEBUG,
	LOG_TRACE,
	LOG_VERBOSE,
	LOG_MAX,
};
static const char* p_log_level_name[LOG_MAX] = {
	"NONE",
	"FATAL",
	"ERROR",
	"WARN",
	"INFO",
	"DEBUG",
	"TRACE",
	"VERBOSE"
};
static const char* p_log_level_color[LOG_MAX] = {
	"\033[30m",
	"\033[31m",
	"\033[32m",
	"\033[33m",
	"\033[34m",
	"\033[35m",
	"\033[36m",
	"\033[37m"
};

__inline static
int exit_log()
{
	if (p_log_set)
	{
		if (p_log_set->arg_list)
		{
			for (size_t i = 0; i < p_log_set->arg_size; i++)
			{
				if (p_log_set->arg_list[i].fd != (-1))
				{
					close(p_log_set->arg_list[i].fd);
				}
				p_log_set->arg_list[i].fd = (-1);
			}
			free(p_log_set->arg_list);
			p_log_set->arg_list = (0);
		}
		free(p_log_set);
		p_log_set = (0);
	}
	return 0;
}
__inline static
struct log_set* init_log(int level, int limit, const struct log_set::log_arg* p_la, size_t arg_size)
{
	if (p_log_set == 0)
	{
		p_log_set = (struct log_set*)malloc(sizeof(struct log_set));
		if (p_log_set == 0)
		{
			return (0);
		}
	}
	p_log_set->level = level;
	p_log_set->limit = limit;
	p_log_set->arg_size = arg_size;
	p_log_set->arg_list = (struct log_set::log_arg*)malloc(p_log_set->arg_size * sizeof(struct log_set::log_arg));
	if (p_log_set->arg_list == 0)
	{
		exit_log();
		return (0);
	}
	memset(p_log_set->arg_list, 0, p_log_set->arg_size * sizeof(struct log_set::log_arg));
	for (size_t i = 0; i < p_log_set->arg_size; i++)
	{
		memcpy(&p_log_set->arg_list[i], &p_la[i], sizeof(struct log_set::log_arg));
		{
			char fname[256] = { 0 };
			for (int n = 0; n < p_log_set->arg_list[i].num; n++)
			{
				memset(fname, 0, sizeof(fname));
				snprintf(fname, sizeof(fname) / sizeof(*fname), "%s.%d", p_log_set->arg_list[i].fname, n);
				if (access(fname, F_OK) != 0)
				{
					p_log_set->arg_list[i].idx = n;
					break;
				}
			}
		}
		p_log_set->arg_list[i].fd = open(p_log_set->arg_list[i].fname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
		if (p_log_set->arg_list[i].fd == (-1))
		{
			exit_log();
			return (0);
		}
	}
	return (p_log_set);
}
__inline static
__off_t get_log_limit()
{
	return p_log_set->limit;
}
__inline static
int get_log_top_level()
{
	return p_log_set->level;
}
__inline static
const char* get_log_level_name(int level)
{
	return p_log_level_name[level];
}
__inline static
const char* get_log_level_color(int level)
{
	return p_log_level_color[level];
}
__inline static
const char* get_log_level_color_end()
{
	return "\033[0m";
}
__inline static
struct log_set::log_arg* get_log_arg(const char* fname)
{
	for (size_t i = 0; i < p_log_set->arg_size; i++)
	{
		if (strcasecmp(p_log_set->arg_list[i].fname, fname) == 0)
		{
			return &p_log_set->arg_list[i];
		}
	}
	return (0);
}
__inline static
int check_log(struct log_set::log_arg* pla, struct tm* tm_now, __off_t limit)
{
	struct stat st = { 0 };
	if (fstat(pla->fd, &st) == (-1))
	{
		switch (errno)
		{
		case ENOENT:
		{
			pla->fd = open(pla->fname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
			if (pla->fd == (-1))
			{
				printf("open log file %s failed. msg: (errno=%d)%s\n", pla->fname, errno, strerror(errno));
				return (-1);
			}
		}
		break;
		default:
		{
			printf("stat log file %s failed. msg: (errno=%d)%s\n", pla->fname, errno, strerror(errno));
			return (-1);
		}
		break;
		}
	}
	else
	{
		struct tm* tm_mod = 
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
		localtime(&st.st_mtime);
#else
		localtime(&st.st_mtim.tv_sec);
#endif
		if ((tm_now->tm_mday != tm_mod->tm_mday || tm_now->tm_mon != tm_mod->tm_mon
			|| tm_now->tm_year != tm_mod->tm_year) || (st.st_size + data_size >= limit))
		{
			//已非当日或即将超过指定大小限制
			char new_fname[256] = { 0 };
			snprintf(new_fname, sizeof(new_fname) / sizeof(*new_fname), "%s.%d", pla->fname, pla->idx);
			close(pla->fd);
			if (rename(pla->fname, new_fname) != 0)
			{
				pla->fd = (-1);
				printf("rename logfile %s -> %s failed msg: (errno=%d)%s\n", pla->fname, new_fname, errno, strerror(errno));
				return (-1);
			}
			pla->idx = (pla->idx + 1) % pla->num;
			int fd = open(pla->fname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
			if (fd == (-1))
			{
				pla->fd = (-1);
				printf("open log file %s failed. msg: (errno=%d)%s\n", pla->fname, errno, strerror(errno));
				return (-1);
			}
			dup2(fd, pla->fd);
			if (fd != pla->fd)
			{
				close(fd);
			}
		}
	}
	return (0);
}

__inline static 
int log(const char* fname, int level, const char* fmt, ...)
{
	va_list arg;
	size_t arg_len = 0;
	time_t tt_secs = 0;
	struct tm *tm_now = 0;
	struct timeval tv = { 0 };
	struct timezone tz = { 0 };
	struct log_set::log_arg* pla = 0;

	// 若level大于指定的level,则不打印日志信息
	if (level > get_log_top_level())
	{
		return (-1);
	}

	if ((pla = get_log_arg(fname)) == 0)
	{
		return (-1);
	}
	gettimeofday(&tv, &tz);
	check_log(pla, (tm_now = localtime(&(tt_secs = tv.tv_sec))), get_log_limit());
	
	va_start(arg, fmt);
	arg_len += snprintf(pla->data, data_size,
		"%s[%04d-%02d-%02d_%02d:%02d:%02d.%06d:%ld:%C]",
		get_log_level_color(level),
		tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
		tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv.tv_usec,
		sys_get_tid(), *get_log_level_name(level));
	arg_len += vsnprintf(pla->data + arg_len, data_size, fmt, arg);
	arg_len += snprintf(pla->data + arg_len, data_size, "%s\n", get_log_level_color_end());
	printf(pla->data);
	write(pla->fd, pla->data, arg_len);
	va_end(arg);

	return (0);
}

//#define LOG(NAME,LEVEL,FMT...)		log(#NAME".log",LEVEL,##FMT)
#define LOG(NAME,LEVEL,FMT,...)	log(#NAME".log",LEVEL,FMT,##__VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <vector>
__inline static 
int log_test_main()
{
	printf("hello from mylog!\n");
	char test[10240] = { 0 };
	int count = snprintf(test, 10240, "%s\n", "1234");
	printf("%s\n", test);
	count += snprintf(test + count, 10240, "%s\n", "1234");
	printf("%s\n", test);
	count += snprintf(test + count, 10240, "%s\n", "1234");
	printf("%s\n", test);

	const struct log_set::log_arg las[] = {
		{-1, "main.log", 30,0,""},
		{-1, "main1.log", 30,0,""},
	};
	init_log(LOG_VERBOSE, 2 * 1024 * 1024, las, sizeof(las) / sizeof(*las));
	for (size_t i = 0; i < 100; i++)
	{
		LOG(main, LOG_INFO, "%s(%d)", "I am test!!!!!!", i);
		LOG(main, LOG_INFO, "%s(%d)", "I am test!!!!!!", i);
		log("main.log", LOG_ERROR, "%s(%d)", "I am test!!!!!!", i);
		log("main.log", LOG_FATAL, "%s(%d)", "I am test!!!!!!", i);
		log("main.log", LOG_WARN, "%s(%d)", "I am test!!!!!!", i);
		log("main.log", LOG_TRACE, "%s(%d)", "I am test!!!!!!", i);
		log("main.log", LOG_DEBUG, "%s(%d)", "I am test!!!!!!", i);
		log("main1.log", LOG_INFO, "%s(%d)", "I am test!!!!!!", i);
	}
	std::vector<std::thread> tv;
	try
	{
		tv.push_back(std::move(std::thread([]() {

			for (size_t i = 0; i < 100; i++)
			{
				log("main.log", LOG_INFO, "%s(%d)", "I am test!!!!!!", i);
				LOG(main, LOG_WARN, "%s(%d)", "I am test!!!!!!", i);
				LOG(main, LOG_TRACE, "%s(%d)", "I am test!!!!!!", i);
				std::this_thread::sleep_for(std::chrono::microseconds(1000));
			}
			})));
		getchar();
		for (auto & it : tv)
		{
			if (it.joinable())
			{
				it.join();
			}
		}
	}
	catch (const std::exception& e)
	{
		printf("%s\n", e.what());
	}

	exit_log();

	return 0;
}

#endif // __CLOG_H__