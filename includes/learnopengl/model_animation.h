#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <learnopengl/assimp_glm_helpers.h>
#include <learnopengl/animdata.h>
#include <learnopengl/anim_name_utils.h>

using namespace std;

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
	
	

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
	void Draw(Shader &shader)
	{
		for(unsigned int i = 0; i < meshes.size(); i++)
			meshes[i].Draw(shader);
	}
    
	auto& GetBoneInfoMap() { return m_BoneInfoMap; }
	const auto& GetBoneInfoMap() const { return m_BoneInfoMap; }
	int& GetBoneCount() { return m_BoneCounter; }
	int GetBoneCount() const { return m_BoneCounter; }
	

private:

	std::map<string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

	void SetVertexBoneDataToDefault(Vertex& vertex)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.m_BoneIDs[i] = -1;
			vertex.m_Weights[i] = 0.0f;
		}
	}


	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			SetVertexBoneDataToDefault(vertex);
			vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
			vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
			
			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		ExtractBoneWeightForVertices(vertices,mesh,scene);

		return Mesh(vertices, indices, textures);
	}

	void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
		// Ignore non-influential weights. Some importers/exporters include explicit zero-weight entries
		// which would otherwise occupy the limited influence slots and prevent real weights from being stored.
		if (weight <= 0.0f)
			return;

		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_BoneIDs[i] < 0 || vertex.m_Weights[i] == 0.0f)
			{
				vertex.m_Weights[i] = weight;
				vertex.m_BoneIDs[i] = boneID;
				return;
			}
		}

		// If all slots are taken, replace the smallest weight if the new one is larger.
		int minIndex = 0;
		for (int i = 1; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_Weights[i] < vertex.m_Weights[minIndex])
				minIndex = i;
		}
		if (vertex.m_Weights[minIndex] < weight)
		{
			vertex.m_Weights[minIndex] = weight;
			vertex.m_BoneIDs[minIndex] = boneID;
		}
	}


	void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
		auto& boneInfoMap = m_BoneInfoMap;
		int& boneCount = m_BoneCounter;

		// Debug counters: helps detect models that contain a skeleton but no actual skin weights.
		// (Seen in some exporter pipelines where animation is on nodes, not on a skinned mesh.)
		std::size_t assignedInfluences = 0;
		std::size_t nonZeroWeights = 0;
		float minAssignedWeight = 1e9f;
		float maxAssignedWeight = -1e9f;

		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = SanitizeAnimNodeName(mesh->mBones[boneIndex]->mName.C_Str());
			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = boneCount;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				boneInfoMap[boneName] = newBoneInfo;
				boneID = boneCount;
				boneCount++;
			}
			else
			{
				boneID = boneInfoMap[boneName].id;
			}
			assert(boneID != -1);
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				// Assimp's mVertexId is an index into the mesh's vertex array.
				assert(vertexId >= 0 && static_cast<std::size_t>(vertexId) < vertices.size());
				SetVertexBoneData(vertices[vertexId], boneID, weight);
				if (weight > 0.0f)
				{
					assignedInfluences++;
					if (weight < minAssignedWeight) minAssignedWeight = weight;
					if (weight > maxAssignedWeight) maxAssignedWeight = weight;
				}
				if (weight != 0.0f)
					nonZeroWeights++;
			}
		}

		// Normalize per-vertex weights so they sum to 1.0 (when any influence exists).
		for (auto& v : vertices)
		{
			float sum = 0.0f;
			for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
				sum += (v.m_Weights[i] > 0.0f) ? v.m_Weights[i] : 0.0f;
			if (sum > 0.0f)
			{
				const float inv = 1.0f / sum;
				for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
					v.m_Weights[i] = (v.m_Weights[i] > 0.0f) ? (v.m_Weights[i] * inv) : 0.0f;
			}
		}

		if (mesh->mNumBones > 0 && assignedInfluences == 0)
		{
			std::cerr << "WARN: mesh has " << mesh->mNumBones
			          << " bones but 0 positive vertex weights"
			          << " (nonZeroWeights=" << nonZeroWeights << ").\n";
		}
	}


	unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false)
    {
    	string filename = string(path);

    	// 🔥 ตัด path เพี้ยน เอาแค่ชื่อไฟล์
    	size_t slash = filename.find_last_of("/\\");
    	if (slash != string::npos)
    		filename = filename.substr(slash + 1);

    	// 🔥 ต่อกับ directory ใหม่
    	filename = directory + '/' + filename;

    	std::cout << "Loading texture: " << filename << std::endl;

    	unsigned int textureID;
    	glGenTextures(1, &textureID);

    	int width, height, nrComponents;
    	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

    	if (data)
    	{
    		GLenum format;
    		if (nrComponents == 1) format = GL_RED;
    		else if (nrComponents == 3) format = GL_RGB;
    		else if (nrComponents == 4) format = GL_RGBA;

    		glBindTexture(GL_TEXTURE_2D, textureID);
    		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    		glGenerateMipmap(GL_TEXTURE_2D);

    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    		stbi_image_free(data);
    	}
    	else
	    	{
	    		std::cout << "Texture failed to load at path: " << filename << " | using fallback 1x1 white texture" << std::endl;
	    		glBindTexture(GL_TEXTURE_2D, textureID);
	    		unsigned char fallbackPixel[4] = {255, 255, 255, 255};
	    		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixel);
	    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    		stbi_image_free(data);
	    	}

    	return textureID;
    }
    
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }
};



#endif
