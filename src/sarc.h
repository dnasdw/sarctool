#ifndef SARC_H_
#define SARC_H_

#include "utility.h"

#include MSC_PUSH_PACKED
struct SSarcHeader
{
	u32 Signature;
	u16 HeaderSize;
	u16 ByteOrder;
	u32 FileSize;
	u32 DataOffset;
	u32 Unknown;
} GNUC_PACKED;

struct SSfatHeader
{
	u32 Signature;
	u16 HeaderSize;
	u16 EntryCount;
	u32 Unknown;
} GNUC_PACKED;

struct SEntry
{
	u32 Hash;
	u32 NameOffset;
	u32 BeginOffset;
	u32 EndOffset;
} GNUC_PACKED;

struct SSfntHeader
{
	u32 Signature;
	u16 HeaderSize;
	u16 Reserved;
} GNUC_PACKED;
#include MSC_POP_PACKED

class CSarc
{
public:
	CSarc();
	~CSarc();
	void SetFileName(const char* a_pFileName);
	void SetDirName(const char* a_pRomFsDirName);
	void SetVerbose(bool a_bVerbose);
	bool ExportFile();
	bool ImportFile();
	static bool IsSarcFile(const char* a_pFileName);
	static const u32 s_uSignatureSarc;
	static const u32 s_uSignatureSfat;
	static const u32 s_uSignatureSfnt;
	static const int s_nEntryAlignment4;
	static const int s_nEntryAlignment128;
private:
	static const char* getExt(char* a_pBuffer);
	bool setupCreate();
	void buildAlignList();
	void writeHeader();
	bool matchInAlignList(const String& a_sPath);
	const char* m_pFileName;
	String m_sDirName;
	bool m_bVerbose;
	FILE* m_fpSarc;
	SSarcHeader m_SarcHeader;
	SSfatHeader m_SfatHeader;
	SSfntHeader m_SfntHeader;
	vector<SEntry> m_vEntry;
	vector<char> m_vName;
	unordered_map<n32, String> m_mFilePath;
	vector<Regex> m_vAlignList;
};

#endif	// SARC_H_
