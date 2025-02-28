// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Importer/ImageImporter.h>
#include <AnKi/Gr/Common.h>
#include <AnKi/Resource/Stb.h>
#include <AnKi/Util/Process.h>
#include <AnKi/Util/File.h>
#include <AnKi/Util/Filesystem.h>

namespace anki {

namespace {

class SurfaceOrVolumeData
{
public:
	DynamicArrayAuto<U8, PtrSize> m_pixels;
	DynamicArrayAuto<U8, PtrSize> m_s3tcPixels;
	DynamicArrayAuto<U8, PtrSize> m_astcPixels;

	SurfaceOrVolumeData(GenericMemoryPoolAllocator<U8> alloc)
		: m_pixels(alloc)
		, m_s3tcPixels(alloc)
		, m_astcPixels(alloc)
	{
	}
};

class Mipmap
{
public:
	DynamicArrayAuto<SurfaceOrVolumeData> m_surfacesOrVolume;

	Mipmap(GenericMemoryPoolAllocator<U8> alloc)
		: m_surfacesOrVolume(alloc)
	{
	}
};

/// Image importer context.
class ImageImporterContext
{
public:
	DynamicArrayAuto<Mipmap> m_mipmaps;
	U32 m_width = 0;
	U32 m_height = 0;
	U32 m_depth = 0;
	U32 m_faceCount = 0;
	U32 m_layerCount = 0;
	U32 m_channelCount = 0;
	U32 m_pixelSize = 0;

	ImageImporterContext(GenericMemoryPoolAllocator<U8> alloc)
		: m_mipmaps(alloc)
	{
	}

	GenericMemoryPoolAllocator<U8> getAllocator() const
	{
		return m_mipmaps.getAllocator();
	}
};

class DdsPixelFormat
{
public:
	U32 m_dwSize;
	U32 m_dwFlags;
	Array<char, 4> m_dwFourCC;
	U32 m_dwRGBBitCount;
	U32 m_dwRBitMask;
	U32 m_dwGBitMask;
	U32 m_dwBBitMask;
	U32 m_dwABitMask;
};

class DdsHeader
{
public:
	Array<U8, 4> m_magic;
	U32 m_dwSize;
	U32 m_dwFlags;
	U32 m_dwHeight;
	U32 m_dwWidth;
	U32 m_dwPitchOrLinearSize;
	U32 m_dwDepth;
	U32 m_dwMipMapCount;
	Array<U32, 11> m_dwReserved1;
	DdsPixelFormat m_ddspf;
	U32 m_dwCaps;
	U32 m_dwCaps2;
	U32 m_dwCaps3;
	U32 m_dwCaps4;
	U32 m_dwReserved2;
};

class AstcHeader
{
public:
	Array<U8, 4> m_magic;
	U8 m_blockX;
	U8 m_blockY;
	U8 m_blockZ;
	Array<U8, 3> m_dimX;
	Array<U8, 3> m_dimY;
	Array<U8, 3> m_dimZ;
};

/// Simple class to delete a file when it goes out of scope.
class CleanupFile
{
public:
	StringAuto m_fileToDelete;

	CleanupFile(GenericMemoryPoolAllocator<U8> alloc, CString filename)
		: m_fileToDelete(alloc, filename)
	{
	}

