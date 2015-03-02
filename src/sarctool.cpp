#include "sarctool.h"
#include "sarc.h"

CSarcTool::SOption CSarcTool::s_Option[] =
{
	{ "export", 'e', "export from the sarc file" },
	{ "import", 'i', "import to the sarc file" },
	{ "file", 'f', "the sarc file" },
	{ "dir", 'd', "the dir for the sarc file" },
	{ "verbose", 'v', "show the info" },
	{ "help", 'h', "show this help" },
	{ nullptr, 0, nullptr }
};

CSarcTool::CSarcTool()
	: m_eAction(kActionNone)
	, m_pFileName(nullptr)
	, m_pDirName(nullptr)
	, m_bVerbose(false)
{
}

CSarcTool::~CSarcTool()
{
}

int CSarcTool::ParseOptions(int a_nArgc, char* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(strlen(a_pArgv[i]));
		int nIndex = i;
		if (a_pArgv[i][0] != '-')
		{
			printf("ERROR: illegal option\n\n");
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != '-')
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnOptionConflict:
					printf("ERROR: option conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == '-')
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				printf("ERROR: illegal option\n\n");
				return 1;
			case kParseOptionReturnNoArgument:
				printf("ERROR: no argument\n\n");
				return 1;
			case kParseOptionReturnOptionConflict:
				printf("ERROR: option conflict\n\n");
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
		printf("ERROR: nothing to do\n\n");
		return 1;
	}
	if (m_eAction != kActionHelp)
	{
		if (m_pFileName == nullptr)
		{
			printf("ERROR: no --file option\n\n");
			return 1;
		}
		if (m_pDirName == nullptr)
		{
			printf("ERROR: no --dir option\n\n");
			return 1;
		}
		if (!CSarc::IsSarcFile(m_pFileName))
		{
			printf("ERROR: %s is not a sarc file\n\n", m_pFileName);
			return 1;
		}
	}
	return 0;
}

int CSarcTool::Help()
{
	printf("sarctool %s by dnasdw\n\n", SARCTOOL_VERSION);
	printf("usage: sarctool [option...] [option]...\n");
	printf("sample:\n");
	printf("  sarctool -evfd input.sarc outputdir\n");
	printf("  sarctool -ivfd output.sarc inputdir\n");
	printf("\n");
	printf("option:\n");
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			printf("  ");
			if (pOption->Key != 0)
			{
				printf("-%c,", pOption->Key);
			}
			else
			{
				printf("   ");
			}
			printf(" --%-8s", pOption->Name);
			if (strlen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				printf("\n%16s", "");
			}
		}
		if (pOption->Doc != nullptr)
		{
			printf("%s", pOption->Doc);
		}
		printf("\n");
		pOption++;
	}
	return 0;
}

int CSarcTool::Action()
{
	if (m_eAction == kActionExport)
	{
		if (!exportFile())
		{
			printf("ERROR: export file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionImport)
	{
		if (!importFile())
		{
			printf("ERROR: import file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CSarcTool::EParseOptionReturn CSarcTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "export") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionExport;
		}
		else if (m_eAction != kActionExport && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "import") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionImport;
		}
		else if (m_eAction != kActionImport && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

CSarcTool::EParseOptionReturn CSarcTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, char* a_pArgv[])
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

bool CSarcTool::exportFile()
{
	CSarc sarc;
	sarc.SetFileName(m_pFileName);
	sarc.SetDirName(m_pDirName);
	sarc.SetVerbose(m_bVerbose);
	return sarc.ExportFile();
}

bool CSarcTool::importFile()
{
	CSarc sarc;
	sarc.SetFileName(m_pFileName);
	sarc.SetDirName(m_pDirName);
	sarc.SetVerbose(m_bVerbose);
	return sarc.ImportFile();
}

int main(int argc, char* argv[])
{
	FSetLocale();
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
