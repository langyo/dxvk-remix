#include "dxvk_query_pool.h"

namespace dxvk {
  
  DxvkQueryPool::DxvkQueryPool(
    const Rc<vk::DeviceFn>& vkd,
          VkQueryType       queryType)
  : m_vkd(vkd), m_queryType(queryType) {
    VkQueryPoolCreateInfo info;
    info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.pNext      = nullptr;
    info.flags      = 0;
    info.queryType  = queryType;
    info.queryCount = MaxNumQueryCountPerPool;
    info.pipelineStatistics = 0;
    
    if (queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
      info.pipelineStatistics
        = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
    }
    
    if (m_vkd->vkCreateQueryPool(m_vkd->device(), &info, nullptr, &m_queryPool) != VK_SUCCESS)
      Logger::err("DxvkQueryPool: Failed to create query pool");
  }
  
  
  DxvkQueryPool::~DxvkQueryPool() {
    m_vkd->vkDestroyQueryPool(
      m_vkd->device(), m_queryPool, nullptr);
  }
  
  
  DxvkQueryHandle DxvkQueryPool::allocQuery(const DxvkQueryRevision& query) {
    const DxvkQueryHandle result = { m_queryPool, m_queryId };
    
    query.query->associateQuery(query.revision, result);
    m_queries.at(m_queryId) = query;
    m_queryId += 1;
    return result;
  }
  
  
  VkResult DxvkQueryPool::getData(
          uint32_t          queryIndex,
          uint32_t          queryCount) {
    std::array<DxvkQueryData, MaxNumQueryCountPerPool> results;
    
    const VkResult status = m_vkd->vkGetQueryPoolResults(
      m_vkd->device(), m_queryPool, queryIndex, queryCount,
      sizeof(DxvkQueryData) * MaxNumQueryCountPerPool,
      results.data(), sizeof(DxvkQueryData),
      VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    
    if (status != VK_SUCCESS)
      return status;
    
    for (uint32_t i = 0; i < queryCount; i++) {
      const DxvkQueryRevision& query = m_queries.at(queryIndex + i);
      query.query->updateData(query.revision, results.at(i));
    }
    
    return VK_SUCCESS;
  }
  
}