	~CleanupFile()
	{
		if(!m_fileToDelete.isEmpty())
		{
			const Error err = removeFile(m_fileToDelete);
			if(!err)
			{
				ANKI_IMPORTER_LOGV("Deleted %s", m_fileToDelete.cstr());
			}
		}
	}
};

} // namespace

static ANKI_USE_RESULT Error checkConfig(const ImageImporterConfig& config)
{
#define ANKI_CFG_ASSERT(x, message) \
	do \
	{ \
		if(!(x)) \
		{ \
			ANKI_IMPORTER_LOGE(message); \
			return Error::USER_DATA; \
		} \
	} while(false)

	// Filenames
	ANKI_CFG_ASSERT(config.m_outFilename.getLength() > 0, "Empty output filename");

	for(CString in : config.m_inputFilenames)
	{
		ANKI_CFG_ASSERT(in.getLength() > 0, "Empty input filename");
	}

	// Type
	ANKI_CFG_ASSERT(config.m_type != ImageBinaryType::NONE, "Wrong image type");
	ANKI_CFG_ASSERT(config.m_inputFilenames.getSize() == 1 || config.m_type != ImageBinaryType::_2D,
					"2D images require only one input image");
	ANKI_CFG_ASSERT(config.m_inputFilenames.getSize() != 1 || config.m_type != ImageBinaryType::_2D_ARRAY,
					"2D array images require more than one input image");
	ANKI_CFG_ASSERT(config.m_inputFilenames.getSize() != 1 || config.m_type != ImageBinaryType::_3D,
					"3D images require more than one input image");
	ANKI_CFG_ASSERT(config.m_inputFilenames.getSize() != 6 || config.m_type != ImageBinaryType::CUBE,
					"Cube images require 6 input images");

	// Compressions
	ANKI_CFG_ASSERT(config.m_compressions != ImageBinaryDataCompression::NONE, "Missing output compressions");
	ANKI_CFG_ASSERT(config.m_compressions == ImageBinaryDataCompression::RAW || config.m_type != ImageBinaryType::_3D,
					"Can't compress 3D textures");

	// ASTC
	if(!!(config.m_compressions & ImageBinaryDataCompression::ASTC))
	{
		ANKI_CFG_ASSERT(config.m_astcBlockSize == UVec2(4u) || config.m_astcBlockSize == UVec2(8u),
						"Incorrect ASTC block sizes");
	}

	// Mip size
	ANKI_CFG_ASSERT(config.m_minMipmapDimension >= 4, "Mimpap min dimension can be less than 4");

#undef ANKI_CFG_ASSERT
	return Error::NONE;
}

static ANKI_USE_RESULT Error checkInputImages(const ImageImporterConfig& config, U32& width, U32& height,
											  U32& channelCount)
{
	width = 0;
	height = 0;
	channelCount = 0;

	for(U32 i = 0; i < config.m_inputFilenames.getSize(); ++i)
	{
		I32 iwidth, iheight, ichannelCount;
		const I ok = stbi_info(config.m_inputFilenames[i].cstr(), &iwidth, &iheight, &ichannelCount);
		if(!ok)
		{
			ANKI_IMPORTER_LOGE("STB failed to load file: %s", config.m_inputFilenames[i].cstr());
			return Error::FUNCTION_FAILED;
		}

		if(width == 0)
		{
			width = U32(iwidth);
			height = U32(iheight);
			channelCount = U32(ichannelCount);
		}
		else if(width != U32(iwidth) || height != U32(iheight) || channelCount != U32(ichannelCount))
		{
			ANKI_IMPORTER_LOGE("Input image doesn't match previous input images: %s",
							   config.m_inputFilenames[i].cstr());
			return Error::USER_DATA;
		}
	}

	ANKI_ASSERT(width > 0 && height > 0 && channelCount > 0);
	if(!isPowerOfTwo(width) || !isPowerOfTwo(height))
	{
		ANKI_IMPORTER_LOGE("Only power of two images are accepted");
		return Error::USER_DATA;
	}

	return Error::NONE;
}

static ANKI_USE_RESULT Error loadFirstMipmap(const ImageImporterConfig& config, ImageImporterContext& ctx)
{
	GenericMemoryPoolAllocator<U8> alloc = ctx.getAllocator();

	ctx.m_mipmaps.emplaceBack(alloc);
	Mipmap& mip0 = ctx.m_mipmaps[0];

	if(ctx.m_depth > 1)
	{
		mip0.m_surfacesOrVolume.create(1, alloc);
		mip0.m_surfacesOrVolume[0].m_pixels.create(ctx.m_pixelSize * ctx.m_width * ctx.m_height * ctx.m_depth);
	}
	else
	{
		mip0.m_surfacesOrVolume.create(ctx.m_faceCount * ctx.m_layerCount, alloc);

		for(U32 f = 0; f < ctx.m_faceCount; ++f)
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				mip0.m_surfacesOrVolume[l * ctx.m_faceCount + f].m_pixels.create(ctx.m_pixelSize * ctx.m_width
																				 * ctx.m_height);
			}
		}
	}

	for(U32 i = 0; i < config.m_inputFilenames.getSize(); ++i)
	{
		I32 width, height, c;
		stbi_set_flip_vertically_on_load_thread(true);
		void* data = stbi_load(config.m_inputFilenames[i].cstr(), &width, &height, &c, ctx.m_channelCount);
		ANKI_ASSERT(U32(c) == ctx.m_channelCount);
		if(!data)
		{
			ANKI_IMPORTER_LOGE("STB load failed: %s", config.m_inputFilenames[i].cstr());
			return Error::FUNCTION_FAILED;
		}

		const PtrSize dataSize = PtrSize(ctx.m_width) * ctx.m_height * ctx.m_pixelSize;

		if(ctx.m_depth > 1)
		{
			memcpy(mip0.m_surfacesOrVolume.getBegin() + i * dataSize, data, dataSize);
		}
		else
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					memcpy(mip0.m_surfacesOrVolume[l * ctx.m_faceCount + f].m_pixels.getBegin(), data, dataSize);
				}
			}
		}

		stbi_image_free(data);
	}

	return Error::NONE;
}

