//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXRenderVk/VkRenderer.h>
#include <MaterialXRenderHw/SimpleWindow.h>
#include <MaterialXRenderVk/Vulkan/vkSwapchain.h>
#include <MaterialXRenderVk/Vulkan/vkRenderTarget.h>
#include <MaterialXRenderVk/Vulkan/vkRenderPass.h>
#include <MaterialXRenderVk/Vulkan/vkBuffer.h>
#include <MaterialXRenderVk/Vulkan/vkDeviceBuffer.h>
#include <MaterialXRenderVk/Vulkan/vkMaterialX.h>
#include <iostream>
#include <fstream>

//#include <iostream>
//#include <algorithm>

// View information
const float FOV_PERSP = 45.0f; // degrees
const float NEAR_PLANE_PERSP = 0.05f;
const float FAR_PLANE_PERSP = 100.0f;

//#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>
//#if defined(_WIN32)
//    #include <vulkan/vulkan_win32.h>
//#elif defined(__linux__)
//    #include <vulkan/vulkan_xcb.h>
//#elif defined(__ANDROID__)
//    #include <vulkan/vulkan_android.h>
//#endif
//

//#include <MaterialXRender/TinyObjLoader.h>
//#include <MaterialXGenShader/HwShaderGenerator.h>

//#include <MaterialXRenderVk/External/Glad/glad.h>
//#include <MaterialXRenderVk/VkUtil.h>


MATERIALX_NAMESPACE_BEGIN

//
// VkRenderer methods
//

VkRendererPtr VkRenderer::create(unsigned int width, unsigned int height, Image::BaseType baseType)
{
    return VkRendererPtr(new VkRenderer(width, height, baseType));
}

VkRenderer::VkRenderer(unsigned int width, unsigned int height, Image::BaseType baseType) :
    ShaderRenderer(width, height, baseType),
    _initialized(false),
    _eye(0.0f, 0.0f, 3.0f),
    _center(0.0f, 0.0f, 0.0f),
    _up(0.0f, 1.0f, 0.0f),
    _objectScale(1.0f),
    _screenColor(DEFAULT_SCREEN_COLOR_LIN_REC709)
{
    //_program = VkProgram::create();

    //_geometryHandler = GeometryHandler::create();
    //_geometryHandler->addLoader(TinyObjLoader::create());

    _camera = Camera::create();
}

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
void VkRenderer::createGraphicsPipeline()
{
    VulkanDevicePtr device = _context->_device;
    swapChainExtent.width = _width;
    swapChainExtent.height = _height;

    auto vertShaderCode = readFile("F:/source/MaterialX-adsk-fork/vert.spv");
    auto fragShaderCode = readFile("F:/source/MaterialX-adsk-fork/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = _renderPass->GetRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device->GetDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(device->GetDevice(), vertShaderModule, nullptr);
}

VkShaderModule VkRenderer::createShaderModule(const std::vector<char>& code)
{
    VulkanDevicePtr device = _context->_device;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void VkRenderer::initialize()
{

    if (!_initialized)
    {
        // Create window
        _window = SimpleWindow::create();

        if (!_window->initialize("Renderer Window", _width, _height, nullptr))
        {
            throw ExceptionRenderError("Failed to initialize renderer window");
        }

        // Create offscreen context
        _context = VkContext::create(_window);
        _context->initializeVulkan(true);
        
        
        VulkanDevicePtr device = _context->_device;

        if (!device)
        {
            throw ExceptionRenderError("Failed to create Vulkan device for renderer");
        }

        glm::uvec2 windowExtents(_width, _height);
        auto swapchain = device->CreateSwapchain(windowExtents);

        assert(swapchain);

        _renderTarget = device->CreateRenderTarget(
            { { device->FindSurfaceFormat().format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT } },
            true, glm::uvec3(windowExtents, 1u));

        assert(_renderTarget);

        swapchain->Bind(_renderTarget->GetColorTarget(0));

        // Render pass
        _renderPass = device->CreateRenderPass();
        _renderPass->Initialize(_renderTarget);

        // Create Buffer
        std::vector<float> vertices = {
            0.0f, -0.75f, 0.0f,
            -0.75f, 0.75f, 0.0f,
            0.75f, 0.75f, 0.0f
        };
        std::vector<uint32_t> indices = {
            0, 1, 2
        };
        std::shared_ptr<VulkanBuffer> vertexBuffer;
        std::shared_ptr<VulkanBuffer> indexBuffer;

        vertexBuffer = device->CreateDeviceBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_FORMAT_UNDEFINED);
        vertexBuffer->Write(vertices.data(), vertices.size() * sizeof(float));
                
        indexBuffer = device->CreateDeviceBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_FORMAT_R32_UINT);
        indexBuffer->Write(indices.data(), indices.size() * sizeof(uint32_t));

        // Create pipeline 1
        createGraphicsPipeline();

        // Create Pipeline
        std::shared_ptr<VulkanMaterialXCache> materialCache;
        if (!materialCache)
            materialCache = std::make_shared<VulkanMaterialXCache>();
        
        static string materialXDocumentPath = R"(F:\source\MaterialX-adsk-fork\resources\Materials\Examples\StandardSurface\standard_surface_default.mtlx)";
        auto material = materialCache->AcquireMaterial(device, _renderPass, materialXDocumentPath);
        int radianceMips = 4;
        material->SetUniform("u_envRadianceMips", &radianceMips, sizeof(int));
        int radianceSamples = 64;
        material->SetUniform("u_envRadianceSamples", &radianceSamples, sizeof(int));

        updateViewInformation();
        updateWorldInformation();

        
        Matrix44 w = _camera->getWorldMatrix();
        material->SetUniform("u_worldMatrix", w.data(), 16);
        
        Matrix44 wi = _camera->getWorldMatrix().getInverse();
        material->SetUniform("u_worldInverseTransposeMatrix", wi.data(), 16);

        material->UpdateUniforms();

        //Clear and Draw
        _renderPass->CreateCommandBuffers();
        auto commandBuffer = _renderPass->GetCommandBuffer();
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VK_ERROR_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        _renderPass->BeginRenderPass(0);

        std::vector<VkDescriptorSet> descriptorSetArray;
        for (auto& ds : material->GetDescriptorSets())
            descriptorSetArray.push_back(ds.second);

        VkDeviceSize vertexstride = 0;
        VkDeviceSize indexstride = 0;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline  /*material->GetPipeline()*/);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->GetBuffer(), &vertexstride);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        //vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout /* material->GetPipelineLayout()*/,
        //    0, static_cast<uint32_t>(descriptorSetArray.size()), descriptorSetArray.data(), 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);
        
        _renderPass->EndRenderPass();
        VK_ERROR_CHECK(vkEndCommandBuffer(commandBuffer));

        _renderPass->Draw();
        swapchain->Blit();
        vkDeviceWaitIdle(device->GetDevice());
    }

}

