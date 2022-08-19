#include "vkMaterialX.h"
#include "vkRenderList.h"
#include "vkRenderPass.h"
#include "vkSwapchain.h"
#include "vkTexture.h"
#include "vkHostBuffer.h"
#include "vkDeviceBuffer.h"
#include "vkRenderTarget.h"
#include "vkHelpers.h"

#include <shaderc/shaderc.h>
#include <spirv_cross/spirv_reflect.hpp>

//#include <stb_image.h>
#include <chrono>

//#include <nlohmann/json.hpp>

//using json = nlohmann::json;

#include <MaterialXCore/Material.h>
#include <MaterialXCore/Types.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenGlsl/VkShaderGenerator.h>

#include <cstring>
#include <unordered_set>

const std::map<std::string, VertexAttribute> VulkanMaterialX::vertexAttributeMapping = {
        {"i_position", VertexAttribute::ATTRIBUTE_POSITION},
        {"i_normal", VertexAttribute::ATTRIBUTE_NORMAL},
        {"i_tangent", VertexAttribute::ATTRIBUTE_TANGENT},
        {"i_texcoord_0", VertexAttribute::ATTRIBUTE_TEXCOORD}
};

static std::string MATERIALX_INSTALL_DIR = ".";

VulkanMaterialX::VulkanMaterialX(VulkanDevicePtr _device, std::string materialXFilename)
:   cullMode(VK_CULL_MODE_BACK_BIT),
    primitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
    lineWidth(1.f),
    depthTesting(true),
    pipelineLayout(nullptr),
    graphicsPipeline(nullptr),
    radianceSampler(nullptr)
{
    device = _device;

    materialXShader = materialXFilename;
    auto pathPos = materialXShader.find_last_of('/');
    materialXShaderDir = materialXShader.substr(0, pathPos);

    for( size_t i=0; i<SS_COUNT; ++i )
        shaderModules[i]=nullptr;

    this->LoadRadianceMaps();
}

