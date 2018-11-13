#include "ShadowMapping.h"

#include <vector>
#include <string>
#include <iostream>

#include <Core/Engine.h>

using namespace std;

ShadowMapping::ShadowMapping()
{
}

ShadowMapping::~ShadowMapping()
{
}

void ShadowMapping::Init()
{
	lightMode = 1;
	renderDebugView = false;
	ToggleGroundPlane();

	TextureManager::LoadTexture(RESOURCE_PATH::TEXTURES, "ground.jpg");

	{
		Mesh* mesh = new Mesh("box");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "box.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("sphere");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "sphere.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("plane");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "plane50.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("quad");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "quad.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	// Create a shader program for drawing face polygon with the color of the normal
	{
		LoadShader("ShadowMapping");
		LoadShader("SimplePass");
		LoadShader("TextureDebug");
	}

	//Light & material properties
	{
		lightDirection = glm::vec3(0, -1, 0);
		materialShininess = 30;
		materialKd = 0.5;
		materialKs = 0.5;
	}

	shadowFBO = new FrameBuffer();
	shadowFBO->Generate(window->props.resolution.x, window->props.resolution.y, 0);

	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 1, 0));
		sceneObjects.push_back(new GameObject(meshes["sphere"], modelMatrix));
	}

	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(2, 0.5f, 0));
		modelMatrix = glm::rotate(modelMatrix, RADIANS(60.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));
		sceneObjects.push_back(new GameObject(meshes["box"], modelMatrix));
	}

	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-2, 0.5f, 0));
		modelMatrix = glm::rotate(modelMatrix, RADIANS(60.0f), glm::vec3(1, 1, 0));
		sceneObjects.push_back(new GameObject(meshes["box"], modelMatrix));
	}

	// Render ground
	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0.01f, 0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));
		sceneObjects.push_back(new GameObject(meshes["plane"], modelMatrix));
	}

	// Render the point light in the scene
	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f));
		lightObject = new GameObject(meshes["sphere"], modelMatrix);
	}

	{
		lightCamera = new Camera();
		lightCamera->SetPerspective(60, window->props.aspectRatio, 0.01f, 200);
		lightCamera->SetOrthographic(10, 10, 0.01f, 100);
		lightCamera->transform->SetWorldPosition(glm::vec3(0, 3, 1));
		lightCamera->transform->SetMoveSpeed(2);
		lightCamera->Update();

		lightCameraInput = new CameraInput(lightCamera);
		lightCameraInput->SetActive(false);
	}
}

void ShadowMapping::FrameStart()
{
	// clears the color buffer (using the previously set color) and depth buffer
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::ivec2 resolution = window->GetResolution();
	// sets the screen area where to draw
	glViewport(0, 0, resolution.x, resolution.y);	
}

void ShadowMapping::Update(float deltaTimeSeconds)
{
	lightObject->modelMatrix = lightCamera->transform->GetModel();

	{
		shadowFBO->Bind();

		auto shader = shaders["SimplePass"];
		shader->Use();

		glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(lightCamera->GetViewMatrix()));
		glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(lightCamera->GetProjectionMatrix()));

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		for (auto object : sceneObjects)
		{
			glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(object->modelMatrix));
			object->mesh->Render();
		}

		glDisable(GL_CULL_FACE);
	}

	// Render a debug quad for scene information
	FrameBuffer::BindDefault(window->GetResolution());
	{
		auto shader = shaders["ShadowMapping"];
		shader->Use();

		{
			int locTexture = shader->GetUniformLocation("texture_depth");
			glUniform1i(locTexture, 1);
			shadowFBO->BindDepthTexture(GL_TEXTURE1);
		}

		int locLightView = shader->GetUniformLocation("LightView");
		glUniformMatrix4fv(locLightView, 1, GL_FALSE, glm::value_ptr(lightCamera->GetViewMatrix()));

		int locLightProjection = shader->GetUniformLocation("LightProjection");
		glUniformMatrix4fv(locLightProjection, 1, GL_FALSE, glm::value_ptr(lightCamera->GetProjectionMatrix()));

		for (auto object : sceneObjects)
		{
			RenderSimpleMesh(object->mesh, shader, object->modelMatrix);
		}

		RenderSimpleMesh(lightObject->mesh, shader, lightObject->modelMatrix);
	}

	if (renderDebugView)
	{
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		{
			auto shader = shaders["TextureDebug"];
			shader->Use();

			{
				int locTexture = shader->GetUniformLocation("texture_1");
				glUniform1i(locTexture, 0);
				shadowFBO->BindDepthTexture(GL_TEXTURE0);
			}

			glm::mat4 modelMatrix(1);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5));
			RenderMesh(meshes["quad"], shader, modelMatrix);
		}
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
}

void ShadowMapping::FrameEnd()
{
	DrawCoordinatSystem();
}

void ShadowMapping::RenderScene(Shader * shader)
{

}

