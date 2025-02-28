// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/Utils/Functions.h>

namespace anki {

template<typename T>
static void writeShaderBlockMemorySanityChecks(const ShaderVariableBlockInfo& varBlkInfo, const void* elements,
											   U32 elementsCount, void* buffBegin, const void* buffEnd)
{
	// Check args
	ANKI_ASSERT(elements != nullptr);
	ANKI_ASSERT(elementsCount > 0);
	ANKI_ASSERT(buffBegin != nullptr);
	ANKI_ASSERT(buffEnd != nullptr);
	ANKI_ASSERT(buffBegin < buffEnd);

	ANKI_ASSERT(isAligned(alignof(T), ptrToNumber(elements)) && "Breaking strict aliasing rules");
	ANKI_ASSERT(isAligned(alignof(T), ptrToNumber(static_cast<U8*>(buffBegin) + varBlkInfo.m_offset))
				&& "Breaking strict aliasing rules");

	// Check varBlkInfo
	ANKI_ASSERT(varBlkInfo.m_offset != -1);
	ANKI_ASSERT(varBlkInfo.m_arraySize > 0);

	if(varBlkInfo.m_arraySize > 1)
	{
		ANKI_ASSERT(varBlkInfo.m_arrayStride > 0);
	}

	// Check array size
	ANKI_ASSERT(I16(elementsCount) <= varBlkInfo.m_arraySize);
}

template<typename T>
static void writeShaderBlockMemorySimple(const ShaderVariableBlockInfo& varBlkInfo, const void* elements,
										 U32 elementsCount, void* buffBegin, const void* buffEnd)
{
	writeShaderBlockMemorySanityChecks<T>(varBlkInfo, elements, elementsCount, buffBegin, buffEnd);

	U8* buff = static_cast<U8*>(buffBegin) + varBlkInfo.m_offset;
	for(U i = 0; i < elementsCount; i++)
	{
		ANKI_ASSERT(buff + sizeof(T) <= static_cast<const U8*>(buffEnd));

		T* out = reinterpret_cast<T*>(buff);
		const T* in = reinterpret_cast<const T*>(elements) + i;
		*out = *in;

		buff += varBlkInfo.m_arrayStride;
	}
}

template<typename T, typename Vec>
static void writeShaderBlockMemoryMatrix(const ShaderVariableBlockInfo& varBlkInfo, const void* elements,
										 U32 elementsCount, void* buffBegin, const void* buffEnd)
{
	writeShaderBlockMemorySanityChecks<T>(varBlkInfo, elements, elementsCount, buffBegin, buffEnd);
	ANKI_ASSERT(varBlkInfo.m_matrixStride > 0);
	ANKI_ASSERT(varBlkInfo.m_matrixStride >= static_cast<I16>(sizeof(Vec)));

	U8* buff = static_cast<U8*>(buffBegin) + varBlkInfo.m_offset;
	for(U i = 0; i < elementsCount; i++)
	{
		U8* subbuff = buff;
		const T& matrix = static_cast<const T*>(elements)[i];
		for(U j = 0; j < sizeof(T) / sizeof(Vec); j++)
		{
			ANKI_ASSERT((subbuff + sizeof(Vec)) <= static_cast<const U8*>(buffEnd));

			Vec* out = reinterpret_cast<Vec*>(subbuff);
			*out = matrix.getRow(j);
			subbuff += varBlkInfo.m_matrixStride;
		}
		buff += varBlkInfo.m_arrayStride;
	}
}

// This is some trickery to select calling between writeShaderBlockMemoryMatrix and writeShaderBlockMemorySimple
namespace {

template<typename T>
class IsShaderVarDataTypeAMatrix
{
public:
	static constexpr Bool VALUE = false;
};

#define ANKI_SVDT_MACRO(capital, type, baseType, rowCount, columnCount) \
	template<> \
	class IsShaderVarDataTypeAMatrix<type> \
	{ \
	public: \
		static constexpr Bool VALUE = rowCount * columnCount > 4; \
	};
#include <AnKi/Gr/ShaderVariableDataTypeDefs.h>
#undef ANKI_SVDT_MACRO

template<typename T, Bool isMatrix = IsShaderVarDataTypeAMatrix<T>::VALUE>
class WriteShaderBlockMemory
{
public:
	void operator()(const ShaderVariableBlockInfo& varBlkInfo, const void* elements, U32 elementsCount, void* buffBegin,
					const void* buffEnd)
	{
		using RowVec = typename T::RowVec;
		writeShaderBlockMemoryMatrix<T, RowVec>(varBlkInfo, elements, elementsCount, buffBegin, buffEnd);
	}
};

template<typename T>
class WriteShaderBlockMemory<T, false>
{
public:
	void operator()(const ShaderVariableBlockInfo& varBlkInfo, const void* elements, U32 elementsCount, void* buffBegin,
					const void* buffEnd)
	{
		writeShaderBlockMemorySimple<T>(varBlkInfo, elements, elementsCount, buffBegin, buffEnd);
	}
};

} // namespace

void writeShaderBlockMemory(ShaderVariableDataType type, const ShaderVariableBlockInfo& varBlkInfo,
							const void* elements, U32 elementsCount, void* buffBegin, const void* buffEnd)
{
	switch(type)
	{
#define ANKI_SVDT_MACRO(capital, type, baseType, rowCount, columnCount) \
	case ShaderVariableDataType::capital: \
		WriteShaderBlockMemory<type>()(varBlkInfo, elements, elementsCount, buffBegin, buffEnd); \
		break;
#include <AnKi/Gr/ShaderVariableDataTypeDefs.h>
#undef ANKI_SVDT_MACRO

	default:
		ANKI_ASSERT(0);
	}
}

const CString shaderVariableDataTypeToString(ShaderVariableDataType t)
{
	switch(t)
	{
	case ShaderVariableDataType::NONE:
		return "NONE";

#define ANKI_SVDT_MACRO(capital, type, baseType, rowCount, columnCount) \
	case ShaderVariableDataType::capital: \
		return ANKI_STRINGIZE(type);
#define ANKI_SVDT_MACRO_OPAQUE(capital, type) ANKI_SVDT_MACRO(capital, type, 0, 0, 0)
#include <AnKi/Gr/ShaderVariableDataTypeDefs.h>
#undef ANKI_SVDT_MACRO
#undef ANKI_SVDT_MACRO_OPAQUE

	default:
		ANKI_ASSERT(0);
	}

	ANKI_ASSERT(0);
	return "";
}

} // end namespace anki
