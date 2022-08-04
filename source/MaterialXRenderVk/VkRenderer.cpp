//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXRenderVk/VkRenderer.h>
#include <MaterialXRenderHw/SimpleWindow.h>

//#include <iostream>
//#include <algorithm>

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

    //_camera = Camera::create();
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
        //_context = VkContext::create(_window);
        //if (!_context)
        //{
        //    throw ExceptionRenderError("Failed to create OpenGL context for renderer");
        //}

        //if (_context->makeCurrent())
        //{
        //    // Initialize glad
        //    if (!gladLoadGL())
        //    {
        //        throw ExceptionRenderError("OpenGL support is required");
        //    }

        //    glClearStencil(0);

        //    _framebuffer = VkFramebuffer::create(_width, _height, 4, _baseType);
        //    _initialized = true;
        //}
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
#if 0
    float fH = std::tan(FOV_PERSP / 360.0f * PI) * NEAR_PLANE_PERSP;
    float fW = fH * 1.0f;

    _camera->setViewMatrix(Camera::createViewMatrix(_eye, _center, _up));
    _camera->setProjectionMatrix(Camera::createPerspectiveMatrix(-fW, fW, -fH, fH, NEAR_PLANE_PERSP, FAR_PLANE_PERSP));
#endif
}

void VkRenderer::updateWorldInformation()
{
#if 0
     _camera->setWorldMatrix(Matrix44::createScale(Vector3(_objectScale)));
#endif
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
