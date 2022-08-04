#pragma once
#include "vkDevice.h"
#include "Geometry.h"

#include <MaterialXGenShader/TypeDesc.h>
#include <MaterialXGenShader/Library.h>

#include <set>

class VulkanRenderPass;
class VulkanRenderList;
class VulkanRenderTarget;
class VulkanTexture;
class VulkanBuffer;
class VulkanHostBuffer;

namespace spirv_cross {
    class CompilerGLSL;
};

class VulkanMaterialX
{
    friend class VulkanMaterialXInstance;
    public:
    VulkanMaterialX(std::shared_ptr<VulkanDevice> device, std::string materialXFilename);
    virtual ~VulkanMaterialX();
    enum ShaderStage
    {
        SS_VERTEX=0,
        SS_TESSELATION_CONTROL,
        SS_TESSELATION_EVALUATION,
        SS_GEOMETRY,
        SS_FRAGMENT,
        SS_COMPUTE,
        SS_RAYGEN,
        SS_ANY_HIT,
        SS_CLOSEST_HIT,
        SS_MISS,
        SS_TASK,
        SS_MESH,

        SS_COUNT // always last
    };

    void SetCullMode( VkCullModeFlagBits _cullMode ) { cullMode = _cullMode; }
    void SetPrimitiveTopology( VkPrimitiveTopology _primitiveTopology) { primitiveTopology = _primitiveTopology; };
    void SetLineWidth( float _lineWidth ) { lineWidth = _lineWidth; }
    void SetDepthTesting( bool enabled ) { depthTesting = enabled; }

    void Initialize(std::shared_ptr<VulkanRenderPass> _renderPass);

    void LoadRadianceMaps();

    std::shared_ptr<VulkanRenderPass> GetRenderPass(){ return renderPass; }

protected:
    struct BackedBuffer
    {
        std::shared_ptr<VulkanHostBuffer> buffer;
        std::shared_ptr<uint8_t> bufferData;
    };
    struct EditableUniform {
        EditableUniform() : isDirty(false), offset(0) {}
        std::shared_ptr<BackedBuffer> backedBuffer;
        uint32_t offset;
        bool isDirty;
    };
    std::map<std::string, EditableUniform> editableUniforms;

public:
    protected:
    void CreatePipeline();

    VkShaderModule shaderModules[SS_COUNT];
    VkShaderStageFlagBits shaderStageBits[SS_COUNT]={
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        VK_SHADER_STAGE_GEOMETRY_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_COMPUTE_BIT,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        VK_SHADER_STAGE_MISS_BIT_KHR,
        VK_SHADER_STAGE_TASK_BIT_NV,
        VK_SHADER_STAGE_MESH_BIT_NV
    };
    std::string shaderFilenames[SS_COUNT];

    static const std::map<std::string, VertexAttribute> vertexAttributeMapping;

    void CreateShaderModules();
    void CreateDescriptorSetLayouts(const uint32_t* shaderCode, uint32_t shaderCodeSize, VkShaderStageFlagBits stage);
    void CreateInputAttributeDescriptions(spirv_cross::CompilerGLSL* compiler);

    // MaterialX
    void WriteMaterialXParameterValues(uint8_t* data, const std::string& parameterBlockName, const std::string& parameterVariableName);
    void WriteMaterialXParameterDataBlock(MaterialX::TypeDesc::BaseType baseType, std::vector<std::string>& values, uint8_t* data);
    void LoadMaterialXTextures();
    void CompileMaterialXShader();

    std::shared_ptr<VulkanDevice> device;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;

    std::map<uint32_t, VkDescriptorSetLayout> descriptorSetLayouts;

    std::vector<VkPushConstantRange> pushConstantRanges;

    std::shared_ptr<VulkanRenderPass> renderPass;

    VkCullModeFlagBits cullMode;
    VkPrimitiveTopology primitiveTopology;
    float lineWidth;
    bool depthTesting;
    glm::uvec2 imageExtent;

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkSampler radianceSampler;

