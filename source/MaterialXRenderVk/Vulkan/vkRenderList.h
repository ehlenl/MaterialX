#include "vkDevice.h"

#include <map>
#include <vector>

class VulkanBuffer;
class VulkanMaterialX;
class VulkanRenderPass;
class VulkanTexture;
class Scene;
class Primitive;

class VulkanRenderList
{
    public:
    VulkanRenderList(VulkanDevicePtr device);
    virtual ~VulkanRenderList();

    void SetGeometryScene( std::shared_ptr<Scene> scene );
    
    void RecordCommands(VkCommandBuffer commandBuffer);

    protected:
    VulkanDevicePtr device;
    std::vector<glm::mat4> geometryTransforms;

    std::shared_ptr<Scene> renderScene;
};