template<U32 TCHANNEL_COUNT>
static void generateSurfaceMipmap(ConstWeakArray<U8, PtrSize> inBuffer, U32 inWidth, U32 inHeight,
								  WeakArray<U8, PtrSize> outBuffer)
{
	using UVecType = typename std::conditional_t<TCHANNEL_COUNT == 3, U8Vec3, U8Vec4>;
	using FVecType = typename std::conditional_t<TCHANNEL_COUNT == 3, Vec3, Vec4>;

	const ConstWeakArray<UVecType, PtrSize> inPixels(reinterpret_cast<const UVecType*>(&inBuffer[0]),
													 inBuffer.getSizeInBytes() / sizeof(UVecType));
	WeakArray<UVecType, PtrSize> outPixels(reinterpret_cast<UVecType*>(&outBuffer[0]),
										   outBuffer.getSizeInBytes() / sizeof(UVecType));

	const U32 outWidth = inWidth >> 1;
	const U32 outHeight = inHeight >> 1;

	for(U32 h = 0; h < outHeight; ++h)
	{
		for(U32 w = 0; w < outWidth; ++w)
		{
			// Gather input
			FVecType average(0.0f);
			for(U32 y = 0; y < 2; ++y)
			{
				for(U32 x = 0; x < 2; ++x)
				{
					const U32 idx = (h * 2 + y) * inWidth + (w * 2 + x);
					const UVecType inPixel = inPixels[idx];
					for(U32 c = 0; c < TCHANNEL_COUNT; ++c)
					{
						average[c] += F32(inPixel[c]) / 255.0f * 0.25f;
					}
				}
			}

			UVecType uaverage;
			for(U32 c = 0; c < TCHANNEL_COUNT; ++c)
			{
				uaverage[c] = U8(average[c] * 255.0f);
			}

			// Store
			const U32 idx = h * outWidth + w;
			outPixels[idx] = uaverage;
		}
	}
}

