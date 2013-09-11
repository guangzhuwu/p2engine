#include "p2engine/utf8.hpp"
#ifdef _WIN32 
#	include <windows.h>
#endif

#ifndef P2ENGINE_NO_WIDE_FUNCTIONS

namespace p2engine {

#ifdef _WIN32 
	static const char *locLang = NULL; 
	static const char *locRegion = NULL; 
	//static const char *locCharset = NULL; 
	//static const char *locModifier = NULL; 
	static bool locMatched = false; 

	// Callback for EnumSystemLocales(), used by OS::SetLocale(). 
	// See EnumLocalesProc in MSDN. 
	static BOOL CALLBACK SetLocaleProc(LPTSTR locale) 
	{ 
		char *s; 
		LCID lcid = strtoul(locale, &s, 16); 
		if (*s != '\0') return TRUE; 

		// Check for matching language name. 
		char curLang[16]; 
		if (GetLocaleInfo(lcid, LOCALE_SISO639LANGNAME, curLang, 16) == 0) return TRUE; 
		if (strcmp(curLang, locLang) != 0) return TRUE; 

		// Check for matching region. 
		if (locRegion == NULL) { 
			if (SUBLANGID(LANGIDFROMLCID(lcid)) != SUBLANG_DEFAULT) return TRUE; 
		} 
		else { 
			char curRegion[16]; 
			if (GetLocaleInfo(lcid, LOCALE_SISO3166CTRYNAME, curRegion, 16) == 0) return TRUE; 
			if (strcmp(curRegion, locRegion) != 0) return TRUE; 
		} 

		// If we made it this far, then we have a match! 
		// Note: This does nothing on Vista and later. 
		if (SetThreadLocale(lcid) == 0) { 
			// Found a matching LCID but couldn't set it. 
			/* 
			DWORD err = GetLastError(); 
			LPVOID errMsg; 
			FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM, 
			NULL, 
			err, 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			(LPTSTR)&errMsg, 
			0, NULL ); 
			0	        MessageBox(NULL, (const char*)errMsg, "AIEEE", MB_ICONERROR | MB_APPLMODAL | MB_OK); 
			LocalFree(errMsg); 
			*/ 
			assert(FALSE); 
		} 
		locMatched = true; 
		return FALSE; 
	} 
#endif 

	void SetLocale()
	{
		// Common setting.
		setlocale(LC_ALL, "");

		std::locale locale;
#   ifdef _WIN32
		// For Win32, each thread has its own locale setting.
		// We need to map the ISO-abbreviated locale name from the env
		// to a Win32 LCID by enumerating all of the supported locales
		// and finding the one that matches.

		char *lang = getenv("LC_ALL");
		if (lang == NULL || *lang == '\0') {
			lang = getenv("LC_MESSAGES");
			if (lang == NULL || *lang == '\0') {
				lang = getenv("LANG");
				if (lang == NULL || *lang == '\0') {
					SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
					return;
				}
			}
		}
		lang = _strdup(lang);

		if (strcmp(lang, "C") == 0) {
			// The "C" locale on Win32 roughly means to use system default.
			SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
		}
		else {
			// POSIX locale string format:
			//   lang[_region][.charset][@modifier]
			// lang corresponds to the LOCALE_SISO639LANGNAME and
			// region corresponds to the LOCALE_SISO3166CTRYNAME.
			// We currently ignore the charset and modifier.
			char* locLang = lang;
			char* locRegion = NULL;
			//locCharset = NULL;
			//locModifier = NULL;
			for (char *s = lang; *s != '\0'; ++s) {
				switch (*s) {
				case '_':
					*s = '\0';
					locRegion = s + 1;
					break;
				case '.':
					*s = '\0';
					//locCharset = s + 1;
					break;
				case '@':
					*s = '\0';
					//locModifier = s + 1;
					break;
				}
			}

			bool locMatched = false;
			EnumSystemLocales(SetLocaleProc, LCID_SUPPORTED);
			if (!locMatched) {
				//TODO: Log that we didn't find any matching language.
				assert(FALSE);
				SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
				locale = std::locale("C");
			}
		}

		free(lang);
#   else
		try {
			locale = std::locale("");
		}
		catch (std::runtime_error&) { 
			/*std::cerr << "Unsupported locale (falling back to default)." << 
			std::endl;*/
			locale = std::locale("C");
		}
#   endif

		// Update the current locale instance.
		std::locale::global(locale);
	};

	static struct local_set 
	{
		local_set(){SetLocale();}
	}local_set;

	int utf8_wchar(const std::string &utf8, std::wstring &wide)
	{
		// allocate space for worst-case
		wide.resize(utf8.size());
		wchar_t const* dst_start = wide.c_str();
		char const* src_start = utf8.c_str();
		ConversionResult ret;
		if (sizeof(wchar_t) == sizeof(UTF32))
		{
			ret = ConvertUTF8toUTF32((const UTF8**)&src_start, (const UTF8*)src_start
				+ utf8.size(), (UTF32**)&dst_start, (UTF32*)dst_start + wide.size()
				, lenientConversion);
			wide.resize(dst_start - wide.c_str());
			return ret;
		}
		else if (sizeof(wchar_t) == sizeof(UTF16))
		{
			ret = ConvertUTF8toUTF16((const UTF8**)&src_start, (const UTF8*)src_start
				+ utf8.size(), (UTF16**)&dst_start, (UTF16*)dst_start + wide.size()
				, lenientConversion);
			wide.resize(dst_start - wide.c_str());
			return ret;
		}
		else
		{
			return sourceIllegal;
		}
	}

