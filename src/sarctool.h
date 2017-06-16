#ifndef SARCTOOL_H_
#define SARCTOOL_H_

#include <sdw.h>
#include "sarc.h"

class CSarcTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionExtract,
		kActionCreate,
		kActionHelp
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	CSarcTool();
	~CSarcTool();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool extractFile();
	bool createFile();
	EAction m_eAction;
	UString m_sFileName;
	UString m_sDirName;
	CSarc::EEndianness m_eEndianness;
	n32 m_nAlignment;
	u32 m_uHashKey;
	u32 m_uCodePage;
	string m_sCodeName;
	bool m_bNoName;
	bool m_bNameIsHash;
	bool m_bVerbose;
	UString m_sMessage;
};

#endif	// SARCTOOL_H_
