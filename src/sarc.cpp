#include "sarc.h"

const u32 CSarc::s_uSignatureSarc = CONVERT_ENDIAN('SARC');
const u32 CSarc::s_uSignatureSfat = CONVERT_ENDIAN('SFAT');
const u32 CSarc::s_uSignatureSfnt = CONVERT_ENDIAN('SFNT');
const int CSarc::s_nEntryAlignment4 = 4;
const int CSarc::s_nEntryAlignment128 = 128;

CSarc::CSarc()
	: m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_fpSarc(nullptr)
{
	memset(&m_SarcHeader, 0, sizeof(m_SarcHeader));
	memset(&m_SfatHeader, 0, sizeof(m_SfatHeader));
	memset(&m_SfntHeader, 0, sizeof(m_SfntHeader));
}

CSarc::~CSarc()
{
}

void CSarc::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CSarc::SetDirName(const char* a_pRomFsDirName)
{
	m_sDirName = FSAToUnicode(a_pRomFsDirName);
}

void CSarc::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CSarc::ExportFile()
{
	bool bResult = true;
	m_fpSarc = FFopen(m_pFileName, "rb");
	if (m_fpSarc == nullptr)
	{
		return false;
	}
	fread(&m_SarcHeader, sizeof(m_SarcHeader), 1, m_fpSarc);
	fread(&m_SfatHeader, sizeof(m_SfatHeader), 1, m_fpSarc);
	m_vEntry.resize(m_SfatHeader.EntryCount);
	if (!m_vEntry.empty())
	{
		fread(&*m_vEntry.begin(), sizeof(SEntry), m_vEntry.size(), m_fpSarc);
	}
	fread(&m_SfntHeader, sizeof(m_SfntHeader), 1, m_fpSarc);
	m_vName.resize(m_SarcHeader.DataOffset - static_cast<u32>(FFtell(m_fpSarc)));
	if (!m_vName.empty())
	{
		fread(&*m_vName.begin(), 1, m_vName.size(), m_fpSarc);
	}
	FMakeDir(m_sDirName.c_str());
	for (n32 i = 0; i < m_SfatHeader.EntryCount; i++)
	{
		String sFilePath;
		bool bNoName = (m_vEntry[i].NameOffset & 0x01000000) == 0;
		if (bNoName)
		{
			char sBuffer[4] = {};
			const char* pExt = "bin";
			if (m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset > 0x28)
			{
				FFseek(m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].EndOffset - 0x28, SEEK_SET);
				fread(sBuffer, 1, 4, m_fpSarc);
				pExt = getExt(sBuffer);
			}
			if (strcmp(pExt, "bflim") != 0)
			{
				FFseek(m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].BeginOffset, SEEK_SET);
				fread(sBuffer, 1, m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset >= 4 ? 4 : m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset, m_fpSarc);
				pExt = getExt(sBuffer);
			}
			sFilePath = FSAToUnicode(FFormat("%08X.%s", m_vEntry[i].Hash, pExt));
		}
		else
		{
			sFilePath = FSAToUnicode(&*m_vName.begin() + (m_vEntry[i].NameOffset & 0xFFFFFF) * 4);
		}
		vector<String> sSplitPath = FSSplitOf<String>(sFilePath, STR("/\\"));
		String sDirPath = m_sDirName;
		for (int j = 0; j < static_cast<int>(sSplitPath.size()) - 1; j++)
		{
			sDirPath += STR("/");
			sDirPath += sSplitPath[j];
			FMakeDir(sDirPath.c_str());
		}
		sFilePath = sDirPath + STR("/") + sSplitPath[sSplitPath.size() - 1];
		FILE* fp = FFopenUnicode(sFilePath.c_str(), STR("wb"));
		if (fp == nullptr)
		{
			bResult = false;
			break;
		}
		if (m_bVerbose)
		{
			FPrintf(STR("save: %s\n"), sFilePath.c_str());
		}
		FCopyFile(fp, m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].BeginOffset, m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset);
		fclose(fp);
	}
	fclose(m_fpSarc);
	return bResult;
}

