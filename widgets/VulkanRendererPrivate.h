#pragma once

#include <vector>
#include <array>
#include <shared_mutex>
#include <QVector2D>
#include <QVulkanWindowRenderer>
#include <QtAV/private/VideoRenderer_p.h>
#include "ColorTransform.h"

namespace QtAV
{
    struct TextureObject
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;
        uint32_t width;
        uint32_t height;
        void* mappedMemory;
        VkSubresourceLayout layout;

        TextureObject()
        {
            image = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
            imageView = VK_NULL_HANDLE;
            width = 0;
            height = 0;
            mappedMemory = nullptr;
        }
    };

    struct UniformBufferObject
    {
        QMatrix4x4 posTransform;
        QMatrix4x4 colorTransform;

        UniformBufferObject()
        {
            posTransform.setToIdentity();
            colorTransform.setToIdentity();
        }
    };

    class VulkanWindowRenderer;
    class VulkanRendererPrivate : public VideoRendererPrivate
    {
    public:
        VulkanRendererPrivate();
        ~VulkanRendererPrivate();

        void setupAspectRatio();

    public:
        QWidget* widget = nullptr;
        QVulkanInstance vulkanInstance;
        VulkanWindowRenderer* vulkanWindowRenderer = nullptr;
        ColorTransform colorTransform;
        QMatrix4x4 matrix;
    };

    class VulkanWindowRenderer : public QVulkanWindowRenderer
    {
    public:
        VulkanWindowRenderer(QVulkanWindow* w);

        // QVulkanWindowRenderer virtual function
        virtual void initResources() override;
        virtual void initSwapChainResources() override;
        virtual void preInitResources() override;
        virtual void releaseResources() override;
        virtual void releaseSwapChainResources() override;
        virtual void startNextFrame() override;

        void setCurrentFrame(const VideoFrame& frame);
        void setColorTransformMatrix(const QMatrix4x4& matrix);
        void setPosTransformMatrix(const QMatrix4x4& matrix);

    private:
        VkShaderModule createShaderModule(const QByteArray& code);
        void createGraphicsPipeline();
        void createVertexBuffer();
        void createDescriptorSetLayout();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void createTextureObject();
        void createTextureSampler();
        void cleanupGraphicsPipeline();
        void cleanupVertexBuffer();
        void cleanupDescriptorSetLayout();
        void cleanupUniformBuffers();
        void cleanupDescriptorPool();
        void cleanupDescriptorSets();
        void cleanupTextureObject();
        void cleanupTextureSampler();
        void updateUniformBuffer(uint32_t currentImage);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);     // ��������ָ�����͵��ڴ�����
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory
        );
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createImage(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory
        );
        VkImageView createImageView(VkImage image, VkFormat format);
        void updateTextureImage(uint32_t currentImage);
        void VulkanWindowRenderer::updateSinglePlaneTexture(const TextureObject& textureObj, const uchar* planeData, int lineSize);
        bool checkTextureObject();

    private:
        QVulkanWindow* m_window = nullptr;
        QVulkanInstance* m_vulkanInstance = nullptr;
        QVulkanFunctions* m_vulkanFunctions = nullptr;
        QVulkanDeviceFunctions* m_vulkanDeviceFunctions = nullptr;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;                 // �����豸
        VkDevice m_device = VK_NULL_HANDLE;									// �߼��豸
        int m_swapChainImageCount = 0;										// ��������ͼ�������
        QSize m_swapChainImageSize = { 0, 0 };								// ��������ͼ��Ĵ�С
        VkRenderPass m_renderPass = VK_NULL_HANDLE;							// ��Ⱦͨ��
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;                 // ���߲���
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;                     // ͼ�ι���
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;                           // ���㻺��
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;               // ���㻺���ڴ�
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;       // ������
        std::vector<VkBuffer> m_uniformBuffers;
        std::vector<VkDeviceMemory> m_uniformBuffersMemory;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;                 // ��������
        std::vector<VkDescriptorSet> m_descriptorSets;
        std::vector<TextureObject> m_yTextureObjects;
        std::vector<TextureObject> m_uTextureObjects;
        std::vector<TextureObject> m_vTextureObjects;
        VkSampler m_textureSampler = VK_NULL_HANDLE;
        VideoFrame m_currentFrame;
        std::mutex m_frameMutex;                                            // ������������VideoFrame
        std::mutex m_uboObjMutex;                                           // ������������uboObj
        UniformBufferObject m_uboObj;
        std::vector<bool> m_uboUploaded;
    };

    struct Vertex {
        QVector2D pos;
        QVector2D texCoord;

        // ��ȡ���������
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;                                         // ������
            bindingDescription.stride = sizeof(Vertex);                             // ÿ��������ֽڴ�С
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;             // ÿ���������һ������

            return bindingDescription;
        }

        // ��ȡ������������
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            // λ�����ԣ�location 0����2��32λ��������R32G32_SFLOAT��
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }
    };
}