static ANKI_USE_RESULT Error compressS3tc(GenericMemoryPoolAllocator<U8> alloc, CString tempDirectory,
										  CString compressonatorPath, ConstWeakArray<U8, PtrSize> inPixels, U32 inWidth,
										  U32 inHeight, U32 channelCount, WeakArray<U8, PtrSize> outPixels)
{
	ANKI_ASSERT(inPixels.getSizeInBytes() == PtrSize(inWidth) * inHeight * channelCount);
	ANKI_ASSERT(inWidth > 0 && isPowerOfTwo(inWidth) && inHeight > 0 && isPowerOfTwo(inHeight));
	ANKI_ASSERT(outPixels.getSizeInBytes() == PtrSize((channelCount == 3) ? 8 : 16) * (inWidth / 4) * (inHeight / 4));

	// Create a PNG image to feed to the compressor
	StringAuto pngFilename(alloc);
	pngFilename.sprintf("%s/AnKiImageImporter_%u.png", tempDirectory.cstr(), U32(std::rand()));
	ANKI_IMPORTER_LOGV("Will store: %s", pngFilename.cstr());
	if(!stbi_write_png(pngFilename.cstr(), inWidth, inHeight, channelCount, inPixels.getBegin(), 0))
	{
		ANKI_IMPORTER_LOGE("STB failed to create: %s", pngFilename.cstr());
		return Error::FUNCTION_FAILED;
	}
	CleanupFile pngCleanup(alloc, pngFilename);

	// Invoke the compressor process
	StringAuto ddsFilename(alloc);
	ddsFilename.sprintf("%s/AnKiImageImporter_%u.dds", tempDirectory.cstr(), U32(std::rand()));
	Process proc;
	Array<CString, 5> args;
	U32 argCount = 0;
	args[argCount++] = "-nomipmap";
	args[argCount++] = "-fd";
	args[argCount++] = (channelCount == 3) ? "BC1" : "BC3";
	args[argCount++] = pngFilename;
	args[argCount++] = ddsFilename;

	ANKI_IMPORTER_LOGV("Will invoke process: CompressonatorCLI %s %s %s %s %s", args[0].cstr(), args[1].cstr(),
					   args[2].cstr(), args[3].cstr(), args[4].cstr());
	ANKI_CHECK(proc.start("CompressonatorCLI", args,
						  (compressonatorPath.isEmpty()) ? ConstWeakArray<CString>()
														 : Array<CString, 2>{{"PATH", compressonatorPath}}));
	CleanupFile ddsCleanup(alloc, ddsFilename);
	ProcessStatus status;
	I32 exitCode;
	ANKI_CHECK(proc.wait(60.0, &status, &exitCode));

	if(status != ProcessStatus::NORMAL_EXIT || exitCode != 0)
	{
		StringAuto errStr(alloc);
		if(exitCode != 0)
		{
			ANKI_CHECK(proc.readFromStdout(errStr));
		}

		if(errStr.isEmpty())
		{
			errStr = "Unknown error";
		}

		ANKI_IMPORTER_LOGE("Invoking compressor process failed: %s", errStr.cstr());
		return Error::FUNCTION_FAILED;
	}

	// Read the DDS file
	File ddsFile;
	ANKI_CHECK(ddsFile.open(ddsFilename, FileOpenFlag::READ | FileOpenFlag::BINARY));
	DdsHeader ddsHeader;
	ANKI_CHECK(ddsFile.read(&ddsHeader, sizeof(DdsHeader)));

	if(channelCount == 3 && memcmp(&ddsHeader.m_ddspf.m_dwFourCC[0], "DXT1", 4) != 0)
	{
		ANKI_IMPORTER_LOGE("Incorrect format. Expecting DXT1");
		return Error::FUNCTION_FAILED;
	}

	if(channelCount == 4 && memcmp(&ddsHeader.m_ddspf.m_dwFourCC[0], "DXT5", 4) != 0)
	{
		ANKI_IMPORTER_LOGE("Incorrect format. Expecting DXT5");
		return Error::FUNCTION_FAILED;
	}

	if(ddsHeader.m_dwWidth != inWidth || ddsHeader.m_dwHeight != inHeight)
	{
		ANKI_IMPORTER_LOGE("Incorrect DDS image size");
		return Error::FUNCTION_FAILED;
	}

	ANKI_CHECK(ddsFile.read(outPixels.getBegin(), outPixels.getSizeInBytes()));

	return Error::NONE;
}

