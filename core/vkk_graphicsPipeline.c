/*
 * Copyright (c) 2019 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_graphicsPipeline.h"
#include "vkk_pipelineLayout.h"
#include "vkk_renderer.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_graphicsPipeline_t*
vkk_graphicsPipeline_new(vkk_engine_t* engine,
                         vkk_graphicsPipelineInfo_t* gpi)
{
	ASSERT(engine);
	ASSERT(gpi);

	VkShaderModule vs;
	VkShaderModule fs;
	vs = vkk_engine_getShaderModule(engine, gpi->vs);
	fs = vkk_engine_getShaderModule(engine, gpi->fs);
	if((vs == VK_NULL_HANDLE) || (fs == VK_NULL_HANDLE))
	{
		return NULL;
	}

	vkk_graphicsPipeline_t* self;
	self = (vkk_graphicsPipeline_t*)
	       CALLOC(1, sizeof(vkk_graphicsPipeline_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}
	self->engine   = engine;
	self->renderer = gpi->renderer;
	self->pl       = gpi->pl;

	VkPipelineShaderStageCreateInfo pss_info[2] =
	{
		// vertex stage
		{
			.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext               = NULL,
			.flags               = 0,
			.stage               = VK_SHADER_STAGE_VERTEX_BIT,
			.module              = vs,
			.pName               = "main",
			.pSpecializationInfo = NULL
		},

		// fragment stage
		{
			.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext               = NULL,
			.flags               = 0,
			.stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module              = fs,
			.pName               = "main",
			.pSpecializationInfo = NULL
		}
	};

	VkVertexInputBindingDescription* vib;
	vib = (VkVertexInputBindingDescription*)
	      CALLOC(gpi->vb_count, sizeof(VkVertexInputBindingDescription));
	if(vib == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_vib;
	}

	VkVertexInputAttributeDescription* via;
	via = (VkVertexInputAttributeDescription*)
	      CALLOC(gpi->vb_count, sizeof(VkVertexInputAttributeDescription));
	if(via == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_via;
	}

	uint32_t stride[VKK_VERTEX_FORMAT_COUNT] =
	{
		sizeof(float),
		sizeof(int32_t),
		sizeof(int16_t),
		sizeof(uint32_t),
		sizeof(uint16_t)
	};

	VkFormat format[4*VKK_VERTEX_FORMAT_COUNT] =
	{
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16A16_UINT,
	};

	int i;
	for(i = 0; i < gpi->vb_count; ++i)
	{
		vkk_vertexBufferInfo_t* vbi= &(gpi->vbi[i]);
		vib[i].binding   = vbi->location;
		vib[i].stride    = stride[vbi->format]*vbi->components;
		vib[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		via[i].location  = vbi->location;
		via[i].binding   = vbi->location;
		via[i].format    = format[4*vbi->format +
		                          vbi->components - 1];
		via[i].offset    = 0;
	}

	VkPipelineVertexInputStateCreateInfo pvis_info =
	{
		.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext                           = NULL,
		.flags                           = 0,
		.vertexBindingDescriptionCount   = gpi->vb_count,
		.pVertexBindingDescriptions      = vib,
		.vertexAttributeDescriptionCount = gpi->vb_count,
		.pVertexAttributeDescriptions    = via
	};

	VkPrimitiveTopology topology[VKK_PRIMITIVE_TRIANGLE_COUNT] =
	{
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
	};
	VkPipelineInputAssemblyStateCreateInfo pias_info =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext                  = NULL,
		.flags                  = 0,
		.topology               = topology[gpi->primitive],
		.primitiveRestartEnable = gpi->primitive_restart
	};

	VkViewport viewport =
	{
		.x        = 0.0f,
		.y        = 0.0f,
		.width    = (float) 0.0,
		.height   = (float) 0.0,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor =
	{
		.offset =
		{
			.x = 0,
			.y = 0
		},
		.extent =
		{
			.width  = (uint32_t) 0,
			.height = (uint32_t) 0,
		}
	};

	VkPipelineViewportStateCreateInfo pvs_info =
	{
		.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext         = NULL,
		.flags         = 0,
		.viewportCount = 1,
		.pViewports    = &viewport,
		.scissorCount  = 1,
		.pScissors     = &scissor
	};

	VkCullModeFlags cullMode;
	cullMode = gpi->cull_back ? VK_CULL_MODE_BACK_BIT :
	                            VK_CULL_MODE_NONE;
	VkPipelineRasterizationStateCreateInfo prs_info =
	{
		.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.depthClampEnable        = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = VK_POLYGON_MODE_FILL,
		.cullMode                = cullMode,
		.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable         = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp          = 0.0f,
		.depthBiasSlopeFactor    = 0.0f,
		.lineWidth               = 1.0f
	};

	VkSampleCountFlagBits sample_count_flag_bits;
	sample_count_flag_bits = VK_SAMPLE_COUNT_4_BIT;
	if(vkk_renderer_msaaSampleCount(self->renderer) == 1)
	{
		sample_count_flag_bits = VK_SAMPLE_COUNT_1_BIT;
	}

	VkPipelineMultisampleStateCreateInfo pms_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.rasterizationSamples  = sample_count_flag_bits,
		.sampleShadingEnable   = VK_FALSE,
		.minSampleShading      = 0.0f,
		.pSampleMask           = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable      = VK_FALSE
	};

	VkPipelineDepthStencilStateCreateInfo pdss_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.depthTestEnable       = gpi->depth_test,
		.depthWriteEnable      = gpi->depth_write,
		.depthCompareOp        = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable     = VK_FALSE,
		.front =
		{
			.failOp      = VK_STENCIL_OP_KEEP,
			.passOp      = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp   = VK_COMPARE_OP_NEVER,
			.compareMask = 0,
			.writeMask   = 0,
			.reference   = 0
		},
		.back =
		{
			.failOp      = VK_STENCIL_OP_KEEP,
			.passOp      = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp   = VK_COMPARE_OP_NEVER,
			.compareMask = 0,
			.writeMask   = 0,
			.reference   = 0
		},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkPipelineColorBlendAttachmentState pcbs =
	{
		.blendEnable         = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp        = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp        = VK_BLEND_OP_MAX,
		.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
		                       VK_COLOR_COMPONENT_G_BIT |
		                       VK_COLOR_COMPONENT_B_BIT |
		                       VK_COLOR_COMPONENT_A_BIT,
	};

	if(gpi->blend_mode == VKK_BLEND_MODE_TRANSPARENCY)
	{
		pcbs.blendEnable = VK_TRUE;
	}

	VkPipelineColorBlendStateCreateInfo pcbs_info =
	{
		.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.logicOpEnable   = VK_FALSE,
		.logicOp         = VK_LOGIC_OP_CLEAR,
		.attachmentCount = 1,
		.pAttachments    = &pcbs,
		.blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	VkDynamicState dynamic_state[2] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo pds_info =
	{
		.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext             = NULL,
		.flags             = 0,
		.dynamicStateCount = 2,
		.pDynamicStates    = dynamic_state,
	};

	VkGraphicsPipelineCreateInfo gp_info =
	{
		.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext               = NULL,
		.flags               = 0,
		.stageCount          = 2,
		.pStages             = pss_info,
		.pVertexInputState   = &pvis_info,
		.pInputAssemblyState = &pias_info,
		.pTessellationState  = NULL,
		.pViewportState      = &pvs_info,
		.pRasterizationState = &prs_info,
		.pMultisampleState   = &pms_info,
		.pDepthStencilState  = &pdss_info,
		.pColorBlendState    = &pcbs_info,
		.pDynamicState       = &pds_info,
		.layout              = gpi->pl->pl,
		.renderPass          = vkk_renderer_renderPass(gpi->renderer),
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1
	};

	if(vkCreateGraphicsPipelines(engine->device,
	                             engine->pipeline_cache,
	                             1, &gp_info, NULL,
	                             &self->pipeline) != VK_SUCCESS)
	{
		LOGE("vkCreateGraphicsPipelines failed");
		goto fail_create;
	}

	FREE(via);
	FREE(vib);

	// success
	return self;

	// failure
	fail_create:
		FREE(via);
	fail_via:
		FREE(vib);
	fail_vib:
		FREE(self);
	return NULL;
}

void vkk_graphicsPipeline_delete(vkk_graphicsPipeline_t** _self)
{
	ASSERT(_self);

	vkk_graphicsPipeline_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_GRAPHICSPIPELINE,
		                        (void*) self);
		*_self = NULL;
	}
}
