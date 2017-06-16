#ifndef SARC_H_
#define SARC_H_

#include <sdw.h>

#include SDW_MSC_PUSH_PACKED
struct SSarcHeader
{
	u32 Signature;
	u16 HeaderSize;
	u16 ByteOrder;
	u32 FileSize;
	u32 DataOffset;
	u16 Version;
	u16 Reserved;
} SDW_GNUC_PACKED;

struct SSfatHeader
{
	u32 Signature;
	u16 HeaderSize;
	u16 EntryCount;
	u32 HashKey;
} SDW_GNUC_PACKED;

struct SEntry
{
	u32 Hash;
	u32 NameOffset;
	u32 BeginOffset;
	u32 EndOffset;
} SDW_GNUC_PACKED;

struct SSfntHeader
{
	u32 Signature;
	u16 HeaderSize;
	u16 Reserved;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CSarc
{
public:
	enum EEndianness
	{
		kEndianBig,
		kEndianLittle
	};
	struct SCreateEntry
	{
		UString Path;
		string EntryName;
		SEntry Entry;
	};
	CSarc();
	~CSarc();
	void SetFileName(const UString& a_sFileName);
	void SetDirName(const UString& a_sDirName);
	void SetEndianness(EEndianness a_eEndianness);
	void SetAlignment(n32 a_nAlignment);
	void SetHashKey(u32 a_uHashKey);
	void SetCodePage(u32 a_uCodePage);
	void SetCodeName(const string& a_sCodeName);
	void SetNoName(bool a_bNoName);
	void SetNameIsHash(bool a_bNameIsHash);
	void SetVerbose(bool a_bVerbose);
	bool ExtractFile();
	bool CreateFile();
	static bool IsSarcFile(const UString& a_sFileName);
	static const u32 s_uSignatureSarc;
	static const u32 s_uSignatureSfat;
	static const u32 s_uSignatureSfnt;
	static const u32 s_uEntryMax;
	static const u32 s_uFileSizeMax;
	static const u32 s_uEntryNameAlign;
private:
	void setupCreate();
	void buildIgnoreList();
	bool createEntryList();
	bool matchInIgnoreList(const UString& a_sPath) const;
	u32 hash(const string& a_sEntryName) const;
	void createEntryName();
	bool calculateFileOffset();
	n32 getAlignment(const string& a_sEntryName, const UString& a_sPath) const;
	void writeHeader();
	bool writeData();
	bool writeFromFile(const UString& a_sPath, u32 a_uOffset, u32 a_uSize);
	static const char* getExt(const u8* a_pData);
	UString m_sFileName;
	UString m_sDirName;
	EEndianness m_eEndianness;
	n32 m_nAlignment;
	u32 m_uHashKey;
	u32 m_uCodePage;
	string m_sCodeName;
	bool m_bNoName;
	bool m_bNameIsHash;
	bool m_bVerbose;
	FILE* m_fpSarc;
	SSarcHeader m_SarcHeader;
	SSfatHeader m_SfatHeader;
	SSfntHeader m_SfntHeader;
	vector<SEntry> m_vEntry;
	vector<char> m_vName;
	vector<URegex> m_vIgnoreList;
	vector<SCreateEntry> m_vCreateEntry;
};

#endif	// SARC_H_