static ANKI_USE_RESULT Error compressAstc(GenericMemoryPoolAllocator<U8> alloc, CString tempDirectory,
										  CString astcencPath, ConstWeakArray<U8, PtrSize> inPixels, U32 inWidth,
										  U32 inHeight, U32 inChannelCount, UVec2 blockSize,
										  WeakArray<U8, PtrSize> outPixels)
{
	const PtrSize blockBytes = 16;
	(void)blockBytes;
	ANKI_ASSERT(inPixels.getSizeInBytes() == PtrSize(inWidth) * inHeight * inChannelCount);
	ANKI_ASSERT(inWidth > 0 && isPowerOfTwo(inWidth) && inHeight > 0 && isPowerOfTwo(inHeight));
	ANKI_ASSERT(outPixels.getSizeInBytes() == blockBytes * (inWidth / blockSize.x()) * (inHeight / blockSize.y()));

	// Create a BMP image to feed to the astcebc
	StringAuto pngFilename(alloc);
	pngFilename.sprintf("%s/AnKiImageImporter_%u.png", tempDirectory.cstr(), U32(std::rand()));
	ANKI_IMPORTER_LOGV("Will store: %s", pngFilename.cstr());
	if(!stbi_write_png(pngFilename.cstr(), inWidth, inHeight, inChannelCount, inPixels.getBegin(), 0))
	{
		ANKI_IMPORTER_LOGE("STB failed to create: %s", pngFilename.cstr());
		return Error::FUNCTION_FAILED;
	}
	CleanupFile pngCleanup(alloc, pngFilename);

	// Invoke the compressor process
	StringAuto astcFilename(alloc);
	astcFilename.sprintf("%s/AnKiImageImporter_%u.astc", tempDirectory.cstr(), U32(std::rand()));
	StringAuto blockStr(alloc);
	blockStr.sprintf("%ux%u", blockSize.x(), blockSize.y());
	Process proc;
	Array<CString, 5> args;
	U32 argCount = 0;
	args[argCount++] = "-cl";
	args[argCount++] = pngFilename;
	args[argCount++] = astcFilename;
	args[argCount++] = blockStr;
	args[argCount++] = "-fast";

	ANKI_IMPORTER_LOGV("Will invoke process: astcenc-avx2 %s %s %s %s %s", args[0].cstr(), args[1].cstr(),
					   args[2].cstr(), args[3].cstr(), args[4].cstr());
	ANKI_CHECK(
		proc.start("astcenc-avx2", args,
				   (astcencPath.isEmpty()) ? ConstWeakArray<CString>() : Array<CString, 2>{{"PATH", astcencPath}}));

	CleanupFile astcCleanup(alloc, astcFilename);
	ProcessStatus status;
	I32 exitCode;
	ANKI_CHECK(proc.wait(60.0, &status, &exitCode));

	if(status != ProcessStatus::NORMAL_EXIT || exitCode != 0)
	{
		StringAuto errStr(alloc);
		if(exitCode != 0)
		{
			ANKI_CHECK(proc.readFromStdout(errStr));
		}

		if(errStr.isEmpty())
		{
			errStr = "Unknown error";
		}

		ANKI_IMPORTER_LOGE("Invoking astcenc-avx2 process failed: %s", errStr.cstr());
		return Error::FUNCTION_FAILED;
	}

	// Read the astc file
	File astcFile;
	ANKI_CHECK(astcFile.open(astcFilename, FileOpenFlag::READ | FileOpenFlag::BINARY));
	AstcHeader header;
	ANKI_CHECK(astcFile.read(&header, sizeof(header)));

	auto unpackBytes = [](U8 a, U8 b, U8 c, U8 d) -> U32 {
		return (U32(a)) + (U32(b) << 8) + (U32(c) << 16) + (U32(d) << 24);
	};

	const U32 magicval = unpackBytes(header.m_magic[0], header.m_magic[1], header.m_magic[2], header.m_magic[3]);
	if(magicval != 0x5CA1AB13)
	{
		ANKI_IMPORTER_LOGE("astcenc-avx2 produced a file with wrong magic");
		return Error::FUNCTION_FAILED;
	}

	const U32 blockx = max<U32>(header.m_blockX, 1u);
	const U32 blocky = max<U32>(header.m_blockY, 1u);
	const U32 blockz = max<U32>(header.m_blockZ, 1u);
	if(blockx != blockSize.x() || blocky != blockSize.y() || blockz != 1)
	{
		ANKI_IMPORTER_LOGE("astcenc-avx2 with wrong block size");
		return Error::FUNCTION_FAILED;
	}

	const U32 dimx = unpackBytes(header.m_dimX[0], header.m_dimX[1], header.m_dimX[2], 0);
	const U32 dimy = unpackBytes(header.m_dimY[0], header.m_dimY[1], header.m_dimY[2], 0);
	const U32 dimz = unpackBytes(header.m_dimZ[0], header.m_dimZ[1], header.m_dimZ[2], 0);
	if(dimx != inWidth || dimy != inHeight || dimz != 1)
	{
		ANKI_IMPORTER_LOGE("astcenc-avx2 with wrong image size");
		return Error::FUNCTION_FAILED;
	}

	ANKI_CHECK(astcFile.read(outPixels.getBegin(), outPixels.getSizeInBytes()));

	return Error::NONE;
}