VulkanMaterialX::~VulkanMaterialX()
{
    if (radianceSampler)
        vkDestroySampler(device->GetDevice(), radianceSampler, nullptr);

    for (auto& descriptorSetLayoutIt : this->descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(device->GetDevice(), descriptorSetLayoutIt.second, nullptr);

    for( size_t i=0; i<SS_COUNT; ++i )
    {
        if( shaderModules[i] != nullptr )
            vkDestroyShaderModule(device->GetDevice(), shaderModules[i], nullptr);
    }
    if( pipelineLayout )
        vkDestroyPipelineLayout(this->device->GetDevice(), pipelineLayout, nullptr);
    if( graphicsPipeline )
        vkDestroyPipeline(this->device->GetDevice(), graphicsPipeline, nullptr);
}

void VulkanMaterialX::LoadRadianceMaps()
{
    auto texture = device->RetrieveTextureFromCache("u_envIrradiance");
    if (!texture)
    {
        texture = LoadTextureFromImageFile(this->device, "F:/source/MaterialX-adsk-fork/resources/Lights/san_giuseppe_bridge_split.hdr", true);
        device->AddTextureToCache("u_envIrradiance", texture);
    }
    this->namedTextures["u_envIrradiance"] = texture;

    texture = device->RetrieveTextureFromCache("u_envRadiance");
    if (!texture)
    {
        texture = LoadTextureFromImageFile(this->device, "F:/source/MaterialX-adsk-fork/resources/Lights/san_giuseppe_bridge.hdr", true);
        device->AddTextureToCache("u_envRadiance", texture);
    }
    this->namedTextures["u_envRadiance"] = texture;

    // create default sampler
    radianceSampler = CreateDefaultSampler(this->device, true);
}

void VulkanMaterialX::CompileMaterialXShader()
{
    //TODO: Fix Shader compile

    auto loadLibraries = [](const MaterialX::FilePathVec& libraryFolders, const MaterialX::FileSearchPath& searchPath) -> MaterialX::DocumentPtr
    {
        MaterialX::DocumentPtr doc = MaterialX::createDocument();
        for (const std::string& libraryFolder : libraryFolders)
        {
            MaterialX::XmlReadOptions readOptions;

            MaterialX::FilePath libraryPath = searchPath.find(libraryFolder);
            for (const MaterialX::FilePath& filename : libraryPath.getFilesInDirectory(MaterialX::MTLX_EXTENSION))
            {
                MaterialX::DocumentPtr libDoc = MaterialX::createDocument();
                MaterialX::readFromXmlFile(libDoc, libraryPath / filename, MaterialX::FileSearchPath(), &readOptions);
                doc->importLibrary(libDoc);
            }
        }
        return doc;
    };
    
    static std::string materialXBaseDir(R"(F:\source\MaterialX-adsk-fork)");
    assert(materialXBaseDir.length() > 0);

    // set search path
    MaterialX::FileSearchPath searchPath;
    searchPath.append(MaterialX::FilePath(materialXBaseDir));
    searchPath.append(MaterialX::FilePath(materialXBaseDir) / "libraries");

    auto docDir = MaterialX::FilePath(materialXShaderDir);
    searchPath.append(docDir);

    // setup read options
    MaterialX::XmlReadOptions readOptions;
    readOptions.readXIncludeFunction = [](MaterialX::DocumentPtr doc, const MaterialX::FilePath& filename,
        const MaterialX::FileSearchPath& searchPath, const MaterialX::XmlReadOptions* options)
    {
        MaterialX::FilePath resolvedFilename = searchPath.find(filename);
        if (resolvedFilename.exists())
        {
            readFromXmlFile(doc, resolvedFilename, searchPath, options);
        }
        else
        {
            std::cerr << "Include file not found: " << filename.asString() << std::endl;
        }
    };

    // setup library folders
    MaterialX::FilePathVec libraryFolders = {   "targets",
                                                "adsk/adsklib", "adsk/adsklib/genglsl",
                                                "stdlib", "stdlib/genglsl",
                                                "pbrlib", "pbrlib/genglsl",
                                                "bxdf",
                                                "lights", "lights/genglsl" };

    // create standard lib
    auto stdLib = MaterialX::createDocument();
    auto xIncludeFiles = MaterialX::loadLibraries(libraryFolders, searchPath, stdLib);
    
    // read document from disk
    MaterialX::DocumentPtr doc = MaterialX::createDocument();

    MaterialX::readFromXmlFile(doc, materialXShader, searchPath, &readOptions);

    doc->importLibrary(stdLib);

    std::vector<MaterialX::TypedElementPtr> elements;
    std::unordered_set<MaterialX::ElementPtr> processedOutputs;

    MaterialX::findRenderableMaterialNodes(doc, elements, false, processedOutputs);

    if (elements.size() == 0)
        throw std::runtime_error("No renderable material nodes found in " + materialXShader);

    auto materialNodeName = elements[0]->getName();

    materialXShaderGenerator = MaterialX::VkShaderGenerator::create();
    //materialXShaderGenerator = MaterialX::GlslShaderGenerator::create();
    auto generatorContext = MaterialX::GenContext(materialXShaderGenerator);

    for (const MaterialX::FilePath& path : searchPath)
        generatorContext.registerSourceCodeSearchPath(path);

    auto generateShaders = [&](auto node)
    {
        try {
            auto &tokenSubstitutions = materialXShaderGenerator->getTokenSubstitutions();
            materialXShaderPtr = materialXShaderGenerator->generate(node->getName(), node, generatorContext);

            shaderc_compiler_t compiler = shaderc_compiler_initialize();
            auto compileOptions = shaderc_compile_options_initialize();
            shaderc_compile_options_set_auto_bind_uniforms(compileOptions, true);
            shaderc_compile_options_set_auto_map_locations(compileOptions, true);

            for (int i = 0; i < materialXShaderPtr->numStages(); ++i)
            {
                auto shaderStage = materialXShaderPtr->getStage(i);
                std::string elementName(node->getName());
                std::string stageId;

                shaderc_shader_kind shaderKind;
                ShaderStage shaderStageIndex;
                if (shaderStage.getName() == "vertex")
                {
                    shaderKind = shaderc_shader_kind::shaderc_glsl_vertex_shader;
                    shaderStageIndex = ShaderStage::SS_VERTEX;
                }
                else
                {
                    shaderKind = shaderc_shader_kind::shaderc_glsl_fragment_shader;
                    shaderStageIndex = ShaderStage::SS_FRAGMENT;
                }

                std::string sourceCode = shaderStage.getSourceCode();

#ifdef _DEBUG
                std::ofstream sourceCodeStream(shaderStage.getName() + ".glsl");
                sourceCodeStream << sourceCode;
                sourceCodeStream.close();
#endif


                std::vector<std::pair<std::string, std::string>> replaceStrings = { std::make_pair("sampler)", "samplerr)"), std::make_pair("sampler,", "samplerr,") };
                for (auto& replaceString : replaceStrings)
                {
                    auto pos = sourceCode.find(replaceString.first);
                    while (pos != std::string::npos)
                    {
                        sourceCode.erase(pos, replaceString.first.length());
                        sourceCode.insert(pos, replaceString.second);
                        pos = sourceCode.find(replaceString.first);
                    }
                }
                std::string compileFileName = elementName + "." + shaderStage.getName();
                shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, sourceCode.c_str(), sourceCode.length(), shaderKind, compileFileName.c_str(), "main", compileOptions);

                auto numErrors = shaderc_result_get_num_errors(result);
                if (numErrors != 0)
                {
                    VK_LOG << shaderc_result_get_error_message(result) << std::endl;
                    throw std::runtime_error("Error compiling shader.");
                }

                auto compiledShaderSize = static_cast<uint32_t>(shaderc_result_get_length(result));
                auto compiledShaderBytes = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));

                shaderModules[shaderStageIndex] = CreateShaderModule(this->device->GetDevice(), compiledShaderBytes, compiledShaderSize);

                VkPipelineShaderStageCreateInfo shaderStageInfo = {};
                shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStageInfo.stage = shaderStageBits[shaderStageIndex];
                shaderStageInfo.module = shaderModules[shaderStageIndex];
                shaderStageInfo.pName = "main";

                shaderStageCreateInfos.push_back(shaderStageInfo);
                
                this->CreateDescriptorSetLayouts(compiledShaderBytes, compiledShaderSize, shaderStageBits[shaderStageIndex]);

                shaderc_result_release(result);

            }
            shaderc_compile_options_release(compileOptions);
            shaderc_compiler_release(compiler);

        }
        catch (MaterialX::ExceptionShaderGenError & e) {
            std::cerr << e.what() << std::endl;
        }
    };

    // generate shaders
    //MaterialX::GlslResourceBindingContextPtr bindCtx = std::make_shared<MaterialX::GlslResourceBindingContext>(0, 0);
    //bindCtx->enableSeparateBindingLocations(true);
    //generatorContext.pushUserData(MaterialX::HW::USER_DATA_BINDING_CONTEXT, bindCtx);

    auto nodeGraph = doc->getNodeGraph(materialNodeName);
    if (nodeGraph)
    {
        MaterialX::TypedElementPtr outputShader = nodeGraph->getOutputs()[0];
        generateShaders(outputShader);
    }
    else {
        auto materialNodes = doc->getMaterialNodes();
        if (!materialNodes.empty())
        {
            for (const MaterialX::NodePtr& materialNode : materialNodes)
            {
                if (materialNode->getName() == materialNodeName)
                {
                    std::vector<MaterialX::NodePtr> shaderNodes = MaterialX::getShaderNodes(materialNode);
                    if (!shaderNodes.empty())
                    {
                        MaterialX::TypedElementPtr shaderNode = *shaderNodes.begin();
                        generateShaders(shaderNode);
                        break;
                    }
                }
            }
        }
    }
    //generatorContext.popUserData(MaterialX::HW::USER_DATA_BINDING_CONTEXT);

}