bool CSarc::ImportFile()
{
	bool bResult = setupCreate();
	do
	{
		if (!bResult)
		{
			break;
		}
		buildAlignList();
		m_fpSarc = FFopen(m_pFileName, "wb");
		if (m_fpSarc == nullptr)
		{
			bResult = false;
			break;
		}
		writeHeader();
		for (n32 i = 0; i < m_SfatHeader.EntryCount; i++)
		{
			int nEntryAlignment = s_nEntryAlignment4;
			if (matchInAlignList(m_mFilePath[i].substr(m_sDirName.size())))
			{
				nEntryAlignment = s_nEntryAlignment128;
			}
			m_SarcHeader.FileSize = static_cast<u32>(FAlign(FFtell(m_fpSarc), nEntryAlignment));
			FSeek(m_fpSarc, m_SarcHeader.FileSize);
			m_vEntry[i].BeginOffset = m_SarcHeader.FileSize - m_SarcHeader.DataOffset;
			FILE* fp = FFopenUnicode(m_mFilePath[i].c_str(), STR("rb"));
			if (fp == nullptr)
			{
				bResult = false;
				break;
			}
			if (m_bVerbose)
			{
				FPrintf(STR("load: %s\n"), m_mFilePath[i].c_str());
			}
			FFseek(fp, 0, SEEK_END);
			n64 nFileSize = FFtell(fp);
			FFseek(fp, 0, SEEK_SET);
			u8* pBuffer = new u8[static_cast<size_t>(nFileSize)];
			fread(pBuffer, 1, static_cast<size_t>(nFileSize), fp);
			fwrite(pBuffer, 1, static_cast<size_t>(nFileSize), m_fpSarc);
			fclose(fp);
			m_SarcHeader.FileSize = static_cast<u32>(FFtell(m_fpSarc));
			m_vEntry[i].EndOffset = m_SarcHeader.FileSize - m_SarcHeader.DataOffset;
		}
		if (bResult)
		{
			writeHeader();
		}
		fclose(m_fpSarc);
	} while (false);
	return bResult;
}

bool CSarc::IsSarcFile(const char* a_pFileName)
{
	FILE* fp = FFopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SSarcHeader sarcHeader;
	fread(&sarcHeader, sizeof(sarcHeader), 1, fp);
	fclose(fp);
	return sarcHeader.Signature == s_uSignatureSarc;
}

const char* CSarc::getExt(char* a_pBuffer)
{
	static char sExt[6] = {};
	memset(sExt, 0, sizeof(sExt));
	if (strcmp(a_pBuffer, "BCH") == 0)
	{
		strcpy(sExt, "bch");
	}
	else if (strcmp(a_pBuffer, "FLAN") == 0)
	{
		strcpy(sExt, "bflan");
	}
	else if (strcmp(a_pBuffer, "FLYT") == 0)
	{
		strcpy(sExt, "bflyt");
	}
	else if (strcmp(a_pBuffer, "FFNT") == 0)
	{
		strcpy(sExt, "bffnt");
	}
	else if (strcmp(a_pBuffer, "FLIM") == 0)
	{
		strcpy(sExt, "bflim");
	}
	else
	{
		memcpy(sExt, a_pBuffer, 4);
		for (int i = 0; i < 4; i++)
		{
			if (!((sExt[i] >= '0' && sExt[i] <= '9') || (sExt[i] >= 'A' && sExt[i] <= 'Z') || (sExt[i] >= 'a' && sExt[i] <= 'z')))
			{
				sExt[i] = 0;
			}
			else if (sExt[i] >= 'A' && sExt[i] <= 'Z')
			{
				sExt[i] = sExt[i] - 'A' + 'a';
			}
		}
		if (sExt[0] == 0)
		{
			strcpy(sExt, "bin");
		}
	}
	return sExt;
}

