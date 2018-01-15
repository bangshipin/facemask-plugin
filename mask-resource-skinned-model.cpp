/*
* Face Masks for SlOBS
* Copyright (C) 2017 General Workings Inc
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "mask-resource-skinned-model.h"
#include "exceptions.h"
#include "plugin.h"
#include "utils.h"
extern "C" {
	#pragma warning( push )
	#pragma warning( disable: 4201 )
	#include <libobs/util/platform.h>
	#include <libobs/obs-module.h>
	#pragma warning( pop )
}
#include <sstream>

static const char* const S_MATERIAL = "material";



Mask::Resource::SkinnedModel::SkinnedModel(Mask::MaskData* parent, std::string name, obs_data_t* data)
	: IBase(parent, name) {

	// Material
	if (!obs_data_has_user_value(data, S_MATERIAL)) {
		PLOG_ERROR("Skinned Model '%s' has no material.", name.c_str());
		throw std::logic_error("Skinned Model has no material.");
	}
	std::string materialName = obs_data_get_string(data, S_MATERIAL);
	m_material = std::dynamic_pointer_cast<Material>(m_parent->GetResource(materialName));
	if (m_material == nullptr) {
		PLOG_ERROR("<Skinned Model '%s'> Dependency on material '%s' could not be resolved.",
			m_name.c_str(), materialName.c_str());
		throw std::logic_error("Model depends on non-existing material.");
	}
	if (m_material->GetType() != Type::Material) {
		PLOG_ERROR("<Skinned Model '%s'> Resolved material dependency on '%s' is not a material.",
			m_name.c_str(), materialName.c_str());
		throw std::logic_error("Material dependency of Model is not a material.");
	}
}

Mask::Resource::SkinnedModel::~SkinnedModel() {}

Mask::Resource::Type Mask::Resource::SkinnedModel::GetType() {
	return Mask::Resource::Type::Model;
}

void Mask::Resource::SkinnedModel::Update(Mask::Part* part, float time) {
	part->mask->instanceDatas.Push(m_id);
	m_material->Update(part, time);
	m_mesh->Update(part, time);
	part->mask->instanceDatas.Pop();
}

void Mask::Resource::SkinnedModel::Render(Mask::Part* part) {
	// Add transparent models as sorted draw objects
	// to draw in a sorted second render pass 
	if (!IsOpaque()) {
		this->sortDrawPart = part;
		part->mask->AddSortedDrawObject(this);
		return;
	}
	DirectRender(part);
}

void Mask::Resource::SkinnedModel::DirectRender(Mask::Part* part) {
	part->mask->instanceDatas.Push(m_id);
	while (m_material->Loop(part)) {
		m_mesh->Render(part);
	}
	part->mask->instanceDatas.Pop();
}


bool Mask::Resource::SkinnedModel::IsDepthOnly() {
	if (m_material != nullptr) {
		return m_material->IsDepthOnly();
	}
	return false;
}

bool Mask::Resource::SkinnedModel::IsOpaque() {
	// note: depth only objects are considered opaque
	if (IsDepthOnly()) {
		return true;
	}
	if (m_material != nullptr) {
		return m_material->IsOpaque();
	}
	return true;
}

float Mask::Resource::SkinnedModel::SortDepth() {
	vec4 c = m_mesh->GetCenter();
	matrix4 m;
	gs_matrix_get(&m);
	vec4_transform(&c, &c, &m);
	return c.z;
}
	
void Mask::Resource::SkinnedModel::SortedRender() {
	sortDrawPart->mask->instanceDatas.Push(m_id);
	while (m_material->Loop(sortDrawPart)) {
		m_mesh->Render(sortDrawPart);
	}
	sortDrawPart->mask->instanceDatas.Pop();
}