void VulkanMaterialX::CreateShaderModules()
{
    shaderStageCreateInfos.clear();

    CompileMaterialXShader();

    // create descriptor set layouts
    for (auto& descriptorSetLayoutData : descriptorSetLayoutDatas)
        VK_ERROR_CHECK(vkCreateDescriptorSetLayout(this->device->GetDevice(), &descriptorSetLayoutData.second.create_info, nullptr, &this->descriptorSetLayouts[descriptorSetLayoutData.first]));
}

void VulkanMaterialX::LoadMaterialXTextures()
{
    static std::string materialXBaseDir(MATERIALX_INSTALL_DIR);
    auto& tokenSubstitutions = this->materialXShaderGenerator->getTokenSubstitutions();
    for (int i = 0; i < this->materialXShaderPtr->numStages(); ++i)
    {
        auto shaderStage = this->materialXShaderPtr->getStage(i);
        auto uniformBlocks = shaderStage.getUniformBlocks();
        for (auto& uniformBlock : uniformBlocks)
        {
            const auto& uniforms = uniformBlock.second;
            for (auto& uniform : uniforms->getVariableOrder())
            {
                auto variableName = uniform->getName();
                auto substitutedName = tokenSubstitutions.find(variableName);
                if (substitutedName != tokenSubstitutions.end())
                    variableName = substitutedName->second;

                if( uniform->getType()->getSemantic() == MaterialX::TypeDesc::SEMANTIC_FILENAME )
                {
                    if (uniform->getValue() && uniform->getValue()->getValueString().length() > 0)
                    {
                        auto textureFile = uniform->getValue()->getValueString();
                        // try and load the texture
                        auto texture = LoadTextureFromImageFile(this->device, materialXShaderDir + "/" + textureFile, false);
                        if (texture)
                            this->namedTextures[variableName] = texture;
                    }
                }
            }
        }
    }
}

