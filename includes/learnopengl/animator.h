#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "animation.h"
#include "bone.h"

class Animator
{
public:
	Animator(Animation* animation)
	{
		m_CurrentTime = 0.0;
		m_CurrentAnimation = animation;

		m_FinalBoneMatrices.reserve(100);

		for (int i = 0; i < 100; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	void UpdateAnimation(float dt)
	{
		m_DeltaTime = dt;
		if (m_CurrentAnimation)
		{
			const float ticksPerSecond = m_CurrentAnimation->GetTicksPerSecond();
			const float duration = m_CurrentAnimation->GetDuration();
			if (ticksPerSecond <= 0.0f || duration <= 0.0f)
				return;
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
			m_CurrentTime = fmod(m_CurrentTime, duration);
			CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
		}
	}

	void PlayAnimation(Animation* pAnimation)
	{
		m_CurrentAnimation = pAnimation;
		m_CurrentTime = 0.0f;
	}

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
	{
		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;

		Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

		if (Bone)
		{
			Bone->Update(m_CurrentTime);
			nodeTransform = Bone->GetLocalTransform();
		}

		glm::mat4 globalTransformation = parentTransform * nodeTransform;

		const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
		auto it = boneInfoMap.find(nodeName);
		if (it == boneInfoMap.end())
		{
			const std::string nodeKey = NormalizeAnimBoneKey(nodeName);
			if (!nodeKey.empty())
			{
				for (auto mapIt = boneInfoMap.begin(); mapIt != boneInfoMap.end(); ++mapIt)
				{
					if (AnimBoneKeyMatches(NormalizeAnimBoneKey(mapIt->first), nodeKey))
					{
						it = mapIt;
						break;
					}
				}
			}
		}
		if (it != boneInfoMap.end())
		{
			int index = it->second.id;
			glm::mat4 offset = it->second.offset;
			// Convert from scene space into the mesh's bind-pose space.
			m_FinalBoneMatrices[index] = m_CurrentAnimation->GetGlobalInverseTransform() * globalTransformation * offset;
		}

		for (int i = 0; i < node->childrenCount; i++)
			CalculateBoneTransform(&node->children[i], globalTransformation);
	}

	std::vector<glm::mat4> GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}

private:
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation* m_CurrentAnimation;
	float m_CurrentTime;
	float m_DeltaTime;

};
