#include "hzpch.h"
#include "MeshFactory.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Hazel {

	Ref<Mesh> MeshFactory::CreateBox(const glm::vec3& size)
	{
		std::vector<Vertex> vertices;
		vertices.resize(8);
		vertices[0].Position = { -size.x / 2.0F, -size.y / 2.0F,  size.z / 2.0F };
		vertices[1].Position = {  size.x / 2.0F, -size.y / 2.0F,  size.z / 2.0F };
		vertices[2].Position = {  size.x / 2.0F,  size.y / 2.0F,  size.z / 2.0F };
		vertices[3].Position = { -size.x / 2.0F,  size.y / 2.0F,  size.z / 2.0F };
		vertices[4].Position = { -size.x / 2.0F, -size.y / 2.0F, -size.z / 2.0F };
		vertices[5].Position = {  size.x / 2.0F, -size.y / 2.0F, -size.z / 2.0F };
		vertices[6].Position = {  size.x / 2.0F,  size.y / 2.0F, -size.z / 2.0F };
		vertices[7].Position = { -size.x / 2.0F,  size.y / 2.0F, -size.z / 2.0F };

		vertices[0].Normal = { -1.0F, -1.0F,  1.0F };
		vertices[1].Normal = {  1.0F, -1.0F,  1.0F };
		vertices[2].Normal = {  1.0F,  1.0F,  1.0F };
		vertices[3].Normal = { -1.0F,  1.0F,  1.0F };
		vertices[4].Normal = { -1.0F, -1.0F, -1.0F };
		vertices[5].Normal = {  1.0F, -1.0F, -1.0F };
		vertices[6].Normal = {  1.0F,  1.0F, -1.0F };
		vertices[7].Normal = { -1.0F,  1.0F, -1.0F };

		std::vector<Index> indices;
		indices.resize(12);
		indices[0] =  { 0, 1, 2 };
		indices[1] =  { 2, 3, 0 };
		indices[2] =  { 1, 5, 6 };
		indices[3] =  { 6, 2, 1 };
		indices[4] =  { 7, 6, 5 };
		indices[5] =  { 5, 4, 7 };
		indices[6] =  { 4, 0, 3 };
		indices[7] =  { 3, 7, 4 };
		indices[8] =  { 4, 5, 1 };
		indices[9] =  { 1, 0, 4 };
		indices[10] = { 3, 2, 6 };
		indices[11] = { 6, 7, 3 };

		return Ref<Mesh>::Create(vertices, indices);
	}

	Hazel::Ref<Hazel::Mesh> MeshFactory::CreateSphere(float radius)
	{
		std::vector<Vertex> vertices;
		std::vector<Index> indices;

		constexpr float latitudeBands = 30;
		constexpr float longitudeBands = 30;

		for (float latitude = 0.0F; latitude <= latitudeBands; latitude++)
		{
			float theta = latitude * M_PI / latitudeBands;
			float sinTheta = glm::sin(theta);
			float cosTheta = glm::cos(theta);

			for (float longitude = 0.0F; longitude <= longitudeBands; longitude++)
			{
				float phi = longitude * 2 * M_PI / longitudeBands;
				float sinPhi = glm::sin(phi);
				float cosPhi = glm::cos(phi);

				Vertex vertex;
				vertex.Normal = { cosPhi * sinTheta, cosTheta, sinPhi * sinTheta };
				vertex.Position = { radius * vertex.Normal.x, radius * vertex.Normal.y, radius * vertex.Normal.z };
				vertices.push_back(vertex);
			}
		}

		for (uint32_t latitude = 0; latitude < latitudeBands; latitude++)
		{
			for (uint32_t longitude = 0; longitude < longitudeBands; longitude++)
			{
				uint32_t first = (latitude * (longitudeBands + 1)) + longitude;
				uint32_t second = first + longitudeBands + 1;

				indices.push_back({ first, second, first + 1 });
				indices.push_back({ second, second + 1, first + 1 });
			}
		}

		return Ref<Mesh>::Create(vertices, indices);
	}

	Ref<Mesh> MeshFactory::CreateCapsule(float radius, float height)
	{
		std::vector<Vertex> vertices;
		std::vector<Index> indices;

		constexpr int segments = 30;
		constexpr int pointCount = segments + 1;

		float pointsX[pointCount];
		float pointsY[pointCount];
		float pointsZ[pointCount];
		float pointsR[pointCount];

		float calcH = 0.0F;
		float calcV = 0.0F;

		for (int i = 0; i < pointCount; i++)
		{
			float calcHRadians = glm::radians(calcH);
			float calcVRadians = glm::radians(calcV);

			pointsX[i] = glm::sin(calcHRadians);
			pointsZ[i] = glm::cos(calcHRadians);
			pointsY[i] = glm::cos(calcVRadians);
			pointsR[i] = glm::sin(calcVRadians);

			calcH += 360.0F / (float)segments;
			calcV += 180.0F / (float)segments;
		}

		float yOffset = (height - (radius * 2.0F)) * 0.5F;
		if (yOffset < 0.0F)
			yOffset = 0.0F;

		int top = glm::ceil(pointCount * 0.5F);

		for (int y = 0; y < top; y++)
		{
			for (int x = 0; x < pointCount; x++)
			{
				Vertex vertex;
				vertex.Position = glm::vec3( pointsX[x] * pointsR[y], pointsY[y] + yOffset, pointsZ[x] * pointsR[y] ) * radius;
				vertices.push_back(vertex);
			}
		}

		int bottom = glm::floor(pointCount * 0.5F);

		for (int y = bottom; y < pointCount; y++)
		{
			for (int x = 0; x < pointCount; x++)
			{
				Vertex vertex;
				vertex.Position = glm::vec3(pointsX[x] * pointsR[y], -yOffset + pointsY[y], pointsZ[x] * pointsR[y]) * radius;
				vertices.push_back(vertex);
			}
		}

		for (int y = 0; y < segments + 1; y++)
		{
			for (int x = 0; x < segments; x++)
			{
				Index index1;
				index1.V1 = ((y + 0) * (segments + 1)) + x + 0;
				index1.V2 = ((y + 1) * (segments + 1)) + x + 0;
				index1.V3 = ((y + 1) * (segments + 1)) + x + 1;
				indices.push_back(index1);

				Index index2;
				index2.V1 = ((y + 0) * (segments + 1)) + x + 1;
				index2.V2 = ((y + 0) * (segments + 1)) + x + 0;
				index2.V3 = ((y + 1) * (segments + 1)) + x + 1;
				indices.push_back(index2);
			}
		}

		return Ref<Mesh>::Create(vertices, indices);
	}

}