void VulkanMaterialX::WriteMaterialXParameterDataBlock(MaterialX::TypeDesc::BaseType baseType, std::vector<std::string>& values, uint8_t* data)
{
    uint32_t offset = 0;
    // this is really just a sanity check for the types
    switch (baseType)
    {
    case MaterialX::TypeDesc::BASETYPE_BOOLEAN:
        offset = 4;
        break;
    case MaterialX::TypeDesc::BASETYPE_FLOAT:
        offset = sizeof(float);
        break;
    case MaterialX::TypeDesc::BASETYPE_INTEGER:
        offset = sizeof(int);
        break;
    case MaterialX::TypeDesc::BASETYPE_STRING: // interpret as int
        offset = sizeof(int);
        baseType = MaterialX::TypeDesc::BASETYPE_INTEGER;
        break;
    default:
        throw std::runtime_error("Unknown basetype " + std::to_string(baseType));
    }

    for (auto& val : values)
    {
        std::string stringValue = val;
        switch (baseType)
        {
        case MaterialX::TypeDesc::BASETYPE_FLOAT:
        {
            auto f = static_cast<float>(std::atof(stringValue.c_str()));
            memcpy(data, (void*)&f, sizeof(float));
            break;
        }
        case MaterialX::TypeDesc::BASETYPE_INTEGER:
        {
            auto i = static_cast<int>(std::atoi(stringValue.c_str()));
            memcpy(data, (void*)&i, sizeof(int));
            break;
        }
        case MaterialX::TypeDesc::BASETYPE_BOOLEAN:
        {
            uint32_t b = stringValue == "true" ? 1u : 0u;
            memcpy(data, (void*)&b, offset);
            break;
        }
        }
        data += offset;
    }
}

