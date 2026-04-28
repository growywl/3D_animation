#pragma once

#include <vector>
#include <map>
#include <cstdint>
#include <iostream>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "bone.h"
#include <functional>
#include <learnopengl/animdata.h>
#include "model_animation.h"
#include <learnopengl/anim_name_utils.h>

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

class Animation
{
public:
	Animation() = default;

	Animation(const std::string& animationPath, Model* model)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

		if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) {
			m_Duration = 0.0f;
			m_TicksPerSecond = 25.0f;
			return;
		}

		if (!model) {
			m_Duration = 0.0f;
			m_TicksPerSecond = 25.0f;
			return;
		}

		try
		{
			auto animation = scene->mAnimations[0];
			m_Duration = animation->mDuration;
			m_TicksPerSecond = animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f;
			aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
			globalTransformation = globalTransformation.Inverse();
			m_GlobalInverseTransform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(globalTransformation);
			ReadHierarchyData(m_RootNode, scene->mRootNode);
			ReadMissingBones(animation, *model);
		}
		catch (const std::exception& e)
		{
			std::cout << "Error loading animation: " << e.what() << std::endl;
			m_Duration = 0.0f;
			m_TicksPerSecond = 25.0f;
		}
		catch (...)
		{
			std::cout << "Unknown error loading animation" << std::endl;
			m_Duration = 0.0f;
			m_TicksPerSecond = 25.0f;
		}
	}

	~Animation()
	{
	}

	Bone* FindBone(const std::string& name)
	{
		const std::string sanitized = SanitizeAnimNodeName(name);
		const std::string key = NormalizeAnimBoneKey(sanitized);
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Bone& Bone)
			{
				return AnimBoneKeyMatches(NormalizeAnimBoneKey(Bone.GetBoneName()), key);
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return &(*iter);
	}

	inline std::size_t GetNumAnimatedBones() const { return m_Bones.size(); }
	
	inline float GetTicksPerSecond() const { return m_TicksPerSecond; }
	inline float GetDuration() const { return m_Duration; }
	inline const AssimpNodeData& GetRootNode() const { return m_RootNode; }
	inline const glm::mat4& GetGlobalInverseTransform() const { return m_GlobalInverseTransform; }
	inline const std::map<std::string,BoneInfo>& GetBoneIDMap() const
	{ 
		return m_BoneInfoMap;
	}

private:
	void ReadMissingBones(const aiAnimation* animation, Model& model)
	{
		if (!animation) return;

		int size = animation->mNumChannels;
		if (size <= 0) return;

		auto& boneInfoMap = model.GetBoneInfoMap();
		(void)model.GetBoneCount();

		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			if (!channel) continue;

			std::string boneName = SanitizeAnimNodeName(channel->mNodeName.data);

			auto it = boneInfoMap.find(boneName);
			if (it == boneInfoMap.end())
			{
				const std::string channelKey = NormalizeAnimBoneKey(boneName);
				if (!channelKey.empty())
				{
					for (auto mapIt = boneInfoMap.begin(); mapIt != boneInfoMap.end(); ++mapIt)
					{
						if (AnimBoneKeyMatches(NormalizeAnimBoneKey(mapIt->first), channelKey))
						{
							it = mapIt;
							boneName = mapIt->first;
							break;
						}
					}
				}
			}
			if (it == boneInfoMap.end()) continue;

			try
			{
				m_Bones.push_back(Bone(
					boneName,
					it->second.id,
					channel
				));
			}
			catch (const std::exception& e)
			{
				std::cout << "Skipped broken bone: " << channel->mNodeName.data << " - " << e.what() << std::endl;
			}
			catch (...)
			{
				std::cout << "Skipped broken bone: " << channel->mNodeName.data << std::endl;
			}
		}

		m_BoneInfoMap = boneInfoMap;
	}

	void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		dest.name = SanitizeAnimNodeName(src->mName.data);
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHierarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}
	float m_Duration;
	int m_TicksPerSecond;
	std::vector<Bone> m_Bones;
	AssimpNodeData m_RootNode;
	std::map<std::string, BoneInfo> m_BoneInfoMap;
	glm::mat4 m_GlobalInverseTransform{ 1.0f };
};
