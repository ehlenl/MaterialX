//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_VKRENDERER_H
#define MATERIALX_VKRENDERER_H

/// @file
/// Glsl Vulkan code renderer

#include <MaterialXRenderVk/Export.h>
// TODO:
//#include <MaterialXRenderVk/Vulkan/vkContext.h>

#include <MaterialXRender/ShaderRenderer.h>
MATERIALX_NAMESPACE_BEGIN

using SimpleWindowPtr = std::shared_ptr<class SimpleWindow>;

/// Shared pointer to a VkRenderer
using VkRendererPtr = std::shared_ptr<class VkRenderer>;

/// @class VkRenderer
/// Helper class for rendering generated Glsl Vulkan code to produce images.
///
/// There are two main interfaces which can be used. One which takes in a HwShader and one which
/// allows for explicit setting of shader stage code.
///
/// The main services provided are:
///     - Validation: All shader stages are compiled and atteched to a Glsl Vulkan shader program.
///     - Introspection: The compiled shader program is examined for uniforms and attributes.
///     - Binding: Uniforms and attributes which match the predefined variables generated the Glsl Vulkan code generator
///       will have values assigned to this. This includes matrices, attribute streams, and textures.
///     - Rendering: The program with bound inputs will be used to drawing geometry to an offscreen buffer.
///     An interface is provided to save this offscreen buffer to disk using an externally defined image handler.
///
class MX_RENDERVK_API VkRenderer : public ShaderRenderer
{
  public:
    /// Create a Glsl Vulkan renderer instance
    static VkRendererPtr create(unsigned int width = 512, unsigned int height = 512, Image::BaseType baseType = Image::BaseType::UINT8);
    
    
    /// Returns Metal Device used for rendering
    // id<MTLDevice> getMetalDevice() const;

    /// Destructor
    virtual ~VkRenderer() { }

    /// @name Setup
    /// @{

    /// Internal initialization of stages and OpenGL constructs
    /// required for program validation and rendering.
    /// An exception is thrown on failure.
    /// The exception will contain a list of initialization errors.
    void initialize(RenderContextHandle renderContextHandle = nullptr) override;

    /// @}
    /// @name Rendering
    /// @{

    /// Create Glsl Vulkan program based on an input shader
    /// @param shader Input HwShader
    void createProgram(ShaderPtr shader) override;

    /// Create Glsl Vulkan program based on shader stage source code.
    /// @param stages Map of name and source code for the shader stages.
    void createProgram(const StageMap& stages) override;

    /// Validate inputs for the program
    void validateInputs() override;

    /// Set the size of the rendered image
    void setSize(unsigned int width, unsigned int height) override;

    /// Render the current program to an offscreen buffer.
    void render() override;

    /// Render the current program in texture space to an off-screen buffer.
    void renderTextureSpace(const Vector2& uvMin, const Vector2& uvMax);

    /// @}
    /// @name Utilities
    /// @{

    /// Capture the current contents of the off-screen hardware buffer as an image.
    ImagePtr captureImage(ImagePtr image = nullptr) override;

    //TODO:
    ///// Return the GL frame buffer.
    //VkFramebufferPtr getFramebuffer() const
    //{
    //    return _framebuffer;
    //}

    ///// Return the Glsl Vulkan program.
    //VkProgramPtr getProgram()
    //{
    //    return _program;
    //}


    /// Set the screen background color.
    void setScreenColor(const Color3& screenColor)
    {
        _screenColor = screenColor;
    }

    /// Return the screen background color.
    Color3 getScreenColor() const
    {
        return _screenColor;
    }

    /// @}
    
  protected:
    VkRenderer(unsigned int width, unsigned int height, Image::BaseType baseType);

    virtual void updateViewInformation();
    virtual void updateWorldInformation();

    //TODO: VULKAN
    void createFrameBuffer(bool encodeSrgb);

  private:
    //VkProgramPtr _program;
    /*
    id<MTLDevice>        _device = nil;
    id<MTLCommandQueue>  _cmdQueue = nil;
    id<MTLCommandBuffer> _cmdBuffer = nil;
    */ 
    // MetalFramebufferPtr  _framebuffer;

    bool _initialized;

    const Vector3 _eye;
    const Vector3 _center;
    const Vector3 _up;
    float _objectScale;

    SimpleWindowPtr _window;
    //VkContextPtr _context;
    //std::shared_ptr<VulkanRenderTarget> _renderTarget;
    Color3 _screenColor;
    // TODO:
    /*
    std::shared_ptr<VulkanRenderPass> _renderPass;

    //HACK
    VkShaderModule load_shader_module(const std::string& path, bool isVertex);

    VkExtent2D swapChainExtent;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    */
};

MATERIALX_NAMESPACE_END
#endif