void VulkanMaterialX::WriteMaterialXParameterValues(uint8_t* data, const std::string &parameterBlockName, const std::string &parameterVariableName)
{
    // TODO fix special case
    if (parameterBlockName == "LightData_pixel")
    {
        struct LightData {
            int type;
            float pad0;
            float pad1;
            float pad2;
        };
        std::vector<LightData> ld = { {1, 0.f, 0.f, 0.f}, {0, 0.f, 0.f, 0.f}, {0, 0.f, 0.f, 0.f} };
        memcpy(data, (void*)ld.data(), sizeof(LightData) * 3);
        return;
    }
    // end TODO

    auto& tokenSubstitutions = materialXShaderGenerator->getTokenSubstitutions();
    for (int i = 0; i < materialXShaderPtr->numStages(); ++i)
    {
        auto shaderStage = materialXShaderPtr->getStage(i);
        auto uniformBlocks = shaderStage.getUniformBlocks();
        for (auto& uniformBlock : uniformBlocks)
        {
            const auto& uniforms = uniformBlock.second;
            std::string blockName = uniforms->getName() + "_" + shaderStage.getName();

            if (blockName != parameterBlockName)
                continue;

            for (auto& uniform : uniforms->getVariableOrder())
            {
                auto variableName = uniform->getName();
                auto substitutedName = tokenSubstitutions.find(variableName);
                if (substitutedName != tokenSubstitutions.end())
                    variableName = substitutedName->second;

                if (variableName != parameterVariableName)
                    continue;

                auto baseType = static_cast<MaterialX::TypeDesc::BaseType>(uniform->getType()->getBaseType());
                if( uniform->getType()->getSemantic() != MaterialX::TypeDesc::SEMANTIC_FILENAME )
                {
                    std::vector<std::string> values;
                    if (uniform->getValue())
                        values = tokenize(uniform->getValue()->getValueString(), ",");
                    else
                        values = std::vector<std::string>(uniform->getType()->getSize(), "0");

                    WriteMaterialXParameterDataBlock(baseType, values, data);
                }
            }
        }
    }
}

void VulkanMaterialX::CreateInputAttributeDescriptions(spirv_cross::CompilerGLSL* comp)
{
    //TODO: Fix reflections

    spirv_cross::ShaderResources res = comp->get_shader_resources();
    for (auto& inputResource : res.stage_inputs)
    {
        auto it = vertexAttributeMapping.find(inputResource.name);
        if (it == vertexAttributeMapping.end())
            throw std::runtime_error("Unknown input attribute " + std::string(inputResource.name));

        assert(comp->has_decoration(inputResource.id, spv::DecorationLocation));
        auto location = comp->get_decoration(inputResource.id, spv::DecorationLocation);
        attributeDescriptions.push_back(Vertex::GetAttributeDescription(0, location, it->second));
    }

    bindingDescriptions.push_back(Vertex::GetBindingDescription(0));

}

