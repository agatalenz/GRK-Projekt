#include "Render_Utils.h"

#include <algorithm>

#include "glew.h"
#include "freeglut.h"

void Core::RenderContext::initFromOBJ(obj::Model& model)
{
	vertexArray = 0;
	vertexBuffer = 0;
	vertexIndexBuffer = 0;
	unsigned int vertexDataBufferSize = sizeof(float) * model.vertex.size();
	unsigned int vertexNormalBufferSize = sizeof(float) * model.normal.size();
	unsigned int vertexTexBufferSize = sizeof(float) * model.texCoord.size();

	size = model.faces["default"].size();
	unsigned int vertexElementBufferSize = sizeof(unsigned short) * size;


	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);


	glGenBuffers(1, &vertexIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexElementBufferSize, &model.faces["default"][0], GL_STATIC_DRAW);

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBufferData(GL_ARRAY_BUFFER, vertexDataBufferSize + vertexNormalBufferSize + vertexTexBufferSize, NULL, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataBufferSize, &model.vertex[0]);

	glBufferSubData(GL_ARRAY_BUFFER, vertexDataBufferSize, vertexNormalBufferSize, &model.normal[0]);

	glBufferSubData(GL_ARRAY_BUFFER, vertexDataBufferSize + vertexNormalBufferSize, vertexTexBufferSize, &model.texCoord[0]);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)(vertexDataBufferSize));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(vertexNormalBufferSize + vertexDataBufferSize));
}

void Core::DrawVertexArray(const float * vertexArray, int numVertices, int elementSize )
{
	glVertexAttribPointer(0, elementSize, GL_FLOAT, false, 0, vertexArray);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, numVertices);
}

void Core::DrawVertexArrayIndexed( const float * vertexArray, const int * indexArray, int numIndexes, int elementSize )
{
	glVertexAttribPointer(0, elementSize, GL_FLOAT, false, 0, vertexArray);
	glEnableVertexAttribArray(0);

	glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, indexArray);
}


void Core::DrawVertexArray( const VertexData & data )
{
	int numAttribs = std::min(VertexData::MAX_ATTRIBS, data.NumActiveAttribs);
	for(int i = 0; i < numAttribs; i++)
	{
		glVertexAttribPointer(i, data.Attribs[i].Size, GL_FLOAT, false, 0, data.Attribs[i].Pointer);
		glEnableVertexAttribArray(i);
	}
	glDrawArrays(GL_TRIANGLES, 0, data.NumVertices);
}

void Core::DrawModel( obj::Model * model )
{
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, &model->vertex[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, &model->texCoord[0]);
	glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, &model->normal[0]);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	unsigned short * tmp = &model->faces["default"][0];
	glDrawElements(GL_TRIANGLES, model->faces["default"].size(), GL_UNSIGNED_SHORT, tmp);
}
void Core::DrawContext(Core::RenderContext& context)
{

	glBindVertexArray(context.vertexArray);
	glDrawElements(
		GL_TRIANGLES,      // mode
		context.size,    // count
		GL_UNSIGNED_SHORT,   // type
		(void*)0           // element array buffer offset
	);
	glBindVertexArray(0);
}

void Core::RayContext::render()
{

	glBindVertexArray(vertexArray);
	glDrawArrays(
		GL_LINES,// mode
		0,     //start
		3 * 7 * 2     // count
	);
	glBindVertexArray(0);
}


void Core::initRay(RayContext& rayContext) {
	rayContext.size = 2;
	glGenVertexArrays(1, &rayContext.vertexArray);
	glBindVertexArray(rayContext.vertexArray);

	glGenBuffers(1, &rayContext.vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, rayContext.vertexBuffer);

	glBufferData(GL_ARRAY_BUFFER, 3 * 7 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
	glBindVertexArray(0);
}

void Core::updateRayPos(RayContext& rayContext, std::vector<glm::vec3> ray) {
	glBindVertexArray(rayContext.vertexArray);
	std::vector<glm::vec3> keyPoints;
	float offset = 0.f;
	float scale = 0.01f;
	float rayEnd = 50.f;
	keyPoints.push_back(ray[0] + ray[1] * offset);
	keyPoints.push_back(ray[0] + ray[1] * rayEnd);

	keyPoints.push_back(ray[0] + ray[1] * offset + scale * glm::vec3(1.f, 1.f, 0.f));
	keyPoints.push_back(ray[0] + ray[1] * offset - scale * glm::vec3(1.f, 1.f, 0.f));
	keyPoints.push_back(ray[0] + ray[1] * offset + scale * glm::vec3(1.f, -1.f, 0.f));
	keyPoints.push_back(ray[0] + ray[1] * offset - scale * glm::vec3(1.f, -1.f, 0.f));

	//keyPoints.push_back(ray[0] + ray[1] * offset + scale * glm::vec3(1.f, 1.f, 0.f));
	//keyPoints.push_back(ray[0] + ray[1] * rayEnd * scale);
	//keyPoints.push_back(ray[0] + ray[1] * offset - scale * glm::vec3(1.f, 1.f, 0.f));
	//keyPoints.push_back(ray[0] + ray[1] * rayEnd * scale);
	//keyPoints.push_back(ray[0] + ray[1] * offset + scale * glm::vec3(1.f, -1.f, 0.f));
	//keyPoints.push_back(ray[0] + ray[1] * rayEnd * scale);
	//keyPoints.push_back(ray[0] + ray[1] * offset - scale * glm::vec3(1.f, -1.f, 0.f));
	//keyPoints.push_back(ray[0] + ray[1] * rayEnd * scale);
	glBindBuffer(GL_ARRAY_BUFFER, rayContext.vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, keyPoints.size() * 3 * sizeof(float), &keyPoints[0]);
	glBindVertexArray(0);
}
