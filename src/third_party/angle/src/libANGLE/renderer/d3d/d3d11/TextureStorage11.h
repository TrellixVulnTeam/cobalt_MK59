//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage11.h: Defines the abstract rx::TextureStorage11 class and its concrete derived
// classes TextureStorage11_2D and TextureStorage11_Cube, which act as the interface to the D3D11 texture.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_TEXTURESTORAGE11_H_
#define LIBANGLE_RENDERER_D3D_D3D11_TEXTURESTORAGE11_H_

#include "libANGLE/Error.h"
#include "libANGLE/Texture.h"
#include "libANGLE/renderer/d3d/TextureStorage.h"
#include "libANGLE/renderer/d3d/d3d11/ResourceManager11.h"
#include "libANGLE/renderer/d3d/d3d11/texture_format_table.h"

#include <array>
#include <map>

namespace gl
{
struct ImageIndex;
}

namespace rx
{
class EGLImageD3D;
class RenderTargetD3D;
class RenderTarget11;
class Renderer11;
class SwapChain11;
class Image11;
struct Renderer11DeviceCaps;

class TextureStorage11 : public TextureStorage
{
  public:
    ~TextureStorage11() override;

    static DWORD GetTextureBindFlags(GLenum internalFormat, const Renderer11DeviceCaps &renderer11DeviceCaps, bool renderTarget);
    static DWORD GetTextureMiscFlags(GLenum internalFormat, const Renderer11DeviceCaps &renderer11DeviceCaps, bool renderTarget, int levels);

    UINT getBindFlags() const;
    UINT getMiscFlags() const;
    const d3d11::Format &getFormatSet() const;
    gl::Error getSRVLevels(GLint baseLevel, GLint maxLevel, ID3D11ShaderResourceView **outSRV);
    gl::Error generateSwizzles(const gl::SwizzleState &swizzleTarget);
    void markLevelDirty(int mipLevel);
    void markDirty();

    gl::Error updateSubresourceLevel(ID3D11Resource *texture, unsigned int sourceSubresource,
                                     const gl::ImageIndex &index, const gl::Box &copyArea);

    gl::Error copySubresourceLevel(ID3D11Resource* dstTexture, unsigned int dstSubresource,
                                   const gl::ImageIndex &index, const gl::Box &region);

    // TextureStorage virtual functions
    int getTopLevel() const override;
    bool isRenderTarget() const override;
    bool isManaged() const override;
    bool supportsNativeMipmapFunction() const override;
    int getLevelCount() const override;
    gl::Error generateMipmap(const gl::ImageIndex &sourceIndex,
                             const gl::ImageIndex &destIndex) override;
    gl::Error copyToStorage(TextureStorage *destStorage) override;
    gl::Error setData(const gl::ImageIndex &index,
                      ImageD3D *image,
                      const gl::Box *destBox,
                      GLenum type,
                      const gl::PixelUnpackState &unpack,
                      const uint8_t *pixelData) override;

    virtual gl::Error getSRV(const gl::TextureState &textureState,
                             ID3D11ShaderResourceView **outSRV);
    virtual UINT getSubresourceIndex(const gl::ImageIndex &index) const;
    virtual gl::Error getResource(ID3D11Resource **outResource) = 0;
    virtual void associateImage(Image11* image, const gl::ImageIndex &index) = 0;
    virtual void disassociateImage(const gl::ImageIndex &index, Image11* expectedImage) = 0;
    virtual void verifyAssociatedImageValid(const gl::ImageIndex &index,
                                            Image11 *expectedImage) = 0;
    virtual gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11* incomingImage) = 0;

  protected:
    TextureStorage11(Renderer11 *renderer, UINT bindFlags, UINT miscFlags, GLenum internalFormat);
    int getLevelWidth(int mipLevel) const;
    int getLevelHeight(int mipLevel) const;
    int getLevelDepth(int mipLevel) const;

    // Some classes (e.g. TextureStorage11_2D) will override getMippedResource.
    virtual gl::Error getMippedResource(ID3D11Resource **outResource) { return getResource(outResource); }

    virtual gl::Error getSwizzleTexture(ID3D11Resource **outTexture) = 0;
    virtual gl::Error getSwizzleRenderTarget(int mipLevel,
                                             const d3d11::RenderTargetView **outRTV) = 0;
    gl::Error getSRVLevel(int mipLevel, bool blitSRV, ID3D11ShaderResourceView **outSRV);

    // Get a version of a depth texture with only depth information, not stencil.
    enum DropStencil
    {
        CREATED,
        ALREADY_EXISTS
    };
    virtual gl::ErrorOrResult<DropStencil> ensureDropStencilTexture();
    gl::Error initDropStencilTexture(const gl::ImageIndexIterator &it);

