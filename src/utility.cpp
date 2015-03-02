#include "utility.h"

#if SARCTOOL_COMPILER != COMPILER_MSC
string g_sLocaleName = "";
#endif

void FSetLocale()
{
	string sLocale = setlocale(LC_ALL, "");
#if SARCTOOL_COMPILER != COMPILER_MSC
	vector<string> vLocale = FSSplit<string>(sLocale, ".");
	if (!vLocale.empty())
	{
		g_sLocaleName = vLocale.back();
	}
#endif
}

#if SARCTOOL_COMPILER == COMPILER_MSC
wstring FSAToW(const string& a_sString)
{
	static wstring_convert<codecvt<wchar_t, char, mbstate_t>> c_cvt_a(new codecvt<wchar_t, char, mbstate_t>(""));
	return c_cvt_a.from_bytes(a_sString);
}
#else
wstring FSAToW(const string& a_sString)
{
	return FSTToT<string, wstring>(a_sString, g_sLocaleName, "WCHAR_T");
}
#endif

static const n32 s_kFormatBufferSize = 4096;

string FFormatV(const char* a_szFormat, va_list a_vaList)
{
	static char c_szBuffer[s_kFormatBufferSize] = {};
	vsnprintf(c_szBuffer, s_kFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

string FFormat(const char* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	string sFormatted = FFormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}

const String& FGetModuleDir()
{
	const int nMaxPath = 4096;
	static String sDir;
	sDir.resize(nMaxPath, STR('\0'));
#if SARCTOOL_COMPILER == COMPILER_MSC
	GetModuleFileNameW(nullptr, &sDir.front(), nMaxPath);
#elif defined(SARCTOOL_APPLE)
	char path[nMaxPath] = {};
	u32 uPathSize = static_cast<u32>(sizeof(path));
	if (_NSGetExecutablePath(path, &uPathSize) != 0)
	{
		printf("ERROR: _NSGetExecutablePath error\n\n");
	}
	else if (realpath(path, &sDir.front()) == nullptr)
	{
		sDir.erase();
	}
#else
	ssize_t nCount = readlink("/proc/self/exe", &sDir.front(), nMaxPath);
	if (nCount == -1)
	{
		printf("ERROR: readlink /proc/self/exe error\n\n");
	}
	else
	{
		sDir[nCount] = '\0';
	}
#endif
	replace(sDir.begin(), sDir.end(), STR('\\'), STR('/'));
	String::size_type nPos = sDir.rfind(STR('/'));
	if (nPos != String::npos)
	{
		sDir.erase(nPos);
	}
	return sDir;
}

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	FFseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		fwrite(pBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pBuffer;
}

bool FMakeDir(const String::value_type* a_pDirName)
{
	if (FMkdir(a_pDirName) != 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}
	return true;
}

FILE* FFopenA(const char* a_pFileName, const char* a_pMode)
{
	FILE* fp = fopen(a_pFileName, a_pMode);
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
	}
	return fp;
}

#if SARCTOOL_COMPILER == COMPILER_MSC
FILE* FFopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode)
{
	FILE* fp = _wfopen(a_pFileName, a_pMode);
	if (fp == nullptr)
	{
		wprintf(L"ERROR: open file %s failed\n\n", a_pFileName);
	}
	return fp;
}
#endif

bool FSeek(FILE* a_fpFile, n64 a_nOffset)
{
	if (fflush(a_fpFile) != 0)
	{
		return false;
	}
	int nFd = FFileno(a_fpFile);
	if (nFd == -1)
	{
		return false;
	}
	FFseek(a_fpFile, 0, SEEK_END);
	n64 nFileSize = FFtell(a_fpFile);
	if (nFileSize < a_nOffset)
	{
		n64 nOffset = FLseek(nFd, a_nOffset - 1, SEEK_SET);
		if (nOffset == -1)
		{
			return false;
		}
		fputc(0, a_fpFile);
		fflush(a_fpFile);
	}
	else
	{
		FFseek(a_fpFile, a_nOffset, SEEK_SET);
	}
	return true;
}

n64 FAlign(n64 a_nOffset, n64 a_nAlignment)
{
	return (a_nOffset + a_nAlignment - 1) / a_nAlignment * a_nAlignment;
}
