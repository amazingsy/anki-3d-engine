// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// VolumetricFog effects.
class VolumetricFog : public RendererObject
{
public:
	VolumetricFog(Renderer* r)
		: RendererObject(r)
	{
	}

	~VolumetricFog()
	{
	}

	ANKI_USE_RESULT Error init(const ConfigSet& config);

	void setFogParticleColor(const Vec3& col)
	{
		m_fogDiffuseColor = col;
	}

	const Vec3& getFogParticleColor() const
	{
		return m_fogDiffuseColor;
	}

	void setParticleDensity(F32 d)
	{
		m_fogDensity = d;
	}

	F32 getParticleDensity() const
	{
		return m_fogDensity;
	}

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

	RenderTargetHandle getRt() const
	{
		return m_runCtx.m_rt;
	}

	const Array<U32, 3>& getVolumeSize() const
	{
		return m_volumeSize;
	}

	/// Get the last cluster split in Z axis that will be affected by lighting.
	U32 getFinalClusterInZ() const
	{
		return m_finalZSplit;
	}

private:
	ShaderProgramResourcePtr m_prog;
	ShaderProgramPtr m_grProg;

	RenderTargetDescription m_rtDescr;

	U32 m_finalZSplit = 0;

	Array<U32, 2> m_workgroupSize = {};
	Array<U32, 3> m_volumeSize;

	Vec3 m_fogDiffuseColor = Vec3(1.0f);
	F32 m_fogDensity = 0.9f;
	F32 m_fogScatteringCoeff = 0.01f;
	F32 m_fogAbsorptionCoeff = 0.02f;

	class
	{
	public:
		RenderTargetHandle m_rt;
	} m_runCtx; ///< Runtime context.

	void run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx);
};
/// @}

} // end namespace anki
