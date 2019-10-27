#include "Renderer.h"


Renderer::Renderer(unsigned int width, unsigned int height) {
	//depth buffer
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//backface culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

	//Entity shaders and program
	std::string _vertexShaderSource = readShader("shaders/vertexShaderEntities.txt");
	std::string _fragmentShaderSource = readShader("shaders/fragmentShaderEntities.txt");
	g_EntityProgramId = createShaderProgram(_vertexShaderSource, _fragmentShaderSource, 0);
	//Terrain shaders and program
	_vertexShaderSource = readShader("shaders/vertexShaderTerrain.txt");
	_fragmentShaderSource = readShader("shaders/fragmentShaderTerrain.txt");
	g_TerrainProgramId = createShaderProgram(_vertexShaderSource, _fragmentShaderSource, 2);
	//Skybox shaders and program
	_vertexShaderSource = readShader("shaders/vertexShaderSkybox.txt");
	_fragmentShaderSource = readShader("shaders/fragmentShaderSkybox.txt");
	g_SkyboxProgramId = createShaderProgram(_vertexShaderSource, _fragmentShaderSource, 4);

	projectionMatrix = glm::perspective(glm::radians(FOV), (float)width / (float)height, 0.1f, 150.0f);

	//load skybox
	this->skyboxVBO = Entity::storeDataInAttributeList(0, this->VERTICES.size(), this->VERTICES);

	//read textures
	std::vector<std::string> files = { "gameFiles/Skybox/right.png", "gameFiles/Skybox/left.png", 
		"gameFiles/Skybox/top.png", "gameFiles/Skybox/bottom.png", "gameFiles/Skybox/back.png", "gameFiles/Skybox/front.png" };
	this->cubeMapTextureId = loadCubeMapTexture(files);

	std::cout << "EntProgram ID: " << g_EntityProgramId << std::endl;
	std::cout << "TerProgram ID: " << g_TerrainProgramId << std::endl;
}

void Renderer::render(glm::vec3& light, Camera& camera) {
	glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//RENDER ENTITIES
	glUseProgram(g_EntityProgramId);
	this->loadUniforms(g_EntityProgramId, light, camera);

	for (unsigned int i = 0; i < entities.size(); i++) {
		Entity* obj = &entities[i];
		//load uniform for model matrix
		int modelMatrixId = glGetUniformLocation(g_EntityProgramId, "modelMatrix");
		glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, &obj->modelMatrix[0][0]);
		//bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, obj->textureId);
		//bind vao
		glBindVertexArray(obj->VAO);
		//render
		glDrawElements(GL_TRIANGLES, obj->indexBufferSize, GL_UNSIGNED_INT, 0);
		//unbind
		glBindVertexArray(0);
		/*glBindBuffer(GL_ARRAY_BUFFER, obj->vertVBOId);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, obj->indexBufferSize);
		glBindBuffer(GL_ARRAY_BUFFER, 0);*/
	}

	//RENDER TERRAINS
	glUseProgram(g_TerrainProgramId);

	this->loadUniforms(g_TerrainProgramId, light, camera);
	//load uniform for model matrix
	int modelMatrixId = glGetUniformLocation(g_TerrainProgramId, "modelMatrix");
	glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, &terrain.modelMatrix[0][0]);
	//bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrain.textureId);
	//bind vao
	glBindVertexArray(terrain.VAO);
	//render
	glDrawElements(GL_TRIANGLES, terrain.indexBufferSize, GL_UNSIGNED_INT, 0);
	//unbind
	glBindVertexArray(0);

	glUseProgram(0);
	
	//RENDER SKYBOX
	{
		glUseProgram(g_SkyboxProgramId);
		glm::mat4 skyboxViewMatrix = camera.getViewMatrix();
		skyboxViewMatrix[3][0] = 0;
		skyboxViewMatrix[3][1] = 0;		//so player never moves closer to skybox
		skyboxViewMatrix[3][2] = 0;
		int projMatrixId = glGetUniformLocation(g_SkyboxProgramId, "projMatrix");
		int viewMatrixId = glGetUniformLocation(g_SkyboxProgramId, "viewMatrix");
		glUniformMatrix4fv(projMatrixId, 1, GL_FALSE, &projectionMatrix[0][0]);
		glUniformMatrix4fv(viewMatrixId, 1, GL_FALSE, &skyboxViewMatrix[0][0]);
		//bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, this->cubeMapTextureId);

		glBindBuffer(GL_ARRAY_BUFFER, this->skyboxVBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, this->VERTICES.size());

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glUseProgram(0);
	}

}

