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

union UTextureConfig
{
	u8 Flag;
	struct
	{
		u8 Unknown0 : 2;
		u8 Rotation : 2;
		u8 Unknown4 : 4;
	};
	struct
	{
		u8 TileMode : 5;
		u8 SwizzlePattern : 3;
	};
};

struct SImageBlock
{
	u32 Signature;
	u32 HeaderSize;
	u16 Width;
	u16 Height;
	u16 Alignment;
	u8 Format;
	UTextureConfig TextureConfig;
	u32 ImageSize;
} GNUC_PACKED;
#include SDW_MSC_POP_PACKED

#endif	// BFLIM_H_
