#include "sarc.h"
#include "bflim.h"

bool HashCompare(const CSarc::SCreateEntry& lhs, const CSarc::SCreateEntry& rhs)
{
	return lhs.Entry.Hash < rhs.Entry.Hash;
}

const u32 CSarc::s_uSignatureSarc = SDW_CONVERT_ENDIAN32('SARC');
const u32 CSarc::s_uSignatureSfat = SDW_CONVERT_ENDIAN32('SFAT');
const u32 CSarc::s_uSignatureSfnt = SDW_CONVERT_ENDIAN32('SFNT');
const u32 CSarc::s_uEntryMax = 0x3FFF;
const u32 CSarc::s_uFileSizeMax = UINT32_MAX;
const u32 CSarc::s_uEntryNameAlign = 4;

CSarc::CSarc()
	: m_eEndianness(kEndianLittle)
	, m_nAlignment(4)
	, m_uHashKey(101)
	, m_uCodePage(0)
	, m_sCodeName("UTF-8")
	, m_bNoName(false)
	, m_bNameIsHash(false)
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

void CSarc::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CSarc::SetDirName(const UString& a_sDirName)
{
	m_sDirName = a_sDirName;
}

void CSarc::SetEndianness(EEndianness a_eEndianness)
{
	m_eEndianness = a_eEndianness;
}

void CSarc::SetAlignment(n32 a_nAlignment)
{
	m_nAlignment = a_nAlignment;
}

void CSarc::SetUniqueAlignment(const map<n32, vector<URegex>>& a_mUniqueAlignment)
{
	m_mUniqueAlignment = a_mUniqueAlignment;
}

void CSarc::SetDataOffsetAlignment(n32 a_nDataOffsetAlignment)
{
	m_nDataOffsetAlignment = a_nDataOffsetAlignment;
}

void CSarc::SetHashKey(u32 a_uHashKey)
{
	m_uHashKey = a_uHashKey;
}

void CSarc::SetCodePage(u32 a_uCodePage)
{
	m_uCodePage = a_uCodePage;
}

void CSarc::SetCodeName(const string& a_sCodeName)
{
	m_sCodeName = a_sCodeName;
}

void CSarc::SetNoName(bool a_bNoName)
{
	m_bNoName = a_bNoName;
}

void CSarc::SetNameIsHash(bool a_bNameIsHash)
{
	m_bNameIsHash = a_bNameIsHash;
}