void ShadowMapping::RenderSceneFromLigth(Shader * shader)
{

}

void ShadowMapping::RenderSimpleMesh(Mesh *mesh, Shader *shader, const glm::mat4 & modelMatrix, const glm::vec3 &color)
{
	if (!mesh || !shader || !shader->GetProgramID())
		return;

	// render an object using the specified shader and the specified position
	shader->Use();

	// Set shader uniforms for light & material properties
	// Set light position uniform
	glm::vec3 lightPosition = lightCamera->transform->GetWorldPosition();
	int light_position = shader->GetUniformLocation("light_position");
	glUniform3f(light_position, lightPosition.x, lightPosition.y, lightPosition.z);

	int light_direction = shader->GetUniformLocation("light_direction");
	glUniform3f(light_direction, lightDirection.x, lightDirection.y, lightDirection.z);

	// Set eye position (camera position) uniform
	glm::vec3 eyePosition = GetSceneCamera()->transform->GetWorldPosition();
	int eye_position = shader->GetUniformLocation("eye_position");
	glUniform3f(eye_position, eyePosition.x, eyePosition.y, eyePosition.z);

	// Set material property uniforms (shininess, kd, ks, object color) 
	int material_shininess = shader->GetUniformLocation("material_shininess");
	glUniform1i(material_shininess, materialShininess);

	int material_kd = shader->GetUniformLocation("material_kd");
	glUniform1f(material_kd, materialKd);

	int material_ks = shader->GetUniformLocation("material_ks");
	glUniform1f(material_ks, materialKs);

	int object_color = shader->GetUniformLocation("object_color");
	glUniform3f(object_color, color.r, color.g, color.b);

	int light_mode = shader->GetUniformLocation("light_mode");
	glUniform1i(light_mode, lightMode);

	// Bind model matrix
	GLint loc_model_matrix = shader->GetUniformLocation("Model");
	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	// Bind view matrix
	glm::mat4 viewMatrix = GetSceneCamera()->GetViewMatrix();
	int loc_view_matrix = shader->GetUniformLocation("View");
	glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	// Bind projection matrix
	glm::mat4 projectionMatrix = GetSceneCamera()->GetProjectionMatrix();
	int loc_projection_matrix = shader->GetUniformLocation("Projection");
	glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	// Draw the object
	mesh->Render();
}

void ShadowMapping::LoadShader(std::string name)
{
	static std::string shaderPath = "Source/Laboratoare/ShadowMapping/Shaders/";

	// Create a shader program for particle system
	{
		Shader *shader = new Shader(name.c_str());
		shader->AddShader((shaderPath + name + ".VS.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((shaderPath + name + ".FS.glsl").c_str(), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

// Documentation for the input functions can be found in: "/Source/Core/Window/InputController.h" or
// https://github.com/UPB-Graphics/PPBG/blob/master/Source/Core/Window/InputController.h

void ShadowMapping::OnInputUpdate(float deltaTime, int mods)
{
	float speed = 2;

	if (!window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT))
	{
		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 right = GetSceneCamera()->transform->GetLocalOXVector();
		glm::vec3 forward = GetSceneCamera()->transform->GetLocalOZVector();
		forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

		// Control light position using on W, A, S, D, E, Q
		//if (window->KeyHold(GLFW_KEY_W)) lightPosition -= forward * deltaTime * speed;
		//if (window->KeyHold(GLFW_KEY_A)) lightPosition -= right * deltaTime * speed;
		//if (window->KeyHold(GLFW_KEY_S)) lightPosition += forward * deltaTime * speed;
		//if (window->KeyHold(GLFW_KEY_D)) lightPosition += right * deltaTime * speed;
		//if (window->KeyHold(GLFW_KEY_E)) lightPosition += up * deltaTime * speed;
		//if (window->KeyHold(GLFW_KEY_Q)) lightPosition -= up * deltaTime * speed;
	}
}

void ShadowMapping::OnKeyPress(int key, int mods)
{
	// add key press event
	if (key == GLFW_KEY_C)
	{
		bool controlCamera = GetCameraInput()->IsActive();
		controlCamera = !controlCamera;
		GetCameraInput()->SetActive(controlCamera);
		lightCameraInput->SetActive(!controlCamera);
	}

	if (key == GLFW_KEY_GRAVE_ACCENT)
	{
		renderDebugView = !renderDebugView;
	}
}

void ShadowMapping::OnKeyRelease(int key, int mods)
{
	// add key release event
}

void ShadowMapping::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// add mouse move event
}

void ShadowMapping::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button press event
}

void ShadowMapping::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button release event
}

void ShadowMapping::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
}

void ShadowMapping::OnWindowResize(int width, int height)
{
}

GameObject::GameObject(Mesh * mesh, glm::mat4 model)
{
	this->mesh = mesh;
	this->modelMatrix = model;
}