	int wchar_utf8(const std::wstring &wide, std::string &utf8)
	{
		// allocate space for worst-case
		utf8.resize(wide.size() * 6);
		if (wide.empty()) return 0;
		char* dst_start = &utf8[0];
		wchar_t const* src_start = wide.c_str();
		ConversionResult ret;
		if (sizeof(wchar_t) == sizeof(UTF32))
		{
			ret = ConvertUTF32toUTF8((const UTF32**)&src_start, (const UTF32*)src_start
				+ wide.size(), (UTF8**)&dst_start, (UTF8*)dst_start + utf8.size()
				, lenientConversion);
			utf8.resize(dst_start - &utf8[0]);
			return ret;
		}
		else if (sizeof(wchar_t) == sizeof(UTF16))
		{
			ret = ConvertUTF16toUTF8((const UTF16**)&src_start, (const UTF16*)src_start
				+ wide.size(), (UTF8**)&dst_start, (UTF8*)dst_start + utf8.size()
				, lenientConversion);
			utf8.resize(dst_start - &utf8[0]);
			return ret;
		}
		else
		{
			return sourceIllegal;
		}
	}

	std::wstring convert_to_wstring(std::string const& s)
	{
		if (s.empty())
			return 	std::wstring();

		std::wstring ret;
		int result = utf8_wchar(s, ret);
		if (result == 0) return ret;

		ret.clear();
		const char* end = &s[0] + s.size();
		for (const char* i = &s[0]; i < end;)
		{
			wchar_t c = '.';
			int result = std::mbtowc(&c, i, end - i);
			if (result > 0) i += result;
			else ++i;
			ret += c;
		}
		return ret;
	}

	std::string convert_from_wstring(std::wstring const& s)
	{
		if (s.empty())
			return "";

		std::string ret;
		int result = wchar_utf8(s, ret);
		if (result == 0) return ret;

		ret.clear();
		const wchar_t* end = &s[0] + s.size();
		for (const wchar_t* i = &s[0]; i < end;)
		{
			char c[10];
			BOOST_ASSERT(sizeof(c) >= MB_CUR_MAX);
			int result = std::wctomb(c, *i);
			if (result > 0)
			{
				i += result;
				ret.append(c, result);
			}
			else
			{
				++i;
				ret += ".";
			}
		}
		return ret;
	}

	std::string convert_to_native(std::string const& s)
	{
		if (s.empty())
			return "";

		std::wstring ws;
		if(utf8_wchar(s, ws)!=conversionOK)
			return s;
		std::size_t size = wcstombs(0, ws.c_str(), 0);
		if (size==std::size_t(-1)) return s;
		std::string ret;
		ret.resize(size);
		size = wcstombs(&ret[0], ws.c_str(), size);
		if (size == std::size_t(-1)) return s;
		return ret;
	}

	std::string convert_from_native(std::string const& s)
	{
		if (s.empty())
			return "";

		std::wstring ws;
		ws.resize(s.size());
		std::size_t size = mbstowcs(&ws[0], s.c_str(), s.size());
		if (size == std::size_t(-1)) return s;
		std::string ret;
		if(wchar_utf8(ws, ret)!=conversionOK)
			return s;
		return ret;
	}

}

#elif defined(POSIX_OS)&&defined(P2ENGINE_USE_ICONV)&&P2ENGINE_USE_ICONV
#include <iconv.h>
#include <locale.h>
#include "p2engine/mutex.hpp"

namespace p2engine {

	std::string iconv_convert_impl(std::string const& s, iconv_t h)
	{
		std::string ret;
		size_t insize = s.size();
		size_t outsize = insize * 4;
		ret.resize(outsize);
		char const* in = s.c_str();
		char* out = &ret[0];
		// posix has a weird iconv signature. implementations
		// differ on what this signature should be, so we use
		// a macro to let config.hpp determine it
		size_t retval = iconv(h, (char**) &in, &insize,
			&out, &outsize);
		if (retval == (size_t)-1) return s;
		// if this string has an invalid utf-8 sequence in it, don't touch it
		if (insize != 0) return s;
		// not sure why this would happen, but it seems to be possible
		if (outsize > s.size() * 4) return s;
		// outsize is the number of bytes unused of the out-buffer
		BOOST_ASSERT(ret.size() >= outsize);
		ret.resize(ret.size() - outsize);
		return ret;
	}

	std::string convert_to_native(std::string const& s)
	{
		static fast_mutex iconv_mutex;
		// only one thread can use this handle at a time
		fast_mutex::scoped_lock l(iconv_mutex);

		// the empty string represents the local dependent encoding
		static iconv_t iconv_handle = iconv_open("", "UTF-8");
		if (iconv_handle == iconv_t(-1)) return s;
		return iconv_convert_impl(s, iconv_handle);
	}

	std::string convert_from_native(std::string const& s)
	{
		static fast_mutex iconv_mutex;
		// only one thread can use this handle at a time
		fast_mutex::scoped_lock l(iconv_mutex);

		// the empty string represents the local dependent encoding
		static iconv_t iconv_handle = iconv_open("UTF-8", "");
		if (iconv_handle == iconv_t(-1)) return s;
		return iconv_convert_impl(s, iconv_handle);
	}
}

#endif//P2ENGINE_NO_WIDE_FUNCTIONS

