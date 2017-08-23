#ifndef BFLIM_H_
#define BFLIM_H_

#include <sdw.h>

#include SDW_MSC_PUSH_PACKED
struct SBflimHeader
{
	u32 Signature;
	u16 ByteOrder;
	u16 HeaderSize;
	u32 Version;
	u32 FileSize;
	u16 DataBlocks;
	u16 Reserved;
} SDW_GNUC_PACKED;

struct SImageBlock
{
	u32 Signature;
	u32 HeaderSize;
	u16 Width;
	u16 Height;
	u16 Alignment;
	u8 Format;
	u8 Flag;
	u32 ImageSize;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

#endif	// BFLIM_H_