void CSarc::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CSarc::ExtractFile()
{
	bool bResult = true;
	m_fpSarc = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (m_fpSarc == nullptr)
	{
		return false;
	}
	fread(&m_SarcHeader, sizeof(m_SarcHeader), 1, m_fpSarc);
	if (m_SarcHeader.ByteOrder == 0xFFFE)
	{
		m_eEndianness = kEndianBig;
	}
	if (m_eEndianness == kEndianBig)
	{
		m_SarcHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SarcHeader.HeaderSize);
		m_SarcHeader.ByteOrder = SDW_CONVERT_ENDIAN16(m_SarcHeader.ByteOrder);
		m_SarcHeader.FileSize = SDW_CONVERT_ENDIAN32(m_SarcHeader.FileSize);
		m_SarcHeader.DataOffset = SDW_CONVERT_ENDIAN32(m_SarcHeader.DataOffset);
		m_SarcHeader.Version = SDW_CONVERT_ENDIAN16(m_SarcHeader.Version);
		m_SarcHeader.Reserved = SDW_CONVERT_ENDIAN16(m_SarcHeader.Reserved);
	}
	fread(&m_SfatHeader, sizeof(m_SfatHeader), 1, m_fpSarc);
	if (m_eEndianness == kEndianBig)
	{
		m_SfatHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SfatHeader.HeaderSize);
		m_SfatHeader.EntryCount = SDW_CONVERT_ENDIAN16(m_SfatHeader.EntryCount);
		m_SfatHeader.HashKey = SDW_CONVERT_ENDIAN32(m_SfatHeader.HashKey);
	}
	m_vEntry.resize(m_SfatHeader.EntryCount);
	if (!m_vEntry.empty())
	{
		fread(&*m_vEntry.begin(), sizeof(SEntry), m_vEntry.size(), m_fpSarc);
	}
	if (m_eEndianness == kEndianBig)
	{
		for (vector<SEntry>::iterator it = m_vEntry.begin(); it != m_vEntry.end(); ++it)
		{
			SEntry& entry = *it;
			entry.Hash = SDW_CONVERT_ENDIAN32(entry.Hash);
			entry.NameOffset = SDW_CONVERT_ENDIAN32(entry.NameOffset);
			entry.BeginOffset = SDW_CONVERT_ENDIAN32(entry.BeginOffset);
			entry.EndOffset = SDW_CONVERT_ENDIAN32(entry.EndOffset);
		}
	}
	fread(&m_SfntHeader, sizeof(m_SfntHeader), 1, m_fpSarc);
	if (m_eEndianness == kEndianBig)
	{
		m_SfntHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SfntHeader.HeaderSize);
		m_SfntHeader.Reserved = SDW_CONVERT_ENDIAN16(m_SfntHeader.Reserved);
	}
	m_vName.resize(m_SarcHeader.DataOffset - static_cast<u32>(Ftell(m_fpSarc)));
	if (!m_vName.empty())
	{
		fread(&*m_vName.begin(), 1, m_vName.size(), m_fpSarc);
	}
	UMakeDir(m_sDirName.c_str());
	for (n32 i = 0; i < m_SfatHeader.EntryCount; i++)
	{
		UString sPath;
		bool bNoName = m_vEntry[i].NameOffset == 0;
		if (bNoName)
		{
			u8 uBuffer[4] = {};
			const char* pExt = "bin";
			if (m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset > (sizeof(SBflimHeader) + sizeof(SImageBlock)))
			{
				Fseek(m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].EndOffset - (sizeof(SBflimHeader) + sizeof(SImageBlock)), SEEK_SET);
				fread(uBuffer, 1, 4, m_fpSarc);
				pExt = getExt(uBuffer);
			}
			if (strcmp(pExt, "bflim") != 0)
			{
				Fseek(m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].BeginOffset, SEEK_SET);
				fread(uBuffer, 1, m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset >= 4 ? 4 : m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset, m_fpSarc);
				pExt = getExt(uBuffer);
			}
			sPath = AToU(Format("%08X.%s", m_vEntry[i].Hash, pExt));
		}
		else
		{
			sPath = XToU(&*m_vName.begin() + (m_vEntry[i].NameOffset & 0xFFFFFF) * s_uEntryNameAlign, m_uCodePage, m_sCodeName.c_str());
		}
		vector<UString> vDirPath = SplitOf(sPath, USTR("/\\"));
		UString sDirName = m_sDirName;
		for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
		{
			sDirName += USTR("/") + vDirPath[j];
			UMakeDir(sDirName.c_str());
		}
		sPath = sDirName + USTR("/") + vDirPath.back();
		FILE* fp = UFopen(sPath.c_str(), USTR("wb"));
		if (fp == nullptr)
		{
			bResult = false;
			continue;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("save: %") PRIUS USTR("\n"), sPath.c_str());
		}
		CopyFile(fp, m_fpSarc, m_SarcHeader.DataOffset + m_vEntry[i].BeginOffset, m_vEntry[i].EndOffset - m_vEntry[i].BeginOffset);
		fclose(fp);
	}
	fclose(m_fpSarc);
	return bResult;
}

bool CSarc::CreateFile()
{
	bool bResult = true;
	setupCreate();
	buildIgnoreList();
	if (!createEntryList())
	{
		return false;
	}
	createEntryName();
	if (!calculateFileOffset())
	{
		return false;
	}
	m_fpSarc = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (m_fpSarc == nullptr)
	{
		return false;
	}
	writeHeader();
	if (!writeData())
	{
		bResult = false;
	}
	fclose(m_fpSarc);
	return bResult;
}