    // The baseLevel parameter should *not* have mTopLevel applied.
    virtual gl::Error createSRV(int baseLevel, int mipLevels, DXGI_FORMAT format, ID3D11Resource *texture,
                                ID3D11ShaderResourceView **outSRV) const = 0;

    void verifySwizzleExists(const gl::SwizzleState &swizzleState);

    // Clear all cached non-swizzle SRVs and invalidate the swizzle cache.
    void clearSRVCache();

    Renderer11 *mRenderer;
    int mTopLevel;
    unsigned int mMipLevels;

    const d3d11::Format &mFormatInfo;
    unsigned int mTextureWidth;
    unsigned int mTextureHeight;
    unsigned int mTextureDepth;

    gl::SwizzleState mSwizzleCache[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
    ID3D11Texture2D *mDropStencilTexture;

  private:
    const UINT mBindFlags;
    const UINT mMiscFlags;

    struct SRVKey
    {
        SRVKey(int baseLevel, int mipLevels, bool swizzle, bool dropStencil);

        bool operator<(const SRVKey &rhs) const;

        int baseLevel    = 0;  // Without mTopLevel applied.
        int mipLevels    = 0;
        bool swizzle     = false;
        bool dropStencil = false;
    };
    typedef std::map<SRVKey, ID3D11ShaderResourceView *> SRVCache;

    gl::Error getCachedOrCreateSRV(const SRVKey &key, ID3D11ShaderResourceView **outSRV);

    SRVCache mSrvCache;
    std::array<ID3D11ShaderResourceView *, gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS> mLevelSRVs;
    std::array<ID3D11ShaderResourceView *, gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS> mLevelBlitSRVs;
};

class TextureStorage11_2D : public TextureStorage11
{
  public:
    TextureStorage11_2D(Renderer11 *renderer, SwapChain11 *swapchain);
    TextureStorage11_2D(Renderer11 *renderer,
                        IUnknown *texture,
                        bool bindChroma,
                        IUnknown *dxgiBuffer);
    TextureStorage11_2D(Renderer11 *renderer, GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels, bool hintLevelZeroOnly = false);
    ~TextureStorage11_2D() override;

    gl::Error getResource(ID3D11Resource **outResource) override;
    gl::Error getMippedResource(ID3D11Resource **outResource) override;
    gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTargetD3D **outRT) override;

    gl::Error copyToStorage(TextureStorage *destStorage) override;

    void associateImage(Image11 *image, const gl::ImageIndex &index) override;
    void disassociateImage(const gl::ImageIndex &index, Image11 *expectedImage) override;
    void verifyAssociatedImageValid(const gl::ImageIndex &index, Image11 *expectedImage) override;
    gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11 *incomingImage) override;

    gl::Error useLevelZeroWorkaroundTexture(bool useLevelZeroTexture) override;

  protected:
    gl::Error getSwizzleTexture(ID3D11Resource **outTexture) override;
    gl::Error getSwizzleRenderTarget(int mipLevel, const d3d11::RenderTargetView **outRTV) override;

    gl::ErrorOrResult<DropStencil> ensureDropStencilTexture() override;

    gl::Error ensureTextureExists(int mipLevels);

  private:
    gl::Error createSRV(int baseLevel,
                        int mipLevels,
                        DXGI_FORMAT format,
                        ID3D11Resource *texture,
                        ID3D11ShaderResourceView **outSRV) const override;

    ID3D11Texture2D *mTexture;
    RenderTarget11 *mRenderTarget[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
    bool mHasKeyedMutex;

    // These are members related to the zero max-LOD workaround.
    // D3D11 Feature Level 9_3 can't disable mipmaps on a mipmapped texture (i.e. solely sample from level zero).
    // These members are used to work around this limitation.
    // Usually only mTexture XOR mLevelZeroTexture will exist.
    // For example, if an app creates a texture with only one level, then 9_3 will only create mLevelZeroTexture.
    // However, in some scenarios, both textures have to be created. This incurs additional memory overhead.
    // One example of this is an application that creates a texture, calls glGenerateMipmap, and then disables mipmaps on the texture.
    // A more likely example is an app that creates an empty texture, renders to it, and then calls glGenerateMipmap
    // TODO: In this rendering scenario, release the mLevelZeroTexture after mTexture has been created to save memory.
    ID3D11Texture2D *mLevelZeroTexture;
    RenderTarget11 *mLevelZeroRenderTarget;
    bool mUseLevelZeroTexture;

    // Swizzle-related variables
    ID3D11Texture2D *mSwizzleTexture;
    d3d11::RenderTargetView mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    Image11 *mAssociatedImages[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
    bool mBindChroma;
    IUnknown *mDxgiBuffer;
};

class TextureStorage11_External : public TextureStorage11
{
  public:
    TextureStorage11_External(Renderer11 *renderer,
                              egl::Stream *stream,
                              const egl::Stream::GLTextureDescription &glDesc);
    ~TextureStorage11_External() override;

    gl::Error getResource(ID3D11Resource **outResource) override;
    gl::Error getMippedResource(ID3D11Resource **outResource) override;
    gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTargetD3D **outRT) override;

    gl::Error copyToStorage(TextureStorage *destStorage) override;

    void associateImage(Image11 *image, const gl::ImageIndex &index) override;
    void disassociateImage(const gl::ImageIndex &index, Image11 *expectedImage) override;
    void verifyAssociatedImageValid(const gl::ImageIndex &index, Image11 *expectedImage) override;
    gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11 *incomingImage) override;

  protected:
    gl::Error getSwizzleTexture(ID3D11Resource **outTexture) override;
    gl::Error getSwizzleRenderTarget(int mipLevel, const d3d11::RenderTargetView **outRTV) override;

  private:
    gl::Error createSRV(int baseLevel,
                        int mipLevels,
                        DXGI_FORMAT format,
                        ID3D11Resource *texture,
                        ID3D11ShaderResourceView **outSRV) const override;

    ID3D11Texture2D *mTexture;
    int mSubresourceIndex;
    bool mHasKeyedMutex;

    Image11 *mAssociatedImage;
};