void Renderer::update() {
	for (Entity e : this->entities)
		e.update();
}

void Renderer::processEntity(Entity e) {
	//should sort by texture/model to speedup rendering
	this->entities.push_back(e); 
}

void Renderer::loadUniforms(unsigned int _pid, glm::vec3& light, Camera& camera) {
	//load uniforms (same for all obj's)
	int projMatrixId = glGetUniformLocation(_pid, "projMatrix");
	int viewMatrixId = glGetUniformLocation(_pid, "viewMatrix");
	int lightPositionId = glGetUniformLocation(_pid, "lightPosition");
	glUniformMatrix4fv(projMatrixId, 1, GL_FALSE, &projectionMatrix[0][0]);
	glUniformMatrix4fv(viewMatrixId, 1, GL_FALSE, &camera.getViewMatrix()[0][0]);
	glUniform3f(lightPositionId, light[0], light[1], light[2]);
}

void Renderer::cleanUp() {
	for (Entity e : this->entities) {
		//this should be handled by a loader class that only loads obj's if unique (not implemented yet)
		//delete VBOs
		glDeleteBuffers(1, &e.vertVBOId);
		glDeleteBuffers(1, &e.normVBOId);
		glDeleteBuffers(1, &e.texVBOId);
		//delete VAO
		glDeleteVertexArrays(1, &e.VAO);
	}
	entities.clear(); //calls deconstructor in all entities

	for (unsigned int _id : shaderIds)
		glDeleteShader(_id);
	glDeleteProgram(g_EntityProgramId);
	glDeleteProgram(g_TerrainProgramId);
	glDeleteProgram(g_SkyboxProgramId);
}

Terrain* Renderer::getTerrain() {
	return &this->terrain;
}

unsigned int Renderer::createShader(unsigned int type, const std::string& source) {
	unsigned int _id = glCreateShader(type);

	auto _source = source.c_str();
	glShaderSource(_id, 1, &_source, 0);

	glCompileShader(_id);

	int _status = false;
	glGetShaderiv(_id, GL_COMPILE_STATUS, &_status);

	//invalid shader print error
	if (_status == GL_FALSE) {
		int _length = 0;
		glGetShaderiv(_id, GL_INFO_LOG_LENGTH, &_length);

		std::vector<char> _info(_length);
		glGetShaderInfoLog(_id, _length, &_length, _info.data());

		std::cout << std::string(_info.begin(), _info.end()) << std::endl;

		glDeleteShader(_id);
		return 0;
	}

	return _id;
}

unsigned int Renderer::createShaderProgram(const std::string& vertexShaderSource,
	const std::string& fragmentShaderSource, unsigned int index) {

	unsigned int vertexShaderId = createShader(GL_VERTEX_SHADER, vertexShaderSource);
	unsigned int fragmentShaderId = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	shaderIds[index] = vertexShaderId;
	shaderIds[index + 1] = fragmentShaderId;

	if (vertexShaderId && fragmentShaderId) {
		unsigned int _programId = glCreateProgram();

		glAttachShader(_programId, vertexShaderId);
		glAttachShader(_programId, fragmentShaderId);

		glLinkProgram(_programId);

		int _status = 0;
		glGetProgramiv(_programId, GL_LINK_STATUS, &_status);

		//couldnt link shaders
		if (_status == GL_FALSE) {
			int _length = 0;
			glGetProgramiv(_programId, GL_INFO_LOG_LENGTH, &_length);

			std::vector<char> _info(_length);
			glGetProgramInfoLog(_programId, _length, &_length, _info.data());

			std::cout << std::string(_info.begin(), _info.end()) << std::endl;

			glDeleteShader(vertexShaderId);
			glDeleteShader(fragmentShaderId);
			return 0;
		}

		glDetachShader(_programId, vertexShaderId);
		glDetachShader(_programId, fragmentShaderId);

		return _programId;
	}

	return 0;
}