bool CSarc::IsSarcFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	SSarcHeader sarcHeader;
	fread(&sarcHeader, sizeof(sarcHeader), 1, fp);
	fclose(fp);
	return sarcHeader.Signature == s_uSignatureSarc;
}

void CSarc::setupCreate()
{
	if (m_bNameIsHash)
	{
		m_bNoName = true;
	}
	m_SarcHeader.Signature = s_uSignatureSarc;
	m_SarcHeader.HeaderSize = sizeof(m_SarcHeader);
	m_SarcHeader.ByteOrder = 0xFEFF;
	m_SarcHeader.Version = 0x0100;
	m_SarcHeader.Reserved = 0;
	m_SfatHeader.Signature = s_uSignatureSfat;
	m_SfatHeader.HeaderSize = sizeof(m_SfatHeader);
	m_SfatHeader.HashKey = m_uHashKey;
	m_SfntHeader.Signature = s_uSignatureSfnt;
	m_SfntHeader.HeaderSize = sizeof(m_SfntHeader);
	m_SfntHeader.Reserved = 0;
}

void CSarc::buildIgnoreList()
{
	m_vIgnoreList.clear();
	UString sIgnorePath = UGetModuleDirName() + USTR("/ignore_sarctool.txt");
	FILE* fp = UFopen(sIgnorePath.c_str(), USTR("rb"));
	if (fp != nullptr)
	{
		Fseek(fp, 0, SEEK_END);
		u32 uSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		char* pTxt = new char[uSize + 1];
		fread(pTxt, 1, uSize, fp);
		fclose(fp);
		pTxt[uSize] = '\0';
		string sTxt(pTxt);
		delete[] pTxt;
		vector<string> vTxt = SplitOf(sTxt, "\r\n");
		for (vector<string>::const_iterator it = vTxt.begin(); it != vTxt.end(); ++it)
		{
			sTxt = Trim(*it);
			if (!sTxt.empty() && !StartWith(sTxt, "//"))
			{
				try
				{
					URegex black(AToU(sTxt), regex_constants::ECMAScript | regex_constants::icase);
					m_vIgnoreList.push_back(black);
				}
				catch (regex_error& e)
				{
					UPrintf(USTR("ERROR: %") PRIUS USTR("\n\n"), AToU(e.what()).c_str());
				}
			}
		}
	}
}

bool CSarc::createEntryList()
{
	bool bResult = true;
	queue<UString> qDir;
	map<UString, UString> mFile;
	qDir.push(m_sDirName);
	while (!qDir.empty())
	{
		UString& sParent = qDir.front();
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
		WIN32_FIND_DATAW ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		wstring sPattern = sParent + L"/*";
		hFind = FindFirstFileW(sPattern.c_str(), &ffd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (matchInIgnoreList(sParent.substr(m_sDirName.size()) + L"/" + ffd.cFileName))
				{
					continue;
				}
				if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					wstring sFileName = sParent + L"/" + ffd.cFileName;
					wstring sFileNameUpper = sFileName;
					transform(sFileNameUpper.begin(), sFileNameUpper.end(), sFileNameUpper.begin(), ::toupper);
					mFile.insert(make_pair(sFileNameUpper, sFileName));
				}
				else if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
				{
					wstring sDir = sParent + L"/" + ffd.cFileName;
					qDir.push(sDir);
				}
			} while (FindNextFileW(hFind, &ffd) != 0);
		}