class TextureStorage11_EGLImage final : public TextureStorage11
{
  public:
    TextureStorage11_EGLImage(Renderer11 *renderer,
                              EGLImageD3D *eglImage,
                              RenderTarget11 *renderTarget11);
    ~TextureStorage11_EGLImage() override;

    gl::Error getResource(ID3D11Resource **outResource) override;
    gl::Error getSRV(const gl::TextureState &textureState,
                     ID3D11ShaderResourceView **outSRV) override;
    gl::Error getMippedResource(ID3D11Resource **outResource) override;
    gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTargetD3D **outRT) override;

    gl::Error copyToStorage(TextureStorage *destStorage) override;

    void associateImage(Image11 *image, const gl::ImageIndex &index) override;
    void disassociateImage(const gl::ImageIndex &index, Image11 *expectedImage) override;
    void verifyAssociatedImageValid(const gl::ImageIndex &index, Image11 *expectedImage) override;
    gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11 *incomingImage) override;

    gl::Error useLevelZeroWorkaroundTexture(bool useLevelZeroTexture) override;

  protected:
    gl::Error getSwizzleTexture(ID3D11Resource **outTexture) override;
    gl::Error getSwizzleRenderTarget(int mipLevel, const d3d11::RenderTargetView **outRTV) override;

  private:
    // Check if the EGL image's render target has been updated due to orphaning and delete
    // any SRVs and other resources based on the image's old render target.
    gl::Error checkForUpdatedRenderTarget();

    gl::Error createSRV(int baseLevel,
                        int mipLevels,
                        DXGI_FORMAT format,
                        ID3D11Resource *texture,
                        ID3D11ShaderResourceView **outSRV) const override;

    gl::Error getImageRenderTarget(RenderTarget11 **outRT) const;

    EGLImageD3D *mImage;
    uintptr_t mCurrentRenderTarget;

    // Swizzle-related variables
    ID3D11Texture2D *mSwizzleTexture;
    std::vector<d3d11::RenderTargetView> mSwizzleRenderTargets;
};

class TextureStorage11_Cube : public TextureStorage11
{
  public:
    TextureStorage11_Cube(Renderer11 *renderer, GLenum internalformat, bool renderTarget, int size, int levels, bool hintLevelZeroOnly);
    ~TextureStorage11_Cube() override;

    UINT getSubresourceIndex(const gl::ImageIndex &index) const override;

