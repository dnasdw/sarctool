#include "sarctool.h"

CSarcTool::SOption CSarcTool::s_Option[] =
{
	{ USTR("extract"), USTR('x'), USTR("extract the sarc file") },
	{ USTR("create"), USTR('c'), USTR("create the sarc file") },
	{ USTR("file"), USTR('f'), USTR("the sarc file") },
	{ USTR("dir"), USTR('d'), USTR("the dir for the sarc file") },
	{ USTR("endianness"), USTR('o'), USTR("[big|little]\n\t\tthe endianness, default is little") },
	{ USTR("alignment"), USTR('a'), USTR("[192|2 factorial (more than 4)]\n\t\tthe alignment, default is 4") },
	{ USTR("unique-alignment"), USTR('r'), USTR("the path regex pattern and the unique alignment") },
	{ USTR("data-offset-alignment"), 0, USTR("[0(auto)|192|2 factorial (more than 4)]\n\t\tthe sarc header DataOffset alignment, default is 0") },
	{ USTR("key"), USTR('k'), USTR("the hash key, default is 101") },
	{ USTR("code-page"), USTR('p'), USTR("code page of SFNT, default is 0 (Windows only)") },
	{ USTR("code-name"), USTR('e'), USTR("encoding name of SFNT, default is UTF-8 (non-Windows only)") },
	{ USTR("no-name"), USTR('n'), USTR("no name in SFNT") },
	{ USTR("name-is-hash"), USTR('i'), USTR("the name is a hash") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, nullptr }
};

CSarcTool::CSarcTool()
	: m_eAction(kActionNone)
	, m_eEndianness(CSarc::kEndianLittle)
	, m_nAlignment(4)
	, m_nDataOffsetAlignment(0)
	, m_uHashKey(101)
	, m_uCodePage(0)
	, m_sCodeName("UTF-8")
	, m_bNoName(false)
	, m_bNameIsHash(false)
	, m_bVerbose(false)
{
}

CSarcTool::~CSarcTool()
{
}