void VulkanMaterialX::CreateDescriptorSetLayouts(const uint32_t* shaderCode, uint32_t shaderCodeSize, VkShaderStageFlagBits stage)
{
    LoadMaterialXTextures();

    try {
    std::vector<uint32_t> shaderCodeVector;
    shaderCodeVector.assign(shaderCode, shaderCode + (shaderCodeSize/sizeof(uint32_t)));
    spirv_cross::CompilerGLSL comp(std::move(shaderCodeVector));
    spirv_cross::ShaderResources res = comp.get_shader_resources();

    if (stage == VK_SHADER_STAGE_VERTEX_BIT)
        this->CreateInputAttributeDescriptions(&comp);

    uint32_t offset = static_cast<uint32_t>(this->descriptorSetLayouts.size());

    auto getResourceData = [&stage, this](const spirv_cross::Resource& resource, spirv_cross::Compiler &comp, VkDescriptorType descriptorType, uint32_t& set, uint32_t& binding)
    {
        assert(comp.has_decoration(resource.id, spv::DecorationDescriptorSet));
        set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        assert(comp.has_decoration(resource.id, spv::DecorationBinding));
        binding = comp.get_decoration(resource.id, spv::DecorationBinding);

        DescriptorSetLayoutData& layout = this->descriptorSetLayoutDatas[set];
        layout.bindings.resize(layout.bindings.size() + 1);
        auto &layoutBinding = *layout.bindings.rbegin();

        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = stage;
        layoutBinding.descriptorType = descriptorType;
    };

    for (const spirv_cross::Resource& resource : res.sampled_images)
    {
        uint32_t set, binding;
        getResourceData(resource, comp, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, set, binding);

        auto it = this->namedTextures.find(resource.name);
        if (it != this->namedTextures.end())
            this->samplerBufferBindings[set+offset][binding] = it->second;
    }

    for (const spirv_cross::Resource& resource : res.uniform_buffers)
    {
        uint32_t set, binding;
        getResourceData(resource, comp, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, set, binding);

        // create buffer for storing the data for this block
        auto uniformBufferData = this->device->CreateHostBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_FORMAT_UNDEFINED);

        auto& type = comp.get_type(resource.base_type_id);
        uint32_t blockDataSize = static_cast<uint32_t>(comp.get_declared_struct_size(type));
        uint8_t* blockData = reinterpret_cast<uint8_t*>(malloc(static_cast<size_t>(blockDataSize)));
        std::shared_ptr<uint8_t> blockDataPtr(blockData, ::free); // hang on to this pointer in case we want to update uniform variables later

        auto backedBuffer = std::make_shared<BackedBuffer>();
        backedBuffer->buffer = uniformBufferData;
        backedBuffer->bufferData = blockDataPtr;

        unsigned member_count = static_cast<unsigned>(type.member_types.size());
        for (unsigned i = 0; i < member_count; i++)
        {
            size_t member_size = comp.get_declared_struct_member_size(type, i);
            size_t offset = comp.type_struct_member_offset(type, i);
            auto memberName = comp.get_member_name(type.self, i);

            if (this->materialXShaderPtr)
                WriteMaterialXParameterValues(blockData + (size_t)offset, resource.name, memberName);

            EditableUniform editableUniform;
            editableUniform.backedBuffer = backedBuffer;
            editableUniform.offset = static_cast<uint32_t>(offset);
            editableUniforms[memberName] = editableUniform;
        }

        uniformBufferData->Write(blockData, blockDataSize);
        this->uniformBufferBindings[set+offset][binding] = uniformBufferData;
    }

    for (auto& descriptorSetLayoutData : descriptorSetLayoutDatas)
    {
        auto& layout = descriptorSetLayoutData.second;
        layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout.create_info.bindingCount = static_cast<uint32_t>(layout.bindings.size());
        layout.create_info.pBindings = layout.bindings.data();
    }

    }
    catch (std::exception & e) {
        VK_LOG << e.what() << std::endl;
    }
}

void VulkanMaterialX::Initialize(std::shared_ptr<VulkanRenderPass> _renderPass)
{
    this->renderPass = _renderPass;
    this->imageExtent = this->renderPass->GetRenderTarget()->GetExtent();
    
    this->CreatePipeline();
}

