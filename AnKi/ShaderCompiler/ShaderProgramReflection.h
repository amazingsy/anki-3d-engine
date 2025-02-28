// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/ShaderCompiler/Common.h>
#include <AnKi/Gr/Enums.h>
#include <AnKi/Gr/Common.h>
#include <AnKi/Util/WeakArray.h>

namespace anki {

/// @addtogroup shader_compiler
/// @{

/// A visitor class that will be used to populate reflection information.
class ShaderReflectionVisitorInterface
{
public:
	virtual ANKI_USE_RESULT Error setWorkgroupSizes(U32 x, U32 y, U32 z, U32 specConstMask) = 0;

	virtual ANKI_USE_RESULT Error setCounts(U32 uniformBlockCount, U32 storageBlockCount, U32 opaqueCount,
											Bool pushConstantBlock, U32 constsCount, U32 structCount) = 0;

	virtual ANKI_USE_RESULT Error visitUniformBlock(U32 idx, CString name, U32 set, U32 binding, U32 size,
													U32 varCount) = 0;

	virtual ANKI_USE_RESULT Error visitUniformVariable(U32 blockIdx, U32 idx, CString name, ShaderVariableDataType type,
													   const ShaderVariableBlockInfo& blockInfo) = 0;

	virtual ANKI_USE_RESULT Error visitStorageBlock(U32 idx, CString name, U32 set, U32 binding, U32 size,
													U32 varCount) = 0;

	virtual ANKI_USE_RESULT Error visitStorageVariable(U32 blockIdx, U32 idx, CString name, ShaderVariableDataType type,
													   const ShaderVariableBlockInfo& blockInfo) = 0;

	virtual ANKI_USE_RESULT Error visitPushConstantsBlock(CString name, U32 size, U32 varCount) = 0;

	virtual ANKI_USE_RESULT Error visitPushConstant(U32 idx, CString name, ShaderVariableDataType type,
													const ShaderVariableBlockInfo& blockInfo) = 0;

	virtual ANKI_USE_RESULT Error visitOpaque(U32 idx, CString name, ShaderVariableDataType type, U32 set, U32 binding,
											  U32 arraySize) = 0;

	virtual ANKI_USE_RESULT Error visitConstant(U32 idx, CString name, ShaderVariableDataType type, U32 constantId) = 0;

	virtual ANKI_USE_RESULT Error visitStruct(U32 idx, CString name, U32 memberCount, U32 size) = 0;

	virtual ANKI_USE_RESULT Error visitStructMember(U32 structIdx, CString structName, U32 memberIdx,
													CString memberName, ShaderVariableDataType type,
													CString typeStructName, U32 offset, U32 arraySize) = 0;

	virtual ANKI_USE_RESULT Bool skipSymbol(CString symbol) const = 0;
};

/// Does reflection using SPIR-V.
ANKI_USE_RESULT Error performSpirvReflection(Array<ConstWeakArray<U8>, U32(ShaderType::COUNT)> spirv,
											 GenericMemoryPoolAllocator<U8> tmpAlloc,
											 ShaderReflectionVisitorInterface& interface);
/// @}

} // end namespace anki