void VkRenderer::createProgram(ShaderPtr shader)
{
#if 0
    if (!_context || !_context->makeCurrent())
    {
        throw ExceptionRenderError("Invalid OpenGL context in createProgram");
    }

    _program->setStages(shader);
    _program->build();
#endif
}

void VkRenderer::createProgram(const StageMap& /*stages*/)
{
#if 0
    if (!_context || !_context->makeCurrent())
    {
        throw ExceptionRenderError("Invalid OpenGL context in createProgram");
    }

    for (const auto& it : stages)
    {
        _program->addStage(it.first, it.second);
    }
    _program->build();
#endif
}

void VkRenderer::renderTextureSpace(const Vector2& /*uvMin*/, const Vector2& /*uvMax*/)
{
#if 0
    _program->bind();
    _program->bindTextures(_imageHandler);

    _framebuffer->bind();
    drawScreenSpaceQuad(uvMin, uvMax);
    _framebuffer->unbind();

    _program->unbind();
#endif
}

void VkRenderer::validateInputs()
{
#if 0
    if (!_context || !_context->makeCurrent())
    {
        throw ExceptionRenderError("Invalid OpenGL context in validateInputs");
    }

    // Check that the generated uniforms and attributes are valid
    _program->getUniformsList();
    _program->getAttributesList();
#endif
}

void VkRenderer::setSize(unsigned int /*width*/, unsigned int /*height*/)
{
    #if 0
    if (_context->makeCurrent())
    {
        if (_framebuffer)
        {
            _framebuffer->resize(width, height);
        }
        else
        {
            _framebuffer = VkFramebuffer::create(width, height, 4, _baseType);
        }
        _width = width;
        _height = height;
    }
    #endif
}

void VkRenderer::updateViewInformation()
{

    float fH = std::tan(FOV_PERSP / 360.0f * 3.141) * NEAR_PLANE_PERSP;
    float fW = fH * 1.0f;

    _camera->setViewMatrix(Camera::createViewMatrix(_eye, _center, _up));
    _camera->setProjectionMatrix(Camera::createPerspectiveMatrix(-fW, fW, -fH, fH, NEAR_PLANE_PERSP, FAR_PLANE_PERSP));

}