    std::shared_ptr<VulkanRenderTarget> pipelineRenderTarget;

    std::map<std::string, std::shared_ptr<VulkanTexture>> namedTextures;

    std::map < uint32_t, std::map<uint32_t, std::shared_ptr<VulkanBuffer>>> uniformBufferBindings;
    std::map < uint32_t, std::map<uint32_t, std::shared_ptr<VulkanTexture>>> samplerBufferBindings;

    struct DescriptorSetLayoutData {
        VkDescriptorSetLayoutCreateInfo create_info;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };
    std::map<uint32_t, DescriptorSetLayoutData> descriptorSetLayoutDatas;

    std::string materialXShader;
    std::string materialXShaderDir;

    MaterialX::ShaderGeneratorPtr materialXShaderGenerator;
    MaterialX::ShaderPtr materialXShaderPtr;

    public:
    std::shared_ptr<VulkanRenderTarget> GetRenderTarget(){ return pipelineRenderTarget; }
};

class VulkanMaterialXInstance
{
public:
    VulkanMaterialXInstance(std::shared_ptr<VulkanMaterialX> _material)
        : material(_material)
    {
        editableUniforms = material->editableUniforms;

        uniformBufferBindings = material->uniformBufferBindings;
        samplerBufferBindings = material->samplerBufferBindings;
        
        ReallocateUniformBuffer({ "u_worldMatrix", "u_worldInverseTransposeMatrix" });

        AllocateDescriptorSets();
        WriteDescriptorBindings();
    }
    virtual ~VulkanMaterialXInstance() {}

    template<class T>
    bool SetUniform(std::string uniformName, T* uniformValuePtr, size_t uniformSize)
    {
        auto uniformIterator = this->editableUniforms.find(uniformName);
        if (uniformIterator == this->editableUniforms.end())
        {
            assert(false);
            return false;
        }
        auto& uniformData = uniformIterator->second;
        memcpy((void*)(uniformData.backedBuffer->bufferData.get() + uniformData.offset), uniformValuePtr, uniformSize);
        uniformData.isDirty = true;
        return true;
    }
    void UpdateUniforms();

    std::map<uint32_t, VkDescriptorSet>& GetDescriptorSets() { return descriptorSets; }
    VkPipeline GetPipeline() { return material->graphicsPipeline; }
    VkPipelineLayout GetPipelineLayout() { return material->pipelineLayout; }

protected:
    // descriptor set number, binding, resource
    void WriteDescriptorBindings();
    void AllocateDescriptorSets();
    void ReallocateUniformBuffer(std::vector<std::string> uniformNames);

    std::shared_ptr<VulkanMaterialX> material;
    std::map<std::string, VulkanMaterialX::EditableUniform> editableUniforms;

    std::map<uint32_t, VkDescriptorSet> descriptorSets;

    std::map < uint32_t, std::map<uint32_t, std::shared_ptr<VulkanBuffer>>> uniformBufferBindings;
    std::map < uint32_t, std::map<uint32_t, std::shared_ptr<VulkanTexture>>> samplerBufferBindings;
};

class VulkanMaterialXCache
{
public:
    VulkanMaterialXCache() {}
    virtual ~VulkanMaterialXCache() {}

    std::shared_ptr<VulkanMaterialXInstance> AcquireMaterial(std::shared_ptr<VulkanDevice> device, std::shared_ptr<VulkanRenderPass> renderPass, std::string mtlxDocumentFilename)
    {
        auto it = materials.find(mtlxDocumentFilename);
        if (it == materials.end())
        {
            auto material = std::make_shared<VulkanMaterialX>(device, mtlxDocumentFilename);
            material->Initialize(renderPass);
            materials[mtlxDocumentFilename] = material;
            it = materials.find(mtlxDocumentFilename);
        }

        auto materialInstance = std::make_shared<VulkanMaterialXInstance>(it->second);

        return materialInstance;
    }
protected:
    std::map<std::string, std::shared_ptr<VulkanMaterialX>> materials;
};
