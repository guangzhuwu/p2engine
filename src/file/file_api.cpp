#include "p2engine/file/file_api.hpp"

#ifdef WIN32
#	include <Windows.h>
#	include <WinIoCtl.h>
#else
#	include <fcntl.h>
#	include <unistd.h>
#endif

namespace p2engine{

	using boost::system::get_system_category;

#ifdef WIN32
#	define OPEN ::_wopen 
#else
#	define OPEN ::open 
#endif

	int open_file(const boost::filesystem::path& path, int mode, 
		boost::system::error_code& ec)
	{
#ifdef S_IRUSR
		// rely on default umask to filter x and w permissions
		// for group and others
		int permissions = S_IRUSR | S_IWUSR
			| S_IRGRP | S_IWGRP
			| S_IROTH | S_IWOTH;

		if (mode & attribute_executable)
			permissions |= S_IXGRP | S_IXOTH | S_IXUSR;
#else
		int permissions=0;
#endif

		static const int mode_array[] = {O_RDONLY, O_WRONLY | O_CREAT, O_RDWR | O_CREAT};

#ifdef O_NOATIME
		static const int no_atime_flag[] = {0, O_NOATIME};
#endif

		int handle = OPEN(native_path_string(path).c_str()
			, mode_array[mode & _rw_mask]
#ifdef O_NOATIME
		| no_atime_flag[(mode & no_atime) >> 4]
#endif
#ifdef O_NONBLOCK
		|O_NONBLOCK
#endif
			, permissions);

		if (handle == -1)
		{
			ec.assign(errno, get_system_category());
			BOOST_ASSERT(ec);
			return handle;
		}

#ifdef F_SETLK
		if (mode & lock_file)
		{
			struct flock l =
			{
				0, // start offset
				0, // length (0 = until EOF)
				getpid(), // owner
				(mode & write_only) ? F_WRLCK : F_RDLCK, // lock type
				SEEK_SET // whence
			};
			if (fcntl(handle, F_SETLK, &l) != 0)
			{
				ec.assign(errno, get_system_category());
				return handle;
			}
		}
#endif
#if defined(HAVE_POSIX_FADVISE)&&HAVE_POSIX_FADVISE || !defined HAVE_POSIX_FADVISE
#	ifdef POSIX_FADV_RANDOM
		if (mode & random_access)
		{
			// disable read-ahead
			posix_fadvise(handle, 0, 0, POSIX_FADV_RANDOM);
		}
#	endif
#endif
		return handle;
	}


#ifdef BOOST_WINDOWS_API
	void* create_file(const boost::filesystem::path& path, int mode, 
		boost::system::error_code& ec)
	{
		struct open_mode_t
		{
			DWORD rw_mode;
			DWORD create_mode;
		};

		const static open_mode_t mode_array[] =
		{
			// read_only
			{GENERIC_READ, OPEN_EXISTING},
			// write_only
			{GENERIC_WRITE, OPEN_ALWAYS},
			// read_write
			{GENERIC_WRITE | GENERIC_READ, OPEN_ALWAYS},
		};

		const static DWORD attrib_array[] =
		{
			FILE_ATTRIBUTE_NORMAL, // no attrib
			FILE_ATTRIBUTE_HIDDEN, // hidden
			FILE_ATTRIBUTE_NORMAL, // executable
			FILE_ATTRIBUTE_HIDDEN, // hidden + executable
		};

		const static DWORD share_array[] =
		{
			// read only (no locking)
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			// write only (no locking)
			FILE_SHARE_READ,
			// read/write (no locking)
			FILE_SHARE_READ,
		};

		BOOST_ASSERT((mode & _rw_mask) < sizeof(mode_array)/sizeof(mode_array[0]));
		open_mode_t const& m = mode_array[mode & _rw_mask];
		DWORD a = attrib_array[(mode & _attribute_mask) >> 12];

		DWORD flags
			= ((mode & random_access) ? FILE_FLAG_RANDOM_ACCESS : 0)
			| (a ? a : FILE_ATTRIBUTE_NORMAL)
			| FILE_FLAG_OVERLAPPED ;

		HANDLE handle = CreateFileW(native_path_string(path).c_str()
			, m.rw_mode
			, (mode & lock_file) ? 0 : share_array[mode & _rw_mask]
		, 0, m.create_mode, flags, 0);

		if (handle == INVALID_HANDLE_VALUE)
		{
			ec.assign(GetLastError(), get_system_category());
			BOOST_ASSERT(ec);
			return handle;
		}

		// try to make the file sparse if supported
		// only set this flag if the file is opened for writing
		if ((mode & sparse) && (mode & _rw_mask) != read_only)
		{
			DWORD temp;
			::DeviceIoControl(handle, FSCTL_SET_SPARSE, 0, 0, 0, 0, &temp, 0);
		}
		return handle;
	}

#endif //WINDOWS


	boost::filesystem::path get_exe_path() {
#if defined(MAC_OS)
		char path[1024];
		uint32_t size = sizeof(path)-1;
		if (_NSGetExecutablePath(&path[0], &size) == 0) 
		{
			path[size]='\0';
			return boost::filesystem::path(path).parent_path();
		}
#elif defined(LINUX_OS)||defined(ANDROID_OS)
		char path[1024];
		ssize_t size = readlink("/proc/self/exe",path, sizeof(path)-1);
		if (size > 0)
		{
			path[size]='\0';
			return boost::filesystem::path(path).parent_path();
		}
#elif defined(WINDOWS_OS)
		char path[1024];
		int size =::GetModuleFileName(NULL, path, sizeof(path)-1);
		if (size > 0) 
		{
			path[size]='\0';
			return boost::filesystem::path(path).parent_path();
		}
#else
# error get_exe_path()?? OS Not Supported!
#endif
		return boost::filesystem::path();
	}

}
