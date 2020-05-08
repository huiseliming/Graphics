#pragma once
#include "D3D12Header.h"
#include "AssimpHeader.h"
struct BoundingBox
{
	Math::Vector3 Max;
	Math::Vector3 Min;
};

struct MeshData
{
	std::vector<DirectX::XMFLOAT3> Vertices;
	std::vector<uint32_t> Indices;
	void LoadFromFile(std::string file)
	{
		Assimp::Importer  importer;
		const auto pScene = importer.ReadFile(file,
			aiProcess_Triangulate |
			aiProcess_ConvertToLeftHanded //使用左手坐标系
			);
		const auto pMesh = pScene->mMeshes[0];
		Vertices.reserve(pMesh->mNumVertices);
		for (UINT i = 0; i < pMesh->mNumVertices; i++)
		{
			Vertices.push_back(*reinterpret_cast<DirectX::XMFLOAT3*>(&pMesh->mVertices[i]));
		}
		Indices.reserve(pMesh->mNumFaces * 3);
		for (UINT i = 0; i < pMesh->mNumFaces; i++)
		{
			const auto& Face = pMesh->mFaces[i];
			assert(Face.mNumIndices == 3);
			Indices.push_back(Face.mIndices[0]);
			Indices.push_back(Face.mIndices[1]);
			Indices.push_back(Face.mIndices[2]);
		}
	}

};


class Drawable
{
public:





private:

	MeshData m_MeshData;
	BoundingBox m_BoundBox;
};