static ANKI_USE_RESULT Error storeAnkiImage(const ImageImporterConfig& config, const ImageImporterContext& ctx)
{
	ANKI_IMPORTER_LOGV("Storing to %s", config.m_outFilename.cstr());

	File outFile;
	ANKI_CHECK(outFile.open(config.m_outFilename, FileOpenFlag::BINARY | FileOpenFlag::WRITE));

	// Header
	ImageBinaryHeader header = {};
	memcpy(&header.m_magic[0], &IMAGE_MAGIC[0], sizeof(header.m_magic));
	header.m_width = ctx.m_width;
	header.m_height = ctx.m_height;
	header.m_depthOrLayerCount = max(ctx.m_layerCount, ctx.m_depth);
	header.m_type = config.m_type;
	header.m_colorFormat = (ctx.m_channelCount == 3) ? ImageBinaryColorFormat::RGB8 : ImageBinaryColorFormat::RGBA8;
	header.m_compressionMask = config.m_compressions;
	header.m_isNormal = false;
	header.m_mipmapCount = ctx.m_mipmaps.getSize();
	header.m_astcBlockSizeX = config.m_astcBlockSize.x();
	header.m_astcBlockSizeY = config.m_astcBlockSize.y();
	ANKI_CHECK(outFile.write(&header, sizeof(header)));

	// Write RAW
	if(!!(config.m_compressions & ImageBinaryDataCompression::RAW))
	{
		ANKI_IMPORTER_LOGV("Storing RAW");

		// for(I32 mip = I32(ctx.m_mipmaps.getSize()) - 1; mip >= 0; --mip)
		for(U32 mip = 0; mip < ctx.m_mipmaps.getSize(); ++mip)
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					const U32 idx = l * ctx.m_faceCount + f;
					const ConstWeakArray<U8, PtrSize> pixels = ctx.m_mipmaps[mip].m_surfacesOrVolume[idx].m_pixels;
					ANKI_CHECK(outFile.write(&pixels[0], pixels.getSizeInBytes()));
				}
			}
		}
	}

	// Write S3TC
	if(!!(config.m_compressions & ImageBinaryDataCompression::S3TC))
	{
		ANKI_IMPORTER_LOGV("Storing S3TC");

		// for(I32 mip = I32(ctx.m_mipmaps.getSize()) - 1; mip >= 0; --mip)
		for(U32 mip = 0; mip < ctx.m_mipmaps.getSize(); ++mip)
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					const U32 idx = l * ctx.m_faceCount + f;
					const ConstWeakArray<U8, PtrSize> pixels = ctx.m_mipmaps[mip].m_surfacesOrVolume[idx].m_s3tcPixels;
					ANKI_CHECK(outFile.write(&pixels[0], pixels.getSizeInBytes()));
				}
			}
		}
	}

	// Write ASTC
	if(!!(config.m_compressions & ImageBinaryDataCompression::ASTC))
	{
		ANKI_IMPORTER_LOGV("Storing ASTC");

		// for(I32 mip = I32(ctx.m_mipmaps.getSize()) - 1; mip >= 0; --mip)
		for(U32 mip = 0; mip < ctx.m_mipmaps.getSize(); ++mip)
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					const U32 idx = l * ctx.m_faceCount + f;
					const ConstWeakArray<U8, PtrSize> pixels = ctx.m_mipmaps[mip].m_surfacesOrVolume[idx].m_astcPixels;
					ANKI_CHECK(outFile.write(&pixels[0], pixels.getSizeInBytes()));
				}
			}
		}
	}

	return Error::NONE;
}