bool CSarc::setupCreate()
{
	m_fpSarc = FFopen(m_pFileName, "rb");
	if (m_fpSarc == nullptr)
	{
		return false;
	}
	fread(&m_SarcHeader, sizeof(m_SarcHeader), 1, m_fpSarc);
	fread(&m_SfatHeader, sizeof(m_SfatHeader), 1, m_fpSarc);
	m_vEntry.resize(m_SfatHeader.EntryCount);
	if (!m_vEntry.empty())
	{
		fread(&*m_vEntry.begin(), sizeof(SEntry), m_vEntry.size(), m_fpSarc);
	}
	fread(&m_SfntHeader, sizeof(m_SfntHeader), 1, m_fpSarc);
	m_vName.resize(m_SarcHeader.DataOffset - static_cast<u32>(FFtell(m_fpSarc)));
	if (!m_vName.empty())
	{
		fread(&*m_vName.begin(), 1, m_vName.size(), m_fpSarc);
	}
	for (n32 i = 0; i < m_SfatHeader.EntryCount; i++)
	{
		String sFilePath;
		bool bNoName = (m_vEntry[i].NameOffset & 0x01000000) == 0;
		if (bNoName)
		{
			char sBuffer[4] = {};
			const char* pExt = "bin";
			if (m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset > 0x28)
			{
				FFseek(m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].EndOffset - 0x28, SEEK_SET);
				fread(sBuffer, 1, 4, m_fpSarc);
				pExt = getExt(sBuffer);
			}
			if (strcmp(pExt, "bflim") != 0)
			{
				FFseek(m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].BeginOffset, SEEK_SET);
				fread(sBuffer, 1, m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset >= 4 ? 4 : m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset, m_fpSarc);
				pExt = getExt(sBuffer);
			}
			sFilePath = FSAToUnicode(FFormat("%08X.%s", m_vEntry[i].Hash, pExt));
		}
		else
		{
			sFilePath = FSAToUnicode(&*m_vName.begin() + (m_vEntry[i].NameOffset & 0xFFFFFF) * 4);
		}
		sFilePath = m_sDirName + STR("/") + sFilePath;
		m_mFilePath[i] = sFilePath;
	}
	fclose(m_fpSarc);
	return true;
}

void CSarc::buildAlignList()
{
	m_vAlignList.clear();
	String sAlignPath = FGetModuleDir() + STR("/align_sarc.txt");
	FILE* fp = FFopenUnicode(sAlignPath.c_str(), STR("rb"));
	if (fp != nullptr)
	{
		FFseek(fp, 0, SEEK_END);
		u32 nSize = static_cast<u32>(FFtell(fp));
		FFseek(fp, 0, SEEK_SET);
		char* pTxt = new char[nSize + 1];
		fread(pTxt, 1, nSize, fp);
		fclose(fp);
		pTxt[nSize] = '\0';
		string sTxt(pTxt);
		delete[] pTxt;
		vector<string> vTxt = FSSplitOf<string>(sTxt, "\r\n");
		for (auto it = vTxt.begin(); it != vTxt.end(); ++it)
		{
			sTxt = FSTrim(*it);
			if (!sTxt.empty() && !FSStartsWith<string>(sTxt, "//"))
			{
				try
				{
					Regex align(FSAToUnicode(sTxt), regex_constants::ECMAScript | regex_constants::icase);
					m_vAlignList.push_back(align);
				}
				catch (regex_error& e)
				{
					printf("ERROR: %s\n\n", e.what());
				}
			}
		}
	}
}

void CSarc::writeHeader()
{
	FFseek(m_fpSarc, 0, SEEK_SET);
	fwrite(&m_SarcHeader, sizeof(m_SarcHeader), 1, m_fpSarc);
	fwrite(&m_SfatHeader, sizeof(m_SfatHeader), 1, m_fpSarc);
	fwrite(&*m_vEntry.begin(), sizeof(SEntry), m_vEntry.size(), m_fpSarc);
	fwrite(&m_SfntHeader, sizeof(m_SfntHeader), 1, m_fpSarc);
	fwrite(&*m_vName.begin(), 1, m_vName.size(), m_fpSarc);
}

bool CSarc::matchInAlignList(const String& a_sPath)
{
	bool bMatch = false;
	for (auto it = m_vAlignList.begin(); it != m_vAlignList.end(); ++it)
	{
		if (regex_search(a_sPath, *it))
		{
			bMatch = true;
			break;
		}
	}
	return bMatch;
}
