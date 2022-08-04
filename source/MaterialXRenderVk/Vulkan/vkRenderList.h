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
    VulkanRenderList(std::shared_ptr<VulkanDevice> device);
    virtual ~VulkanRenderList();

    void SetGeometryScene( std::shared_ptr<Scene> scene );
    
    void RecordCommands(VkCommandBuffer commandBuffer);

    protected:
    std::shared_ptr<VulkanDevice> device;
    std::vector<glm::mat4> geometryTransforms;

    std::shared_ptr<Scene> renderScene;
};