#else
		DIR* pDir = opendir(sParent.c_str());
		if (pDir != nullptr)
		{
			dirent* pDirent = nullptr;
			while ((pDirent = readdir(pDir)) != nullptr)
			{
				if (matchInIgnoreList(sParent.substr(m_sDirName.size()) + "/" + pDirent->d_name))
				{
					continue;
				}
				if (pDirent->d_type == DT_REG)
				{
					string sFileName = sParent + "/" + pDirent->d_name;
					string sFileNameUpper = sFileName;
					transform(sFileNameUpper.begin(), sFileNameUpper.end(), sFileNameUpper.begin(), ::toupper);
					mFile.insert(make_pair(sFileNameUpper, sFileName));
				}
				else if (pDirent->d_type == DT_DIR && strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0)
				{
					string sDir = sParent + "/" + pDirent->d_name;
					qDir.push(sDir);
				}
			}
			closedir(pDir);
		}
#endif
		qDir.pop();
	}
	if (mFile.size() > s_uEntryMax)
	{
		bResult = false;
	}
	m_vCreateEntry.reserve(mFile.size());
	set<u32> sHash;
	for (map<UString, UString>::const_iterator it = mFile.begin(); it != mFile.end(); ++it)
	{
		m_vCreateEntry.push_back(SCreateEntry());
		SCreateEntry& currentEntry = m_vCreateEntry.back();
		currentEntry.Path = it->second;
		currentEntry.EntryName = UToX(currentEntry.Path.substr(m_sDirName.size() + 1), m_uCodePage, m_sCodeName.c_str());
		if (m_bNameIsHash)
		{
			currentEntry.Entry.Hash = SToU32(currentEntry.EntryName, 16);
		}
		else
		{
			currentEntry.Entry.Hash = hash(currentEntry.EntryName);
		}
		currentEntry.Entry.NameOffset = 0;
		currentEntry.Entry.BeginOffset = 0;
		n64 nFileSize = 0;
		if (!UGetFileSize(currentEntry.Path.c_str(), nFileSize))
		{
			bResult = false;
			UPrintf(USTR("ERROR: %") PRIUS USTR(" stat error\n\n"), currentEntry.Path.c_str());
		}
		else
		{
			currentEntry.Entry.EndOffset = static_cast<n32>(nFileSize);
		}
		sHash.insert(currentEntry.Entry.Hash);
	}
	if (sHash.size() != m_vCreateEntry.size())
	{
		if (m_bNameIsHash)
		{
			bResult = false;
		}
		m_bNoName = false;
	}
	sort(m_vCreateEntry.begin(), m_vCreateEntry.end(), HashCompare);
	m_SfatHeader.EntryCount = static_cast<u16>(m_vCreateEntry.size());
	return bResult;
}

bool CSarc::matchInIgnoreList(const UString& a_sPath) const
{
	bool bMatch = false;
	for (vector<URegex>::const_iterator it = m_vIgnoreList.begin(); it != m_vIgnoreList.end(); ++it)
	{
		if (regex_search(a_sPath, *it))
		{
			bMatch = true;
			break;
		}
	}
	return bMatch;
}

u32 CSarc::hash(const string& a_sEntryName) const
{
	u32 uHash = 0;
	for (n32 i = 0; i < static_cast<n32>(a_sEntryName.size()); i++)
	{
		uHash = uHash * m_uHashKey + a_sEntryName[i];
	}
	return uHash;
}

void CSarc::createEntryName()
{
	if (!m_bNoName)
	{
		for (u32 i = 0; i < static_cast<u32>(m_vCreateEntry.size()); i++)
		{
			SCreateEntry& currentEntry = m_vCreateEntry[i];
			m_vName.resize(static_cast<u32>(Align(m_vName.size(), s_uEntryNameAlign)));
			currentEntry.Entry.NameOffset = static_cast<u32>(m_vName.size() / s_uEntryNameAlign);
			m_vName.resize(m_vName.size() + currentEntry.EntryName.size() + 1);
			if (currentEntry.EntryName.size() != 0)
			{
				memcpy(&*m_vName.begin() + currentEntry.Entry.NameOffset * s_uEntryNameAlign, &*currentEntry.EntryName.begin(), currentEntry.EntryName.size());
			}
			u32 uIndexTop8 = 1;
			if (i != 0)
			{
				SCreateEntry& prevEntry = m_vCreateEntry[i - 1];
				if (currentEntry.Entry.Hash == prevEntry.Entry.Hash)
				{
					uIndexTop8 = ((prevEntry.Entry.NameOffset >> 24 & 0xFF) + 1) & 0xFF;
				}
			}
			currentEntry.Entry.NameOffset |= uIndexTop8 << 24;
		}
	}
	m_SarcHeader.FileSize = static_cast<u32>(Align(sizeof(m_SarcHeader) + sizeof(m_SfatHeader) + m_vCreateEntry.size() * sizeof(SEntry) + sizeof(m_SfntHeader) + m_vName.size(), s_uEntryNameAlign));
}