int CSarcTool::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnUnknownArgument:
					UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnUnknownArgument:
				UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int CSarcTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionHelp)
	{
		if (m_sFileName.empty())
		{
			UPrintf(USTR("ERROR: no --file option\n\n"));
			return 1;
		}
		if (m_sDirName.empty())
		{
			UPrintf(USTR("ERROR: no --dir option\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionExtract)
	{
		if (!CSarc::IsSarcFile(m_sFileName))
		{
			UPrintf(USTR("ERROR: %") PRIUS USTR(" is not a sarc file\n\n"), m_sFileName.c_str());
			return 1;
		}
	}
	return 0;
}

int CSarcTool::Help()
{
	UPrintf(USTR("sarctool %") PRIUS USTR(" by dnasdw\n\n"), AToU(SARCTOOL_VERSION).c_str());
	UPrintf(USTR("usage: sarctool [option...] [option]...\n\n"));
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("  sarctool -xvfd input.sarc outputdir\n"));
	UPrintf(USTR("  sarctool -cvfd output.sarc inputdir\n"));
	UPrintf(USTR("  sarctool -cvfd output.sarc inputdir -o big -a 8192\n"));
	UPrintf(USTR("  sarctool -cvfd output.sarc inputdir -r \\.bch 128 --data-offset-alignment 128\n"));
	UPrintf(USTR("\n"));
	UPrintf(USTR("option:\n"));
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
		pOption++;
	}
	return 0;
}

int CSarcTool::Action()
{
	if (m_eAction == kActionExtract)
	{
		if (!extractFile())
		{
			UPrintf(USTR("ERROR: extract file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			UPrintf(USTR("ERROR: create file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CSarcTool::EParseOptionReturn CSarcTool::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("extract")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionExtract;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("create")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("dir")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sDirName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("endianness")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sEndianness = a_pArgv[++a_nIndex];
		if (sEndianness == USTR("big"))
		{
			m_eEndianness = CSarc::kEndianBig;
		}
		else if (sEndianness == USTR("little"))
		{
			m_eEndianness = CSarc::kEndianLittle;
		}
		else
		{
			m_sMessage = sEndianness;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("alignment")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sAlignment = a_pArgv[++a_nIndex];
		n32 nAlignment = SToN32(sAlignment);
		if (nAlignment < 4)
		{
			m_sMessage = sAlignment;
			return kParseOptionReturnUnknownArgument;
		}
		if (nAlignment != 192)
		{
			bitset<32> bsAlignment(nAlignment);
			if (bsAlignment.count() != 1)
			{
				m_sMessage = sAlignment;
				return kParseOptionReturnUnknownArgument;
			}
		}
		m_nAlignment = nAlignment;
	}
	else if (UCscmp(a_pName, USTR("unique-alignment")) == 0)
	{
		if (a_nIndex + 2 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sPattern = a_pArgv[++a_nIndex];
		UString sAlignment = a_pArgv[++a_nIndex];
		n32 nAlignment = SToN32(sAlignment);
		if (nAlignment < 4)
		{
			m_sMessage = sAlignment;
			return kParseOptionReturnUnknownArgument;
		}
		if (nAlignment != 192)
		{
			bitset<32> bsAlignment(nAlignment);
			if (bsAlignment.count() != 1)
			{
				m_sMessage = sAlignment;
				return kParseOptionReturnUnknownArgument;
			}
		}
		try
		{
			URegex rPattern(sPattern, regex_constants::ECMAScript | regex_constants::icase);
			m_mUniqueAlignment[nAlignment].push_back(rPattern);
		}
		catch (regex_error& e)
		{
			UPrintf(USTR("ERROR: %") PRIUS USTR("\n\n"), AToU(e.what()).c_str());
			m_sMessage = sPattern;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("data-offset-alignment")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sAlignment = a_pArgv[++a_nIndex];
		n32 nAlignment = SToN32(sAlignment);
		if (nAlignment != 0)
		{
			if (nAlignment < 4)
			{
				m_sMessage = sAlignment;
				return kParseOptionReturnUnknownArgument;
			}
			if (nAlignment != 192)
			{
				bitset<32> bsAlignment(nAlignment);
				if (bsAlignment.count() != 1)
				{
					m_sMessage = sAlignment;
					return kParseOptionReturnUnknownArgument;
				}
			}
		}
		m_nDataOffsetAlignment = nAlignment;
	}
	else if (UCscmp(a_pName, USTR("key")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sKey = a_pArgv[++a_nIndex];
		m_uHashKey = SToU32(sKey);
	}
	else if (UCscmp(a_pName, USTR("code-page")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sCodePage = a_pArgv[++a_nIndex];
		m_uCodePage = SToU32(sCodePage);
	}
	else if (UCscmp(a_pName, USTR("code-name")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UString sCodeName = a_pArgv[++a_nIndex];
		m_sCodeName = UToA(sCodeName);
	}
	else if (UCscmp(a_pName, USTR("no-name")) == 0)
	{
		m_bNoName = true;
	}
	else if (UCscmp(a_pName, USTR("name-is-hash")) == 0)
	{
		m_bNameIsHash = true;
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

CSarcTool::EParseOptionReturn CSarcTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, UChar* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool CSarcTool::extractFile()
{
	CSarc sarc;
	sarc.SetFileName(m_sFileName);
	sarc.SetDirName(m_sDirName);
	sarc.SetCodePage(m_uCodePage);
	sarc.SetCodeName(m_sCodeName);
	sarc.SetVerbose(m_bVerbose);
	return sarc.ExtractFile();
}

bool CSarcTool::createFile()
{
	CSarc sarc;
	sarc.SetFileName(m_sFileName);
	sarc.SetDirName(m_sDirName);
	sarc.SetEndianness(m_eEndianness);
	sarc.SetAlignment(m_nAlignment);
	sarc.SetUniqueAlignment(m_mUniqueAlignment);
	sarc.SetDataOffsetAlignment(m_nDataOffsetAlignment);
	sarc.SetHashKey(m_uHashKey);
	sarc.SetCodePage(m_uCodePage);
	sarc.SetCodeName(m_sCodeName);
	sarc.SetNoName(m_bNoName);
	sarc.SetNameIsHash(m_bNameIsHash);
	sarc.SetVerbose(m_bVerbose);
	return sarc.CreateFile();
}

int UMain(int argc, UChar* argv[])
{
	CSarcTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
