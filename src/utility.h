#ifndef UTILITY_H_
#define UTILITY_H_

#define COMPILER_MSC  1
#define COMPILER_GNUC 2

#if defined(_MSC_VER)
#define SARCTOOL_COMPILER COMPILER_MSC
#else
#define SARCTOOL_COMPILER COMPILER_GNUC
#endif

#if SARCTOOL_COMPILER == COMPILER_MSC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <codecvt>
#else
#if defined(SARCTOOL_APPLE)
#include <mach-o/dyld.h>
#endif
#include <iconv.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

typedef int8_t n8;
typedef int16_t n16;
typedef int32_t n32;
typedef int64_t n64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#if SARCTOOL_COMPILER == COMPILER_MSC
typedef wstring String;
typedef wregex Regex;
#define STR(x) L##x
#define FMkdir _wmkdir
#define FFopen FFopenA
#define FFopenUnicode FFopenW
#define FFseek _fseeki64
#define FFtell _ftelli64
#define FFileno _fileno
#define FLseek _lseeki64
#define FPrintf wprintf
#define MSC_PUSH_PACKED <pshpack1.h>
#define MSC_POP_PACKED <poppack.h>
#define GNUC_PACKED
#else
typedef string String;
typedef regex Regex;
#define STR(x) x
#define FMkdir(x) mkdir((x), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define FFopen FFopenA
#define FFopenUnicode FFopenA
#define FFseek fseeko
#define FFtell ftello
#define FFileno fileno
#define FLseek lseek
#define FPrintf printf
#define MSC_PUSH_PACKED <stdlib.h>
#define MSC_POP_PACKED <stdlib.h>
#define GNUC_PACKED __attribute__((packed))
#endif

#define CONVERT_ENDIAN(n) (((n) >> 24 & 0xFF) | ((n) >> 8 & 0xFF00) | (((n) & 0xFF00) << 8) | (((n) & 0xFF) << 24))

void FSetLocale();

template<typename T>
T FSTrim(const T& a_sString)
{
	typename T::size_type stSize = a_sString.size();
	typename T::size_type st = 0;
	while (st < stSize && a_sString[st] >= 0 && a_sString[st] <= static_cast<typename T::value_type>(' '))
	{
		st++;
	}
	while (st < stSize && a_sString[stSize - 1] >= 0 && a_sString[stSize - 1] <= static_cast<typename T::value_type>(' '))
	{
		stSize--;
	}
	return (st > 0 || stSize < a_sString.size()) ? a_sString.substr(st, stSize - st) : a_sString;
}

template<typename T>
vector<T> FSSplit(const T& a_sString, const T& a_sSeparator)
{
	vector<T> vString;
	for (typename T::size_type nOffset = 0; nOffset < a_sString.size(); nOffset += a_sSeparator.size())
	{
		typename T::size_type nPos = a_sString.find(a_sSeparator, nOffset);
		if (nPos != T::npos)
		{
			vString.push_back(a_sString.substr(nOffset, nPos - nOffset));
			nOffset = nPos;
		}
		else
		{
			vString.push_back(a_sString.substr(nOffset));
			break;
		}
	}
	return vString;
}

template<typename T>
vector<T> FSSplitOf(const T& a_sString, const T& a_sSeparatorSet)
{
	vector<T> vString;
	for (auto it = a_sString.begin(); it != a_sString.end(); ++it)
	{
		auto itPos = find_first_of(it, a_sString.end(), a_sSeparatorSet.begin(), a_sSeparatorSet.end());
		if (itPos != a_sString.end())
		{
			vString.push_back(a_sString.substr(it - a_sString.begin(), itPos - it));
			it = itPos;
		}
		else
		{
			vString.push_back(a_sString.substr(it - a_sString.begin()));
			break;
		}
	}
	return vString;
}

#if SARCTOOL_COMPILER != COMPILER_MSC
template<typename TSrc, typename TDest>
TDest FSTToT(const TSrc& a_sString, const string& a_sSrcType, const string& a_sDestType)
{
	TDest sConverted;
	iconv_t cvt = iconv_open(a_sDestType.c_str(), a_sSrcType.c_str());
	if (cvt == reinterpret_cast<iconv_t>(-1))
	{
		return sConverted;
	}
	size_t nStringLeft = a_sString.size() * sizeof(typename TSrc::value_type);
	static const n32 c_kBufferSize = 1024;
	static const n32 c_kConvertBufferSize = c_kBufferSize - 4;
	char buffer[c_kBufferSize];
	do
	{
		typename TSrc::value_type* pString = const_cast<typename TSrc::value_type*>(a_sString.c_str());
		char* pBuffer = buffer;
		size_t nBufferLeft = c_kConvertBufferSize;
		n32 nError = iconv(cvt, reinterpret_cast<char**>(&pString), &nStringLeft, &pBuffer, &nBufferLeft);
		if (nError == 0 || (nError == static_cast<size_t>(-1) && errno == E2BIG))
		{
			*reinterpret_cast<typename TDest::value_type*>(buffer + c_kConvertBufferSize - nBufferLeft) = 0;
			sConverted += reinterpret_cast<typename TDest::value_type*>(buffer);
			if (nError == 0)
			{
				break;
			}
		}
		else
		{
			break;
		}
	} while (true);
	iconv_close(cvt);
	return sConverted;
}
#endif

wstring FSAToW(const string& a_sString);

#if SARCTOOL_COMPILER == COMPILER_MSC
#define FSAToUnicode(x) FSAToW(x)
#else
#define FSAToUnicode(x) string(x)
#endif

string FFormatV(const char* a_szFormat, va_list a_vaList);
string FFormat(const char* a_szFormat, ...);

template<typename T>
bool FSStartsWith(const T& a_sString, const T& a_sPrefix, typename T::size_type a_stStart = 0)
{
	if (a_stStart > a_sString.size())
	{
		return false;
	}
	return a_sString.compare(a_stStart, a_sPrefix.size(), a_sPrefix) == 0;
}

const String& FGetModuleDir();

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize);

bool FMakeDir(const String::value_type* a_pDirName);

FILE* FFopenA(const char* a_pFileName, const char* a_pMode);

#if SARCTOOL_COMPILER == COMPILER_MSC
FILE* FFopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode);
#endif

bool FSeek(FILE* a_fpFile, n64 a_nOffset);

n64 FAlign(n64 a_nOffset, n64 a_nAlignment);

#endif	// UTILITY_H_