void VkRenderer::updateWorldInformation()
{

     _camera->setWorldMatrix(Matrix44::createScale(Vector3(_objectScale)));

}

void VkRenderer::render()
{
#if 0
    if (!_context || !_context->makeCurrent())
    {
        throw ExceptionRenderError("Invalid OpenGL context in render");
    }

    // Set up target
    _framebuffer->bind();

    glClearColor(_screenColor[0], _screenColor[1], _screenColor[2], 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateViewInformation();
    updateWorldInformation();

    try
    {
        // Bind program and input parameters
        if (_program)
        {
            // Check if we have any attributes to bind. If not then
            // there is nothing to draw
            if (!_program->hasActiveAttributes())
            {
                throw ExceptionRenderError("Program has no input vertex data");
            }
            else
            {
                // Bind the shader program.
                if (!_program->bind())
                {
                    throw ExceptionRenderError("Cannot bind inputs without a valid program");
                }

                // Update uniforms and attributes.
                _program->getUniformsList();
                _program->getAttributesList();

                // Bind shader properties.
                _program->bindViewInformation(_camera);
                _program->bindTextures(_imageHandler);
                _program->bindLighting(_lightHandler, _imageHandler);
                _program->bindTimeAndFrame();

                // Set blend state for the given material.
                if (_program->getShader()->hasAttribute(HW::ATTR_TRANSPARENT))
                {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                else
                {
                    glDisable(GL_BLEND);
                }

                // Bind each mesh and draw its partitions.
                for (MeshPtr mesh : _geometryHandler->getMeshes())
                {
                    _program->bindMesh(mesh);
                    for (size_t i = 0; i < mesh->getPartitionCount(); i++)
                    {
                        MeshPartitionPtr part = mesh->getPartition(i);
                        _program->bindPartition(part);
                        MeshIndexBuffer& indexData = part->getIndices();
                        glDrawElements(GL_TRIANGLES, (GLsizei)indexData.size(), GL_UNSIGNED_INT, (void*)0);
                    }
                }

                // Unbind resources
                _imageHandler->unbindImages();
                _program->unbind();

                // Restore blend state.
                if (_program->getShader()->hasAttribute(HW::ATTR_TRANSPARENT))
                {
                    glDisable(GL_BLEND);
                }
            }
        }
    }
    catch (ExceptionRenderError& e)
    {
        _framebuffer->unbind();
        throw e;
    }

    // Unset target
    _framebuffer->unbind();
#endif
}

ImagePtr VkRenderer::captureImage(ImagePtr /*image*/)
{
    //return _framebuffer->getColorImage(image);
    return nullptr;
}

void VkRenderer::drawScreenSpaceQuad(const Vector2& /*uvMin*/, const Vector2& /*uvMax*/)
{
#if 0
    const float QUAD_VERTICES[] =
    {
         1.0f,  1.0f, 0.0f, uvMax[0], uvMax[1], // position, texcoord
         1.0f, -1.0f, 0.0f, uvMax[0], uvMin[1],
        -1.0f, -1.0f, 0.0f, uvMin[0], uvMin[1],
        -1.0f,  1.0f, 0.0f, uvMin[0], uvMax[1]
    };
    const unsigned int QUAD_INDICES[] =
    {
        0, 1, 3,
        1, 2, 3
    };
    const unsigned int VERTEX_STRIDE = 5;
    const unsigned int TEXCOORD_OFFSET = 3;

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES, GL_STATIC_DRAW);

    for (const auto& pair : _program->getAttributesList())
    {
        if (pair.first.find(HW::IN_POSITION) != std::string::npos)
        {
            glEnableVertexAttribArray(pair.second->location);
            glVertexAttribPointer(pair.second->location, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float), (void*) 0);
        }

        if (pair.first.find(HW::IN_TEXCOORD + "_") != std::string::npos)
        {
            glEnableVertexAttribArray(pair.second->location);
            glVertexAttribPointer(pair.second->location, 2, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float), (void*) (TEXCOORD_OFFSET * sizeof(float)));
        }
    }

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QUAD_INDICES), QUAD_INDICES, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
    glBindBuffer(GL_ARRAY_BUFFER, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VkProgram::UNDEFINED_OPENGL_RESOURCE_ID);

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    checkGlErrors("after draw screen-space quad");
#endif
}

MATERIALX_NAMESPACE_END