static ANKI_USE_RESULT Error importImageInternal(const ImageImporterConfig& config)
{
	// Checks
	ANKI_CHECK(checkConfig(config));
	U32 width, height, channelCount;
	ANKI_CHECK(checkInputImages(config, width, height, channelCount));

	// Init image
	GenericMemoryPoolAllocator<U8> alloc = config.m_allocator;
	ImageImporterContext ctx(alloc);

	ctx.m_width = width;
	ctx.m_height = height;
	ctx.m_depth = (config.m_type == ImageBinaryType::_3D) ? config.m_inputFilenames.getSize() : 1;
	ctx.m_faceCount = (config.m_type == ImageBinaryType::CUBE) ? 6 : 1;
	ctx.m_layerCount = (config.m_type == ImageBinaryType::_2D_ARRAY) ? config.m_inputFilenames.getSize() : 1;

	U32 desiredChannelCount;
	if(config.m_noAlpha || channelCount == 1)
	{
		// no alpha or 1 component grey
		desiredChannelCount = 3;
	}
	else if(channelCount == 2)
	{
		// grey with alpha
		desiredChannelCount = 4;
	}
	else
	{
		desiredChannelCount = channelCount;
	}

	ctx.m_channelCount = desiredChannelCount;
	ctx.m_pixelSize = ctx.m_channelCount;

	// Load first mip from the files
	ANKI_CHECK(loadFirstMipmap(config, ctx));

	// Generate mipmaps
	U32 minMipDimension = max(config.m_minMipmapDimension, 4u);
	if(!!(config.m_compressions & ImageBinaryDataCompression::ASTC))
	{
		minMipDimension = max(minMipDimension, config.m_astcBlockSize.x());
		minMipDimension = max(minMipDimension, config.m_astcBlockSize.y());
	}

	const U32 mipCount =
		min(config.m_mipmapCount, (config.m_type == ImageBinaryType::_3D)
									  ? computeMaxMipmapCount3d(width, height, ctx.m_depth, minMipDimension)
									  : computeMaxMipmapCount2d(width, height, minMipDimension));
	for(U32 mip = 1; mip < mipCount; ++mip)
	{
		ctx.m_mipmaps.emplaceBack(alloc);

		if(config.m_type != ImageBinaryType::_3D)
		{
			ctx.m_mipmaps[mip].m_surfacesOrVolume.create(ctx.m_faceCount * ctx.m_layerCount, alloc);
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					const U32 idx = l * ctx.m_faceCount + f;
					const SurfaceOrVolumeData& inSurface = ctx.m_mipmaps[mip - 1].m_surfacesOrVolume[idx];
					SurfaceOrVolumeData& outSurface = ctx.m_mipmaps[mip].m_surfacesOrVolume[idx];
					outSurface.m_pixels.create((ctx.m_width >> mip) * (ctx.m_height >> mip) * ctx.m_pixelSize);

					if(ctx.m_channelCount == 3)
					{
						generateSurfaceMipmap<3>(ConstWeakArray<U8, PtrSize>(inSurface.m_pixels),
												 ctx.m_width >> (mip - 1), ctx.m_height >> (mip - 1),
												 WeakArray<U8, PtrSize>(outSurface.m_pixels));
					}
					else
					{
						ANKI_ASSERT(ctx.m_channelCount == 4);
						generateSurfaceMipmap<4>(ConstWeakArray<U8, PtrSize>(inSurface.m_pixels),
												 ctx.m_width >> (mip - 1), ctx.m_height >> (mip - 1),
												 WeakArray<U8, PtrSize>(outSurface.m_pixels));
					}
				}
			}
		}
		else
		{
			ANKI_ASSERT(!"TODO");
		}
	}

	// Compress
	if(!!(config.m_compressions & ImageBinaryDataCompression::S3TC))
	{
		ANKI_IMPORTER_LOGV("Will compress in S3TC");

		for(U32 mip = 0; mip < mipCount; ++mip)
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					const U32 idx = l * ctx.m_faceCount + f;
					SurfaceOrVolumeData& surface = ctx.m_mipmaps[mip].m_surfacesOrVolume[idx];

					const U32 width = ctx.m_width >> mip;
					const U32 height = ctx.m_height >> mip;
					const PtrSize blockSize = (ctx.m_channelCount == 3) ? 8 : 16;
					const PtrSize s3tcImageSize = blockSize * (width / 4) * (height / 4);

					surface.m_s3tcPixels.create(s3tcImageSize);

					ANKI_CHECK(compressS3tc(alloc, config.m_tempDirectory, config.m_compressonatorPath,
											ConstWeakArray<U8, PtrSize>(surface.m_pixels), width, height,
											ctx.m_channelCount, WeakArray<U8, PtrSize>(surface.m_s3tcPixels)));
				}
			}
		}
	}

	if(!!(config.m_compressions & ImageBinaryDataCompression::ASTC))
	{
		ANKI_IMPORTER_LOGV("Will compress in ASTC");

		for(U32 mip = 0; mip < mipCount; ++mip)
		{
			for(U32 l = 0; l < ctx.m_layerCount; ++l)
			{
				for(U32 f = 0; f < ctx.m_faceCount; ++f)
				{
					const U32 idx = l * ctx.m_faceCount + f;
					SurfaceOrVolumeData& surface = ctx.m_mipmaps[mip].m_surfacesOrVolume[idx];

					const U32 width = ctx.m_width >> mip;
					const U32 height = ctx.m_height >> mip;
					const PtrSize blockSize = 16;
					const PtrSize astcImageSize =
						blockSize * (width / config.m_astcBlockSize.x()) * (height / config.m_astcBlockSize.y());

					surface.m_astcPixels.create(astcImageSize);

					ANKI_CHECK(compressAstc(alloc, config.m_tempDirectory, config.m_astcencPath,
											ConstWeakArray<U8, PtrSize>(surface.m_pixels), width, height,
											ctx.m_channelCount, config.m_astcBlockSize,
											WeakArray<U8, PtrSize>(surface.m_astcPixels)));
				}
			}
		}
	}

	if(!!(config.m_compressions & ImageBinaryDataCompression::ETC))
	{
		ANKI_ASSERT(!"TODO");
	}

	// Store the image
	ANKI_CHECK(storeAnkiImage(config, ctx));

	return Error::NONE;
}

Error importImage(const ImageImporterConfig& config)
{
	const Error err = importImageInternal(config);
	if(err)
	{
		ANKI_IMPORTER_LOGE("Image importing failed");
		return err;
	}

	return Error::NONE;
}

} // end namespace anki