bool CSarc::calculateFileOffset()
{
	n32 nAlignment = m_nDataOffsetAlignment;
	if (m_nDataOffsetAlignment == 0)
	{
		n32 nAlignmentMax = m_nAlignment;
		for (vector<SCreateEntry>::iterator it = m_vCreateEntry.begin(); it != m_vCreateEntry.end(); ++it)
		{
			SCreateEntry& currentEntry = *it;
			n32 nEntryAlignment = getAlignment(currentEntry.EntryName, currentEntry.Path);
			if (nEntryAlignment > nAlignmentMax)
			{
				nAlignmentMax = nEntryAlignment;
			}
		}
		nAlignment = nAlignmentMax;
	}
	m_SarcHeader.FileSize = static_cast<u32>(Align(m_SarcHeader.FileSize, nAlignment));
	for (vector<SCreateEntry>::iterator it = m_vCreateEntry.begin(); it != m_vCreateEntry.end(); ++it)
	{
		SCreateEntry& currentEntry = *it;
		n32 nEntryAlignment = getAlignment(currentEntry.EntryName, currentEntry.Path);
		n64 nFileSize = Align(m_SarcHeader.FileSize, nEntryAlignment);
		if (nFileSize > s_uFileSizeMax)
		{
			return false;
		}
		currentEntry.Entry.BeginOffset = static_cast<u32>(nFileSize);
		nFileSize += currentEntry.Entry.EndOffset;
		if (nFileSize > s_uFileSizeMax)
		{
			return false;
		}
		if (m_SarcHeader.DataOffset == 0)
		{
			m_SarcHeader.DataOffset = currentEntry.Entry.BeginOffset;
		}
		currentEntry.Entry.BeginOffset -= m_SarcHeader.DataOffset;
		currentEntry.Entry.EndOffset += currentEntry.Entry.BeginOffset;
		m_SarcHeader.FileSize = m_SarcHeader.DataOffset + currentEntry.Entry.EndOffset;
	}
	return true;
}

n32 CSarc::getAlignment(const string& a_sEntryName, const UString& a_sPath) const
{
	if (EndWith(a_sEntryName, ".bffnt"))
	{
		switch (m_eEndianness)
		{
		case kEndianBig:
			return 0x2000;
		case kEndianLittle:
			return 0x80;
		}
	}
	else if (EndWith(a_sEntryName, ".bflim"))
	{
		do
		{
			FILE* fp = UFopen(a_sPath.c_str(), USTR("rb"));
			if (fp != nullptr)
			{
				SBflimHeader bflimHeader;
				SImageBlock imageBlock;
				Fseek(fp, 0, SEEK_END);
				u32 uBflimSize = ftell(fp);
				if (uBflimSize < sizeof(SBflimHeader) + sizeof(SImageBlock))
				{
					fclose(fp);
					break;
				}
				Fseek(fp, uBflimSize - (sizeof(SBflimHeader) + sizeof(SImageBlock)), SEEK_SET);
				fread(&bflimHeader, sizeof(bflimHeader), 1, fp);
				fread(&imageBlock, sizeof(imageBlock), 1, fp);
				fclose(fp);
				if (bflimHeader.ByteOrder == 0xFFFE)
				{
					imageBlock.Alignment = SDW_CONVERT_ENDIAN16(imageBlock.Alignment);
				}
				return imageBlock.Alignment;
			}
		} while (false);
		switch (m_eEndianness)
		{
		case kEndianBig:
			return 0x800;
		case kEndianLittle:
			return 0x80;
		}
	}
	for (map<n32, vector<URegex>>::const_reverse_iterator it = m_mUniqueAlignment.rbegin(); it != m_mUniqueAlignment.rend(); ++it)
	{
		n32 nAlignment = it->first;
		const vector<URegex>& vRegex = it->second;
		for (vector<URegex>::const_iterator itRegex = vRegex.begin(); itRegex != vRegex.end(); ++itRegex)
		{
			if (regex_search(a_sPath, *itRegex))
			{
				return nAlignment;
			}
		}
	}
	return m_nAlignment;
}