void VulkanMaterialX::CreatePipeline()
{
    if( pipelineLayout )
        vkDestroyPipelineLayout(this->device->GetDevice(), pipelineLayout, nullptr);
    if( graphicsPipeline )
        vkDestroyPipeline(this->device->GetDevice(), graphicsPipeline, nullptr);

    if( renderPass == nullptr )
        VK_LOG << " Creating VulkanMaterialX with null VulkanRenderPass." << std::endl;

    this->CreateShaderModules();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(this->bindingDescriptions.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(this->attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = this->bindingDescriptions.data();
    vertexInputInfo.pVertexAttributeDescriptions = this->attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = primitiveTopology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(imageExtent.x);
    viewport.height = static_cast<float>(imageExtent.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = { imageExtent.x, imageExtent.y};

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
    rasterizer.lineWidth = lineWidth;
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTesting;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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

    std::vector<VkDescriptorSetLayout> descriptorSetLayoutArray;
    for (auto& ds : descriptorSetLayouts)
        descriptorSetLayoutArray.push_back(ds.second);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayoutArray.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayoutArray.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(this->pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = this->pushConstantRanges.data();

    VK_ERROR_CHECK(vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
    pipelineInfo.pStages = shaderStageCreateInfos.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = primitiveTopology;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // need to do this for resizing viewport

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass->GetRenderPass();
    pipelineInfo.subpass = 0;

    VK_ERROR_CHECK(vkCreateGraphicsPipelines(this->device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphicsPipeline));
}

void VulkanMaterialXInstance::WriteDescriptorBindings()
{
    for (auto& descriptorSetBindings : uniformBufferBindings)
    {
        for (auto& bindingBuffer : descriptorSetBindings.second)
            WriteDescriptorBuffer(this->material->device, 
                                  bindingBuffer.second->GetBuffer(), 
                                  bindingBuffer.second->GetBufferSize(), 
                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bindingBuffer.first, 
                                  descriptorSets[descriptorSetBindings.first], 0);
    }

    for (auto& descriptorSetBindings : samplerBufferBindings)
    {
        for (auto& bindingBuffer : descriptorSetBindings.second)
            WriteDescriptorImage(this->material->device, bindingBuffer.second->GetImageView(), material->radianceSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, bindingBuffer.first, descriptorSets[descriptorSetBindings.first]);
    }

}

void VulkanMaterialXInstance::UpdateUniforms()
{
    // group updates of individual uniforms within a block
    std::map<std::shared_ptr<VulkanBuffer>, std::shared_ptr<uint8_t>> buffers;
    for (auto& uniformIterator : this->editableUniforms)
    {
        if (uniformIterator.second.isDirty)
        {
            buffers[uniformIterator.second.backedBuffer->buffer] = uniformIterator.second.backedBuffer->bufferData;
            uniformIterator.second.isDirty = false;
        }
    }

    for (auto it : buffers)
        it.first->Write(it.second.get(), it.first->GetBufferSize());
}

void VulkanMaterialXInstance::AllocateDescriptorSets()
{
    for (auto& descriptorSetLayoutData : material->descriptorSetLayoutDatas)
    {
        VkResult result = VK_ERROR_OUT_OF_POOL_MEMORY;
        while (result != VK_SUCCESS)
        {
            // allocate descriptor sets for the layouts
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = this->material->device->GetDescriptorPool(material->renderPass->GetCommandBuffer());
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &this->material->descriptorSetLayouts[descriptorSetLayoutData.first];

            result = vkAllocateDescriptorSets(
                this->material->device->GetDevice(), &allocInfo, &this->descriptorSets[descriptorSetLayoutData.first]);

            if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
                this->material->device->AllocateNewDescriptorPool(material->renderPass->GetCommandBuffer());
        }
    }
}

void VulkanMaterialXInstance::ReallocateUniformBuffer(std::vector<std::string> uniformNames)
{
    std::map<std::shared_ptr<VulkanHostBuffer>, std::shared_ptr<VulkanHostBuffer>> oldNew;
    std::map<std::shared_ptr<VulkanHostBuffer>, std::shared_ptr<VulkanMaterialX::BackedBuffer>> oldNewBacked;
    for (auto& set : uniformBufferBindings)
    {
        for (auto& binding : set.second)
        {
            auto oldBuffer = std::dynamic_pointer_cast<VulkanHostBuffer>(binding.second);
            auto newBuffer = material->device->CreateHostBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_FORMAT_UNDEFINED);
            newBuffer->Allocate(oldBuffer->GetBufferSize());
            auto* bufferData = oldBuffer->Map();
            newBuffer->Write(bufferData, oldBuffer->GetBufferSize());
            oldBuffer->UnMap();
            oldNew[oldBuffer] = newBuffer;
            binding.second = newBuffer;
            
            uint8_t* backingStorage = reinterpret_cast<uint8_t*>(malloc(static_cast<size_t>(oldBuffer->GetBufferSize())));
            std::shared_ptr<uint8_t> backingStoragePtr(backingStorage, ::free); // hang on to this pointer in case we want to update uniform variables later

            auto backedBuffer = std::make_shared<VulkanMaterialX::BackedBuffer>();
            backedBuffer->buffer = newBuffer;
            backedBuffer->bufferData = backingStoragePtr;
            oldNewBacked[oldBuffer] = backedBuffer;
        }
    }
    
    for (auto& editableUniform : editableUniforms)
    {
        auto it = oldNewBacked.find(editableUniform.second.backedBuffer->buffer);
        memcpy(it->second->bufferData.get(), editableUniform.second.backedBuffer->bufferData.get(), it->second->buffer->GetBufferSize());
        editableUniform.second.backedBuffer = it->second;
        editableUniform.second.isDirty = true;
    }
}
