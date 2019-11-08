/** 
 * @file lltexlayerparams.h
 * @brief Texture layer parameters, used by lltexlayer.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLTEXLAYERPARAMS_H
#define LL_LLTEXLAYERPARAMS_H

#include "llpointer.h"
#include "v4color.h"
#include "llviewervisualparam.h"

class LLAvatarAppearance;
class LLImageRaw;
class LLImageTGA;
class LLTexLayer;
class LLTexLayerInterface;
class LLGLTexture;
class LLWearable;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParam
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerParam : public LLViewerVisualParam
{
public: 
	LLTexLayerParam(LLTexLayerInterface *layer);
	LLTexLayerParam(LLAvatarAppearance *appearance);
	/*virtual*/ BOOL setInfo(LLViewerVisualParamInfo *info, BOOL add_to_appearance);
	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable) const = 0;

protected:
	LLTexLayerParam(const LLTexLayerParam& pOther);

	LLTexLayerInterface*	mTexLayer;
	LLAvatarAppearance*		mAvatarAppearance;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParamAlpha
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LL_ALIGN_PREFIX(16)
class LLTexLayerParamAlpha : public LLTexLayerParam
{
public:
	LLTexLayerParamAlpha( LLTexLayerInterface* layer );
	LLTexLayerParamAlpha( LLAvatarAppearance* appearance );
	/*virtual*/ ~LLTexLayerParamAlpha();

	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable = NULL) const;

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL		parseData(LLXmlTreeNode* node);
	/*virtual*/ void		apply( ESex avatar_sex ) {}
	/*virtual*/ void		setWeight(F32 weight);
	/*virtual*/ void		setAnimationTarget(F32 target_value); 
	/*virtual*/ void		animate(F32 delta);

	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion()									{ return 1.f; }
	/*virtual*/ const LLVector4a&	getAvgDistortion()										{ return mAvgDistortionVec; }
	/*virtual*/ F32					getMaxDistortion()										{ return 3.f; }
	/*virtual*/ LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)	{ return LLVector4a(1.f, 1.f, 1.f);}
	/*virtual*/ const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return &mAvgDistortionVec;};
	/*virtual*/ const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return NULL;};

	// New functions
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	BOOL					getSkip() const;
	void					deleteCaches();
	BOOL					getMultiplyBlend() const;

private:
	LLTexLayerParamAlpha(const LLTexLayerParamAlpha& pOther);

	std::shared_ptr<LLGLTexture>    mCachedProcessedTexture;
	LLPointer<LLImageTGA>	mStaticImageTGA;
	LLPointer<LLImageRaw>	mStaticImageRaw;
	BOOL					mNeedsCreateTexture;
	BOOL					mStaticImageInvalid;
	LL_ALIGN_16(LLVector4a				mAvgDistortionVec);
	F32						mCachedEffectiveWeight;

public:
	// Global list of instances for gathering statistics
	static void				dumpCacheByteCount();
	static void				getCacheByteCount( S32* gl_bytes );

	typedef std::list< LLTexLayerParamAlpha* > param_alpha_ptr_list_t;
	static param_alpha_ptr_list_t sInstances;
} LL_ALIGN_POSTFIX(16);
class LLTexLayerParamAlphaInfo : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamAlpha;
public:
	LLTexLayerParamAlphaInfo();
	/*virtual*/ ~LLTexLayerParamAlphaInfo() {};

	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

private:
	std::string				mStaticImageFileName;
	BOOL					mMultiplyBlend;
	BOOL					mSkipIfZeroWeight;
	F32						mDomain;
};
//
// LLTexLayerParamAlpha
//-----------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParamColor
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LL_ALIGN_PREFIX(16)
class LLTexLayerParamColor : public LLTexLayerParam
{
public:
	enum EColorOperation
	{
		OP_ADD = 0,
		OP_MULTIPLY = 1,
		OP_BLEND = 2,
		OP_COUNT = 3 // Number of operations
	};

	LLTexLayerParamColor( LLTexLayerInterface* layer );
	LLTexLayerParamColor( LLAvatarAppearance* appearance );

	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	/* virtual */ ~LLTexLayerParamColor();

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable = NULL) const;

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL			parseData(LLXmlTreeNode* node);
	/*virtual*/ void			apply( ESex avatar_sex ) {}
	/*virtual*/ void			setWeight(F32 weight);
	/*virtual*/ void			setAnimationTarget(F32 target_value);
	/*virtual*/ void			animate(F32 delta);


	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion()									{ return 1.f; }
	/*virtual*/ const LLVector4a&	getAvgDistortion()										{ return mAvgDistortionVec; }
	/*virtual*/ F32					getMaxDistortion()										{ return 3.f; }
	/*virtual*/ LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)	{ return LLVector4a(1.f, 1.f, 1.f); }
	/*virtual*/ const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return &mAvgDistortionVec;};
	/*virtual*/ const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return NULL;};

	// New functions
	LLColor4				getNetColor() const;
protected:
	LLTexLayerParamColor(const LLTexLayerParamColor& pOther);

	virtual void onGlobalColorChanged() {}
private:
	LL_ALIGN_16(LLVector4a				mAvgDistortionVec);
} LL_ALIGN_POSTFIX(16);

class LLTexLayerParamColorInfo : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamColor;

public:
	LLTexLayerParamColorInfo();
	virtual ~LLTexLayerParamColorInfo() {};
	BOOL parseXml( LLXmlTreeNode* node );
	LLTexLayerParamColor::EColorOperation getOperation() const { return mOperation; }
private:
	enum { MAX_COLOR_VALUES = 20 };
	LLTexLayerParamColor::EColorOperation		mOperation;
	LLColor4			mColors[MAX_COLOR_VALUES];
	S32					mNumColors;
};

typedef std::vector<LLTexLayerParamColor *> param_color_list_t;
typedef std::vector<LLTexLayerParamAlpha *> param_alpha_list_t;
typedef std::vector<LLTexLayerParamColorInfo *> param_color_info_list_t;
typedef std::vector<LLTexLayerParamAlphaInfo *> param_alpha_info_list_t;

#endif