    gl::Error getResource(ID3D11Resource **outResource) override;
    gl::Error getMippedResource(ID3D11Resource **outResource) override;
    gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTargetD3D **outRT) override;

    gl::Error copyToStorage(TextureStorage *destStorage) override;

    void associateImage(Image11 *image, const gl::ImageIndex &index) override;
    void disassociateImage(const gl::ImageIndex &index, Image11 *expectedImage) override;
    void verifyAssociatedImageValid(const gl::ImageIndex &index, Image11 *expectedImage) override;
    gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11 *incomingImage) override;

    gl::Error useLevelZeroWorkaroundTexture(bool useLevelZeroTexture) override;

  protected:
    gl::Error getSwizzleTexture(ID3D11Resource **outTexture) override;
    gl::Error getSwizzleRenderTarget(int mipLevel, const d3d11::RenderTargetView **outRTV) override;

    gl::ErrorOrResult<DropStencil> ensureDropStencilTexture() override;

    gl::Error ensureTextureExists(int mipLevels);

  private:
    gl::Error createSRV(int baseLevel,
                        int mipLevels,
                        DXGI_FORMAT format,
                        ID3D11Resource *texture,
                        ID3D11ShaderResourceView **outSRV) const override;
    gl::Error createRenderTargetSRV(ID3D11Resource *texture,
                                    const gl::ImageIndex &index,
                                    DXGI_FORMAT resourceFormat,
                                    ID3D11ShaderResourceView **srv) const;

    static const size_t CUBE_FACE_COUNT = 6;

    ID3D11Texture2D *mTexture;
    RenderTarget11 *mRenderTarget[CUBE_FACE_COUNT][gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    // Level-zero workaround members. See TextureStorage11_2D's workaround members for a description.
    ID3D11Texture2D *mLevelZeroTexture;
    RenderTarget11 *mLevelZeroRenderTarget[CUBE_FACE_COUNT];
    bool mUseLevelZeroTexture;

    ID3D11Texture2D *mSwizzleTexture;
    d3d11::RenderTargetView mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    Image11 *mAssociatedImages[CUBE_FACE_COUNT][gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureStorage11_3D : public TextureStorage11
{
  public:
    TextureStorage11_3D(Renderer11 *renderer, GLenum internalformat, bool renderTarget,
                        GLsizei width, GLsizei height, GLsizei depth, int levels);
    ~TextureStorage11_3D() override;

    gl::Error getResource(ID3D11Resource **outResource) override;

    // Handles both layer and non-layer RTs
    gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTargetD3D **outRT) override;

    void associateImage(Image11 *image, const gl::ImageIndex &index) override;
    void disassociateImage(const gl::ImageIndex &index, Image11 *expectedImage) override;
    void verifyAssociatedImageValid(const gl::ImageIndex &index, Image11 *expectedImage) override;
    gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11 *incomingImage) override;

  protected:
    gl::Error getSwizzleTexture(ID3D11Resource **outTexture) override;
    gl::Error getSwizzleRenderTarget(int mipLevel, const d3d11::RenderTargetView **outRTV) override;

  private:
    gl::Error createSRV(int baseLevel,
                        int mipLevels,
                        DXGI_FORMAT format,
                        ID3D11Resource *texture,
                        ID3D11ShaderResourceView **outSRV) const override;

    typedef std::pair<int, int> LevelLayerKey;
    typedef std::map<LevelLayerKey, RenderTarget11*> RenderTargetMap;
    RenderTargetMap mLevelLayerRenderTargets;

    RenderTarget11 *mLevelRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11Texture3D *mTexture;
    ID3D11Texture3D *mSwizzleTexture;
    d3d11::RenderTargetView mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    Image11 *mAssociatedImages[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureStorage11_2DArray : public TextureStorage11
{
  public:
    TextureStorage11_2DArray(Renderer11 *renderer, GLenum internalformat, bool renderTarget,
                             GLsizei width, GLsizei height, GLsizei depth, int levels);
    ~TextureStorage11_2DArray() override;

    gl::Error getResource(ID3D11Resource **outResource) override;
    gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTargetD3D **outRT) override;

    void associateImage(Image11 *image, const gl::ImageIndex &index) override;
    void disassociateImage(const gl::ImageIndex &index, Image11 *expectedImage) override;
    void verifyAssociatedImageValid(const gl::ImageIndex &index, Image11 *expectedImage) override;
    gl::Error releaseAssociatedImage(const gl::ImageIndex &index, Image11 *incomingImage) override;

  protected:
    gl::Error getSwizzleTexture(ID3D11Resource **outTexture) override;
    gl::Error getSwizzleRenderTarget(int mipLevel, const d3d11::RenderTargetView **outRTV) override;

    gl::ErrorOrResult<DropStencil> ensureDropStencilTexture() override;

  private:
    gl::Error createSRV(int baseLevel,
                        int mipLevels,
                        DXGI_FORMAT format,
                        ID3D11Resource *texture,
                        ID3D11ShaderResourceView **outSRV) const override;
    gl::Error createRenderTargetSRV(ID3D11Resource *texture,
                                    const gl::ImageIndex &index,
                                    DXGI_FORMAT resourceFormat,
                                    ID3D11ShaderResourceView **srv) const;

    typedef std::pair<int, int> LevelLayerKey;
    typedef std::map<LevelLayerKey, RenderTarget11*> RenderTargetMap;
    RenderTargetMap mRenderTargets;

    ID3D11Texture2D *mTexture;

    ID3D11Texture2D *mSwizzleTexture;
    d3d11::RenderTargetView mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    typedef std::map<LevelLayerKey, Image11*> ImageMap;
    ImageMap mAssociatedImages;
};

}

#endif // LIBANGLE_RENDERER_D3D_D3D11_TEXTURESTORAGE11_H_