void CSarc::writeHeader()
{
	Fseek(m_fpSarc, 0, SEEK_SET);
	if (m_eEndianness == kEndianBig)
	{
		m_SarcHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SarcHeader.HeaderSize);
		m_SarcHeader.ByteOrder = SDW_CONVERT_ENDIAN16(m_SarcHeader.ByteOrder);
		m_SarcHeader.FileSize = SDW_CONVERT_ENDIAN32(m_SarcHeader.FileSize);
		m_SarcHeader.DataOffset = SDW_CONVERT_ENDIAN32(m_SarcHeader.DataOffset);
		m_SarcHeader.Version = SDW_CONVERT_ENDIAN16(m_SarcHeader.Version);
		m_SarcHeader.Reserved = SDW_CONVERT_ENDIAN16(m_SarcHeader.Reserved);
	}
	fwrite(&m_SarcHeader, sizeof(m_SarcHeader), 1, m_fpSarc);
	if (m_eEndianness == kEndianBig)
	{
		m_SarcHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SarcHeader.HeaderSize);
		m_SarcHeader.ByteOrder = SDW_CONVERT_ENDIAN16(m_SarcHeader.ByteOrder);
		m_SarcHeader.FileSize = SDW_CONVERT_ENDIAN32(m_SarcHeader.FileSize);
		m_SarcHeader.DataOffset = SDW_CONVERT_ENDIAN32(m_SarcHeader.DataOffset);
		m_SarcHeader.Version = SDW_CONVERT_ENDIAN16(m_SarcHeader.Version);
		m_SarcHeader.Reserved = SDW_CONVERT_ENDIAN16(m_SarcHeader.Reserved);
	}
	if (m_eEndianness == kEndianBig)
	{
		m_SfatHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SfatHeader.HeaderSize);
		m_SfatHeader.EntryCount = SDW_CONVERT_ENDIAN16(m_SfatHeader.EntryCount);
		m_SfatHeader.HashKey = SDW_CONVERT_ENDIAN32(m_SfatHeader.HashKey);
	}
	fwrite(&m_SfatHeader, sizeof(m_SfatHeader), 1, m_fpSarc);
	if (m_eEndianness == kEndianBig)
	{
		m_SfatHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SfatHeader.HeaderSize);
		m_SfatHeader.EntryCount = SDW_CONVERT_ENDIAN16(m_SfatHeader.EntryCount);
		m_SfatHeader.HashKey = SDW_CONVERT_ENDIAN32(m_SfatHeader.HashKey);
	}
	for (vector<SCreateEntry>::iterator it = m_vCreateEntry.begin(); it != m_vCreateEntry.end(); ++it)
	{
		SCreateEntry& currentEntry = *it;
		SEntry& entry = currentEntry.Entry;
		if (m_eEndianness == kEndianBig)
		{
			entry.Hash = SDW_CONVERT_ENDIAN32(entry.Hash);
			entry.NameOffset = SDW_CONVERT_ENDIAN32(entry.NameOffset);
			entry.BeginOffset = SDW_CONVERT_ENDIAN32(entry.BeginOffset);
			entry.EndOffset = SDW_CONVERT_ENDIAN32(entry.EndOffset);
		}
		fwrite(&entry, sizeof(entry), 1, m_fpSarc);
		if (m_eEndianness == kEndianBig)
		{
			entry.Hash = SDW_CONVERT_ENDIAN32(entry.Hash);
			entry.NameOffset = SDW_CONVERT_ENDIAN32(entry.NameOffset);
			entry.BeginOffset = SDW_CONVERT_ENDIAN32(entry.BeginOffset);
			entry.EndOffset = SDW_CONVERT_ENDIAN32(entry.EndOffset);
		}
	}
	if (m_eEndianness == kEndianBig)
	{
		m_SfntHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SfntHeader.HeaderSize);
		m_SfntHeader.Reserved = SDW_CONVERT_ENDIAN16(m_SfntHeader.Reserved);
	}
	fwrite(&m_SfntHeader, sizeof(m_SfntHeader), 1, m_fpSarc);
	if (m_eEndianness == kEndianBig)
	{
		m_SfntHeader.HeaderSize = SDW_CONVERT_ENDIAN16(m_SfntHeader.HeaderSize);
		m_SfntHeader.Reserved = SDW_CONVERT_ENDIAN16(m_SfntHeader.Reserved);
	}
	if (!m_vName.empty())
	{
		fwrite(&*m_vName.begin(), 1, m_vName.size(), m_fpSarc);
	}
}

