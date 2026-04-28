#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <learnopengl/assimp_glm_helpers.h>

struct KeyPosition
{
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    float timeStamp;
};

class Bone
{
public:
    Bone(const std::string& name, int ID, const aiNodeAnim* channel)
        :
        m_Name(name),
        m_ID(ID),
        m_LocalTransform(1.0f),
        m_NumPositions(0),
        m_NumRotations(0),
        m_NumScalings(0)
    {
        const unsigned int kMaxSafeKeys = 2048;
        if (!channel) return;

        // ================= POSITION =================
        if (channel->mPositionKeys && channel->mNumPositionKeys > 0 && channel->mNumPositionKeys <= kMaxSafeKeys)
        {
            m_NumPositions = static_cast<int>(channel->mNumPositionKeys);
            for (int i = 0; i < m_NumPositions; ++i)
            {
                if (!channel->mPositionKeys) break;
                try
                {
                    const aiVector3D& aiPosition = channel->mPositionKeys[i].mValue;
                    float timeStamp = static_cast<float>(channel->mPositionKeys[i].mTime);

                    KeyPosition data;
                    data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
                    data.timeStamp = timeStamp;
                    m_Positions.push_back(data);
                }
                catch (...)
                {
                    break;
                }
            }
        }

        // ================= ROTATION =================
        if (channel->mRotationKeys && channel->mNumRotationKeys > 0 && channel->mNumRotationKeys <= kMaxSafeKeys)
        {
            m_NumRotations = static_cast<int>(channel->mNumRotationKeys);
            for (int i = 0; i < m_NumRotations; ++i)
            {
                if (!channel->mRotationKeys) break;
                try
                {
                    const aiQuaternion& aiOrientation = channel->mRotationKeys[i].mValue;
                    float timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);

                    KeyRotation data;
                    data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
                    data.timeStamp = timeStamp;
                    m_Rotations.push_back(data);
                }
                catch (...)
                {
                    break;
                }
            }
        }

        // ================= SCALE =================
        if (channel->mScalingKeys && channel->mNumScalingKeys > 0 && channel->mNumScalingKeys <= kMaxSafeKeys)
        {
            m_NumScalings = static_cast<int>(channel->mNumScalingKeys);
            for (int i = 0; i < m_NumScalings; ++i)
            {
                if (!channel->mScalingKeys) break;
                try
                {
                    const aiVector3D& scale = channel->mScalingKeys[i].mValue;
                    float timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);

                    KeyScale data;
                    data.scale = AssimpGLMHelpers::GetGLMVec(scale);
                    data.timeStamp = timeStamp;
                    m_Scales.push_back(data);
                }
                catch (...)
                {
                    break;
                }
            }
        }
    }

    void Update(float animationTime)
    {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        m_LocalTransform = translation * rotation * scale;
    }

    glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() const { return m_ID; }

private:
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
    {
        float diff = nextTimeStamp - lastTimeStamp;
        if (diff == 0.0f) return 0.0f;
        return (animationTime - lastTimeStamp) / diff;
    }

    int GetPositionIndex(float animationTime) const
    {
        for (int index = 0; index < m_NumPositions - 1; ++index)
        {
            if (animationTime < m_Positions[index + 1].timeStamp)
                return index;
        }
        return m_NumPositions > 1 ? m_NumPositions - 2 : 0;
    }

    int GetRotationIndex(float animationTime) const
    {
        for (int index = 0; index < m_NumRotations - 1; ++index)
        {
            if (animationTime < m_Rotations[index + 1].timeStamp)
                return index;
        }
        return m_NumRotations > 1 ? m_NumRotations - 2 : 0;
    }

    int GetScaleIndex(float animationTime) const
    {
        for (int index = 0; index < m_NumScalings - 1; ++index)
        {
            if (animationTime < m_Scales[index + 1].timeStamp)
                return index;
        }
        return m_NumScalings > 1 ? m_NumScalings - 2 : 0;
    }

    glm::mat4 InterpolatePosition(float animationTime)
    {
        if (m_NumPositions == 0) return glm::mat4(1.0f);
        if (m_NumPositions == 1)
            return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

        int p0 = GetPositionIndex(animationTime);
        int p1 = p0 + 1;

        float factor = GetScaleFactor(
            m_Positions[p0].timeStamp,
            m_Positions[p1].timeStamp,
            animationTime
        );

        glm::vec3 finalPos = glm::mix(m_Positions[p0].position, m_Positions[p1].position, factor);
        return glm::translate(glm::mat4(1.0f), finalPos);
    }

    glm::mat4 InterpolateRotation(float animationTime)
    {
        if (m_NumRotations == 0) return glm::mat4(1.0f);
        if (m_NumRotations == 1)
            return glm::toMat4(glm::normalize(m_Rotations[0].orientation));

        int p0 = GetRotationIndex(animationTime);
        int p1 = p0 + 1;

        float factor = GetScaleFactor(
            m_Rotations[p0].timeStamp,
            m_Rotations[p1].timeStamp,
            animationTime
        );

        glm::quat rot = glm::slerp(m_Rotations[p0].orientation, m_Rotations[p1].orientation, factor);
        return glm::toMat4(glm::normalize(rot));
    }

    glm::mat4 InterpolateScaling(float animationTime)
    {
        if (m_NumScalings == 0) return glm::mat4(1.0f);
        if (m_NumScalings == 1)
            return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

        int p0 = GetScaleIndex(animationTime);
        int p1 = p0 + 1;

        float factor = GetScaleFactor(
            m_Scales[p0].timeStamp,
            m_Scales[p1].timeStamp,
            animationTime
        );

        glm::vec3 scale = glm::mix(m_Scales[p0].scale, m_Scales[p1].scale, factor);
        return glm::scale(glm::mat4(1.0f), scale);
    }

private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;

    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;
};
