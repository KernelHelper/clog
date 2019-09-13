
#ifndef __CLOG_H__
#define __CLOG_H__

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
///////////////////////////////////////////////////////////////////////////
// Disable all warning: _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#include <io.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <direct.h>

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
#define PATH_SEPARATOR			"\\"
#define PATH_SEPARATOR_CHAR		'\\'
#define make_dir(x)				mkdir(x)
#define LOG_STDOUT_FILENO		fileno(stdout)

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

static CRITICAL_SECTION log_locker = { 0 };
#define log_init_lock() InitializeCriticalSection(&log_locker)
#define log_lock()		EnterCriticalSection(&log_locker)
#define log_unlock()	LeaveCriticalSection(&log_locker)
#define log_exit_lock() DeleteCriticalSection(&log_locker)
#else
///////////////////////////////////////////////////////////////////////////
#include <stdarg.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>
#define sys_get_tid() syscall(SYS_gettid)
#define PATH_SEPARATOR			"/"
#define PATH_SEPARATOR_CHAR		'/'
#define make_dir(x)				mkdir(x, DEFFILEMODE)
#define LOG_STDOUT_FILENO		STDOUT_FILENO

static pthread_mutex_t log_locker;
#define log_init_lock() pthread_mutex_init(&log_locker,0)
#define log_lock()		pthread_mutex_lock(&log_locker)
#define log_unlock()	pthread_mutex_unlock(&log_locker)
#define log_exit_lock() pthread_mutex_destroy(&log_locker)

#endif

#define MAX_LOG_PATH			4096
#define DATA_SIZE				102400

#define DAY_SECONDS				86400
#define DEFAULT_LOG_ROTATETIME	DAY_SECONDS
#define DEFAULT_LOG_LIMIT_SIZE	10*1024*1024
#define DEFAULT_LOG_FMT			""
#define DEFAULT_LOG_EXT			".log"
#define DEFAULT_LOG_PATH		"." PATH_SEPARATOR
#define DEFAULT_LOG_CONS		0xFF
#define NEED_LOG_CONS(X)		(X!=0x00)
#define LOG_FILE_FORMAT			"%Y-%m-%d %H:%M:%S"
#define LOG_LR					'\r'
#define LOG_CF					'\n'

