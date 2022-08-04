#pragma once
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "vkDeviceBuffer.h"
#include "vkDevice.h"

#include <vector>
#include <algorithm>

class Box3
{
    public:
    Box3()
    : initialized(false)
    {}
    Box3(glm::vec3 _min, glm::vec3 _max)
    : initialized(true), min(_min), max(_max)
    {}
    void Expand(glm::vec3 p)
    {
        if( !initialized )
        {
            min = p;
            max = p;
            initialized = true;
            return;
        }
        min.x=std::min(p.x, min.x);
        min.y=std::min(p.y, min.y);
        min.z=std::min(p.z, min.z);
        max.x=std::max(p.x, max.x);
        max.y=std::max(p.y, max.y);
        max.z=std::max(p.z, max.z);
    }
    glm::vec3 min;
    glm::vec3 max;
    bool initialized;
};

static const std::string RESOURCE_NAME_VERTEX_BUFFER = "Vertices";
static const std::string RESOURCE_NAME_INDEX_BUFFER = "Indices";

enum VertexAttribute
{
    ATTRIBUTE_POSITION=0,
    ATTRIBUTE_NORMAL,
    ATTRIBUTE_TEXCOORD,
    ATTRIBUTE_TANGENT
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
    glm::vec4 tangent;
    static VkVertexInputBindingDescription GetBindingDescription(uint32_t binding)
    {
        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding = binding;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static VkVertexInputAttributeDescription GetAttributeDescription(uint32_t binding, uint32_t location, VertexAttribute attribute)
    {
        switch(attribute)
        {
            case ATTRIBUTE_POSITION: 
                return VkVertexInputAttributeDescription( { location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) } );
            case ATTRIBUTE_NORMAL:
                return VkVertexInputAttributeDescription( { location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) } );
            case ATTRIBUTE_TEXCOORD:
                return VkVertexInputAttributeDescription( { location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texcoord) } );
            case ATTRIBUTE_TANGENT:
                return VkVertexInputAttributeDescription( { location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) } );
            default:
                throw std::runtime_error("Invalid attribute in Vertex attribute description.");
        }
    }
};
class VulkanMaterialXInstance;
class Primitive
{
public:
    Primitive(){}

    glm::mat4 transform;

    uint32_t sceneDataIndexOffset;
    uint32_t indexCount;
    uint32_t indexOffset;
    std::shared_ptr<VulkanBuffer> indexBuffer;
    
    uint32_t sceneDataVertexOffset;
    uint32_t vertexCount;
    uint32_t vertexOffset;
    std::shared_ptr<VulkanBuffer> vertexBuffer;

    std::shared_ptr<VulkanMaterialXInstance> material;
};

struct Mesh
{
    std::string name = "";
    std::vector<Primitive> primitives;
};

class Scene
{
    public:
    Scene() {}
    virtual ~Scene(){}

    void BuildBuffers(std::vector<uint32_t> &sceneIndexData, std::vector<Vertex> &sceneVertexData, std::shared_ptr<VulkanDevice> device)
    {
        for (auto& mesh : meshes)
        {
            for (auto& primitive : mesh.primitives)
            {
                primitive.indexBuffer = device->CreateDeviceBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_FORMAT_R32_UINT);
                primitive.indexBuffer->Write(static_cast<void*>(sceneIndexData.data() + primitive.sceneDataIndexOffset /sizeof(uint32_t)), static_cast<uint32_t>(primitive.indexCount) * sizeof(uint32_t));

                primitive.vertexBuffer = device->CreateDeviceBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_FORMAT_UNDEFINED);
                primitive.vertexBuffer->Write(static_cast<void*>(sceneVertexData.data() + primitive.sceneDataVertexOffset /sizeof(Vertex)), static_cast<uint32_t>(primitive.vertexCount) * sizeof(Vertex));

                // fix this
                primitive.vertexOffset = 0;
                primitive.indexOffset = 0;
            }
        }
    }

    Box3 extents;

    std::vector<Mesh> meshes;
    protected:
};