bool CSarc::writeData()
{
	bool bResult = true;
	for (vector<SCreateEntry>::const_iterator it = m_vCreateEntry.begin(); it != m_vCreateEntry.end(); ++it)
	{
		const SCreateEntry& currentEntry = *it;
		const SEntry& entry = currentEntry.Entry;
		if (!writeFromFile(currentEntry.Path, m_SarcHeader.DataOffset + entry.BeginOffset, entry.EndOffset - entry.BeginOffset))
		{
			bResult = false;
		}
	}
	return bResult;
}

bool CSarc::writeFromFile(const UString& a_sPath, u32 a_uOffset, u32 a_uSize)
{
	FILE* fp = UFopen(a_sPath.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), a_sPath.c_str());
	}
	Fseek(m_fpSarc, a_uOffset, SEEK_SET);
	CopyFile(m_fpSarc, fp, 0, a_uSize);
	fclose(fp);
	return true;
}

const char* CSarc::getExt(const u8* a_pData)
{
	u32 uExtU32 = *reinterpret_cast<const u32*>(a_pData);
	if (uExtU32 == SDW_CONVERT_ENDIAN32('BCH\0'))
	{
		return "bch";
	}
	else if (uExtU32 == SDW_CONVERT_ENDIAN32('FFNA'))
	{
		return "bffna";
	}
	else if (uExtU32 == SDW_CONVERT_ENDIAN32('FFNT'))
	{
		return "bffnt";
	}
	else if (uExtU32 == SDW_CONVERT_ENDIAN32('FLAN'))
	{
		return "bflan";
	}
	else if (uExtU32 == SDW_CONVERT_ENDIAN32('FLIM'))
	{
		return "bflim";
	}
	else if (uExtU32 == SDW_CONVERT_ENDIAN32('FLYT'))
	{
		return "bflyt";
	}
	static u8 c_szExt[5] = {};
	memcpy(c_szExt, a_pData, 4);
	for (n32 i = 0; i < 4; i++)
	{
		if (c_szExt[i] == 0xFF)
		{
			c_szExt[i] = 0;
		}
		else if (c_szExt[i] >= 'A' && c_szExt[i] <= 'Z')
		{
			c_szExt[i] = c_szExt[i] - 'A' + 'a';
		}
		else if (c_szExt[i] != 0 && (c_szExt[i] < '0' || (c_szExt[i] > '9' && c_szExt[i] < 'a') || c_szExt[i] > 'z'))
		{
			return "bin";
		}
	}
	if (c_szExt[0] == 0)
	{
		return "bin";
	}
	return reinterpret_cast<char*>(c_szExt);
}