#pragma pack (1)
struct log_arg {
	int fd;
	long num;
	long idx;
	time_t ftime;
	char fname[64];
	char lname[MAX_LOG_PATH];
	long size;
	char* data;
};
struct log_set {
	long args;
	int level;
	char ext[8];
	char fmt[64];
	__off_t limit;
	long rotatetime;
	unsigned char cons;
	char path[MAX_LOG_PATH];
	struct log_arg* argl;
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
#define IS_VALID_LOG_LEVEL_TYPE(X) (X > LOG_NONE && X < LOG_MAX)
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
#define NONE_COLOR	"\033[00m"
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
time_t get_log_rotatetime()
{
	return p_log_set->rotatetime;
}
__inline static
const char* get_log_path()
{
	return p_log_set->path;
}
__inline static
const char* get_log_ext()
{
	return p_log_set->ext;
}
__inline static
const char* get_log_fmt()
{
	return p_log_set->fmt;
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
unsigned char get_log_cons()
{
	return p_log_set->cons;
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
	return NONE_COLOR;
}
__inline static
struct log_arg* get_log_arg(const char* fname)
{
	for (long i = 0; i < p_log_set->args; i++)
	{
		if (strcasecmp(p_log_set->argl[i].fname, fname) == 0)
		{
			return &p_log_set->argl[i];
		}
	}
	return (0);
}
__inline static
int exit_log()
{
	if (p_log_set)
	{
		if (p_log_set->argl)
		{
			for (long i = 0; i < p_log_set->args; i++)
			{
				if (p_log_set->argl[i].fd != (-1))
				{
					close(p_log_set->argl[i].fd);
					p_log_set->argl[i].fd = (-1);
				}
				if (p_log_set->argl[i].data != 0)
				{
					free(p_log_set->argl[i].data);
					p_log_set->argl[i].data = 0;
				}
			}
			free(p_log_set->argl);
			p_log_set->argl = (0);
		}
		free(p_log_set);
		p_log_set = (0);
	}
	
	log_exit_lock();

	return 0;
}
__inline static
struct log_set* init_log(const char* path, const char* fmt, const char* ext, unsigned char cons,
	long rotatetime, int level, long limit, const struct log_arg* pargl, unsigned long args)
{
	char* p = 0;
	int nCount = 0;
	time_t log_secs = 0;
	struct tm* tm_log = 0;
	struct timeval tv = { 0 };
	struct timezone tz = { 0 };
	char lname[MAX_LOG_PATH] = { 0 };
	char lpath[MAX_LOG_PATH] = { 0 };

	log_init_lock();

	if (p_log_set == 0)
	{
		p_log_set = (struct log_set*)malloc(sizeof(struct log_set));
		if (p_log_set == 0)
		{
			exit_log();
			return (0);
		}
	}

	p = (char*)path;
	while (((p = strchr((char*)path + (p - path), PATH_SEPARATOR_CHAR)) != 0))
	{
		snprintf(lpath, sizeof(lpath) / sizeof(*lpath), "%.*s\0", (int)((++p) - path), path);
		make_dir(lpath);
	}
	snprintf(p_log_set->path, sizeof(p_log_set->path) / sizeof(*(p_log_set->path)), "%s\0", (path != 0) ? (path) : DEFAULT_LOG_PATH);
	if ((p_log_set->path[strlen(p_log_set->path) - 1] != PATH_SEPARATOR_CHAR))
	{
		p_log_set->path[strlen(p_log_set->path)] = PATH_SEPARATOR_CHAR;
	}

	snprintf(p_log_set->fmt, sizeof(p_log_set->fmt) / sizeof(*(p_log_set->fmt)), "%s\0", (fmt != 0) ? (fmt) : "");
	snprintf(p_log_set->ext, sizeof(p_log_set->ext) / sizeof(*(p_log_set->ext)), "%s\0", (path != 0) ? (ext) : DEFAULT_LOG_EXT);
	p_log_set->level = IS_VALID_LOG_LEVEL_TYPE(level) ? level : LOG_VERBOSE;
	p_log_set->limit = limit > 0 ? limit : DEFAULT_LOG_LIMIT_SIZE;
	p_log_set->cons = cons;
	p_log_set->args = args;
	p_log_set->rotatetime = rotatetime > 0 ? rotatetime : DAY_SECONDS;
	p_log_set->argl = (struct log_arg*)malloc(p_log_set->args * sizeof(struct log_arg));
	if (p_log_set->argl == 0)
	{
		exit_log();
		return (0);
	}
	memset(p_log_set->argl, 0, p_log_set->args * sizeof(struct log_arg));

	gettimeofday(&tv, &tz);
	log_secs = tv.tv_sec;
	log_secs -= log_secs % p_log_set->rotatetime;

	for (long i = 0; i < p_log_set->args; i++)
	{
		nCount = 0;
		memcpy(&p_log_set->argl[i], &pargl[i], sizeof(struct log_arg));

		p_log_set->argl[i].data = (char*)malloc(p_log_set->argl[i].size);
		if(p_log_set->argl[i].data == 0)
		{
			exit_log();
			return (0);
		}
		nCount += snprintf(lname + nCount, sizeof(lname) / sizeof(*lname), "%s%s\0", p_log_set->path, p_log_set->argl[i].fname);
		
		if (*p_log_set->fmt)
		{
			tm_log = localtime(&log_secs);
			nCount += (int)(strftime(lname + nCount, sizeof(lname) / sizeof(*lname), p_log_set->fmt, tm_log));
		}
		p_log_set->argl[i].ftime = log_secs;
		nCount += snprintf(lname + nCount, sizeof(lname) / sizeof(*lname), "%s\0", p_log_set->ext);
		snprintf(p_log_set->argl[i].lname, sizeof(p_log_set->argl[i].lname) / sizeof(*p_log_set->argl[i].lname), "%s\0", lname);

		for (long n = 0; n < p_log_set->argl[i].num; n++)
		{
			snprintf(lname, sizeof(lname) / sizeof(*lname), "%s.%d\0", p_log_set->argl[i].lname, n);
			if (access(lname, F_OK) != 0)
			{
				p_log_set->argl[i].idx = (n + 1);
				p_log_set->argl[i].idx %= (p_log_set->argl[i].num + 1);
				p_log_set->argl[i].idx = (p_log_set->argl[i].idx <= 0) ? 1 : p_log_set->argl[i].idx;
				break;
			}
		}

		p_log_set->argl[i].fd = open(p_log_set->argl[i].lname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
		if (p_log_set->argl[i].fd == (-1))
		{
			exit_log();
			return (0);
		}
	}
	
	return (p_log_set);
}

__inline static
int check_log(struct log_arg* pla, time_t now_secs)
{
	int nCount = 0;
	time_t fsecs = 0;
	struct stat st = { 0 };
	__off_t limit = get_log_limit();
	const char* ext = get_log_ext();
	const char* fmt = get_log_fmt();
	char lname[MAX_LOG_PATH] = { 0 };
	const char* path = get_log_path();

	fsecs = now_secs - now_secs % get_log_rotatetime();

	if(pla->ftime != fsecs)
	{
		pla->ftime = fsecs;

		nCount += snprintf(lname + nCount, sizeof(lname) / sizeof(*lname), "%s%s\0", path, pla->fname);
		if (*fmt)
		{
			nCount += (int)(strftime(lname + nCount, sizeof(lname) / sizeof(*lname), fmt, localtime(&fsecs)));
			nCount += snprintf(lname + nCount, sizeof(lname) / sizeof(*lname), "%s\0", ext);

			close(pla->fd);
			snprintf(pla->lname, sizeof(pla->lname) / sizeof(*pla->lname), "%s\0", lname);
			int fd = open(pla->lname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
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
		else
		{
			if (pla->num > 0)
			{
				pla->idx %= (pla->num + 1);
				pla->idx = (pla->idx <= 0) ? 1 : pla->idx;
			}
			snprintf(lname, sizeof(lname) / sizeof(*lname), "%s.%d\0", pla->lname, pla->idx++);

			close(pla->fd);
			unlink(lname);
			if (rename(pla->lname, lname) != 0)
			{
				pla->fd = (-1);
				printf("rename logfile %s -> %s failed msg: (errno=%d)%s\n", pla->lname, lname, errno, strerror(errno));
				return (-1);
			}
			int fd = open(pla->lname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
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
	else
	{
		if (fstat(pla->fd, &st) == (-1))
		{
			switch (errno)
			{
			case ENOENT:
			{
				nCount += snprintf(lname + nCount, sizeof(lname) / sizeof(*lname), "%s%s\0", path, pla->fname);
				if (*fmt)
				{
					nCount += (int)(strftime(lname + nCount, sizeof(lname) / sizeof(*lname), fmt, localtime(&now_secs)));
				}
				nCount += snprintf(lname + nCount, sizeof(lname) / sizeof(*lname), "%s\0", ext);
				snprintf(pla->lname, sizeof(pla->lname) / sizeof(*pla->lname), "%s\0", lname);
				pla->fd = open(pla->lname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
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
			if (st.st_size + pla->size >= limit)
			{
				if (pla->num > 0)
				{
					pla->idx %= (pla->num + 1);
					pla->idx = (pla->idx <= 0) ? 1 : pla->idx;
				}
				snprintf(lname, sizeof(lname) / sizeof(*lname), "%s.%d\0", pla->lname, pla->idx++);

				close(pla->fd);
				unlink(lname);
				if (rename(pla->lname, lname) != 0)
				{
					pla->fd = (-1);
					printf("rename logfile %s -> %s failed msg: (errno=%d)%s\n", pla->lname, lname, errno, strerror(errno));
					return (-1);
				}
				int fd = open(pla->lname, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
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
	}
	
	return (0);
}

__inline static 
int log(const char* fname, int level, const char* fmt, ...)
{
	va_list arg;
	int data_len = 0;
	long tt_now_usecs = 0;
	struct stat st = { 0 };
	struct tm * tm_now = 0;
	time_t tt_now_secs = 0;
	struct log_arg* pla = 0;
	struct timeval tv = { 0 };
	struct timezone tz = { 0 };
	char date_time[128] = { 0 };

	// 若level大于指定的level,则不打印日志信息
	if (level > get_log_top_level())
	{
		return (-1);
	}

	if ((pla = get_log_arg(fname)) == 0)
	{
		return (-1);
	}

	log_lock();

	memset(pla->data, 0, pla->size);

	gettimeofday(&tv, &tz);
	tt_now_secs = tv.tv_sec;
	tt_now_usecs = tv.tv_usec;
	tm_now = localtime(&tt_now_secs);

	strftime(date_time, sizeof(date_time) / sizeof(*date_time), LOG_FILE_FORMAT, tm_now);
	data_len += snprintf(pla->data + data_len, pla->size, "%s[%s.%ld][%ld][%C]\0",
		get_log_level_color(level), date_time, tt_now_usecs, sys_get_tid(), *get_log_level_name(level));
	va_start(arg, fmt);
	data_len += vsnprintf(pla->data + data_len, pla->size, fmt, arg);
	va_end(arg);
	if (LOG_CF == pla->data[data_len - 1])
	{
		data_len--;
		if (LOG_LR == pla->data[data_len - 1])
		{
			data_len--;
		}
	}	
	data_len += snprintf(pla->data + data_len, pla->size, "%s\n\0", get_log_level_color_end());

	check_log(pla, tt_now_secs);
	write(pla->fd, pla->data, data_len);
	if (NEED_LOG_CONS(get_log_cons()))
	{
		//printf("%.*s", data_len, data);
		write(LOG_STDOUT_FILENO, pla->data, data_len);
	}
	log_unlock();
	
	return (0);
}

//#define LOG(NAME,LEVEL,FMT...)		log(#NAME,LEVEL,##FMT)
#define LOG(NAME,LEVEL,FMT,...)	log(#NAME,LEVEL,FMT,##__VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////////


__inline static
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
DWORD
#else
void *
#endif
log_task(void * p) {

	for (size_t i = 0; i < 1000; i++)
	{
		log("main", LOG_INFO, "%s(%d)\n", "I am test!!!!!!", i);
		LOG(main, LOG_WARN, "%s(%d)\n", "I am test!!!!!!", i);
		LOG(main, LOG_TRACE, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_ERROR, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_FATAL, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_WARN, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_TRACE, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_DEBUG, "%s(%d)\n", "I am test!!!!!!", i);

		log("main1", LOG_INFO, "%s(%d)\n", "I am test!!!!!!", i);
		LOG(main1, LOG_WARN, "%s(%d)\n", "I am test!!!!!!", i);
		LOG(main1, LOG_TRACE, "%s(%d)\n", "I am test!!!!!!", i);
		log("main1", LOG_ERROR, "%s(%d)\n", "I am test!!!!!!", i);
		log("main1", LOG_FATAL, "%s(%d)\n", "I am test!!!!!!", i);
		log("main1", LOG_WARN, "%s(%d)\n", "I am test!!!!!!", i);
		log("main1", LOG_TRACE, "%s(%d)\n", "I am test!!!!!!", i);
		log("main1", LOG_DEBUG, "%s(%d)\n", "I am test!!!!!!", i);
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
		//_sleep(1000);
	}
	return (0);
#else
		usleep(1000000);
}
	pthread_exit(0);
	return (void*)(0);
#endif
}

__inline static 
int log_test_main()
{
	const struct log_arg largl[] = {
		{-1,30,0,0,"main","", DATA_SIZE, 0},
		{-1,30,0,0,"main1","", DATA_SIZE, 0},
	};
	long n_rotatetime = 10;//DEFAULT_LOG_ROTATETIME
	const char* log_fmt = "%Y%m%d%H%M%S";// DEFAULT_LOG_FMT;
	init_log(DEFAULT_LOG_PATH, log_fmt, DEFAULT_LOG_EXT, DEFAULT_LOG_CONS,
		n_rotatetime, LOG_VERBOSE, DEFAULT_LOG_LIMIT_SIZE, largl, sizeof(largl) / sizeof(*largl));
	for (size_t i = 0; i < 100; i++)
	{
		LOG(main, LOG_INFO, "%s(%d)\n", "I am test!!!!!!", i);
		LOG(main1, LOG_INFO, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_ERROR, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_FATAL, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_WARN, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_TRACE, "%s(%d)\n", "I am test!!!!!!", i);
		log("main", LOG_DEBUG, "%s(%d)\n", "I am test!!!!!!", i);
		log("main1", LOG_INFO, "%s(%d)\n", "I am test!!!!!!", i);
	}

	int ret = 0;
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
	DWORD t[4] = { 0 };
	HANDLE h[4] = { 0 };
	for (int i = 0; i < sizeof(t) / sizeof(*t); i++)
	{
		h[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)log_task, 0, 0, &t[i]);
	}
	getchar();
	WaitForMultipleObjects(sizeof(t) / sizeof(*t), h, TRUE, INFINITE);	
#else
	pthread_t t[4] = { 0 };
	for (int i = 0; i < sizeof(t)/sizeof(*t); i++)
	{
		ret = pthread_create(&t[i], 0, log_task, 0);
	}		
	getchar();
	for (int i = 0; i < sizeof(t) / sizeof(*t); i++)
	{
		pthread_join(t[i], 0);
	}
#endif

	exit_log();

	return 0;
}

#endif // __CLOG_H__