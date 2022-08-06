#include "vkRenderList.h"
#include "vkBuffer.h"
#include "vkMaterialX.h"
#include "vkRenderPass.h"
#include "vkSwapchain.h"
#include "vkHostBuffer.h"
#include "vkDeviceBuffer.h"
#include "vkTexture.h"
#include "vkHelpers.h"

#include <glm/gtc/type_ptr.hpp>
#include <array>

#include "Geometry.h"

VulkanRenderList::VulkanRenderList(VulkanDevicePtr _device)
:   device(_device)
{
}

VulkanRenderList::~VulkanRenderList()
{
}

void VulkanRenderList::SetGeometryScene( std::shared_ptr<Scene> scene )
{
    this->renderScene = scene;
}

void VulkanRenderList::RecordCommands(VkCommandBuffer commandBuffer)
{
    for (auto& mesh : renderScene->meshes)
    {
        for (auto& prim : mesh.primitives)
        {
            std::vector<VkDescriptorSet> descriptorSetArray;
            for (auto& ds : prim.material->GetDescriptorSets())
                descriptorSetArray.push_back(ds.second);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prim.material->GetPipeline());

            // use uniforms instead of push constants since shaders generated from MaterialX won't have push constants defined
            auto world = prim.transform;
            prim.material->SetUniform("u_worldMatrix", glm::value_ptr(world), sizeof(glm::mat4x4));
            auto worldInveseTranspose = glm::inverse(glm::transpose(world));
            prim.material->SetUniform("u_worldInverseTransposeMatrix", glm::value_ptr(worldInveseTranspose), sizeof(glm::mat4x4));
            prim.material->UpdateUniforms();

            VkDeviceSize vertexOffset = prim.vertexOffset;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &prim.vertexBuffer->GetBuffer(), &vertexOffset);

            vkCmdBindIndexBuffer(commandBuffer, prim.indexBuffer->GetBuffer(), prim.indexOffset, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prim.material->GetPipelineLayout(), 0, static_cast<uint32_t>(descriptorSetArray.size()), descriptorSetArray.data(), 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, prim.indexCount, 1, 0, 0, 0);
        }
    }
}
