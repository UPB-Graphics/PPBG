#pragma once
#include <Component/CameraInput.h>
#include <Component/SimpleScene.h>
#include <Component/Transform/Transform.h>
#include <Core/GPU/Mesh.h>

class GameObject
{
	public:
		GameObject(Mesh *mesh, glm::mat4 model);

	public:
		Mesh *mesh;
		glm::mat4 modelMatrix;
};

class ShadowMapping : public SimpleScene
{
	public:
		ShadowMapping();
		~ShadowMapping();

		void Init() override;

	private:
		void FrameStart() override;
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;

		void RenderScene(Shader *shader);
		void RenderSceneFromLigth(Shader *shader);
		void RenderSimpleMesh(Mesh *mesh, Shader *shader, const glm::mat4 &modelMatrix, const glm::vec3 &color = glm::vec3(1));

		void LoadShader(std::string name);

		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;

	private:
		glm::vec3 lightDirection;
		unsigned int materialShininess;
		float materialKd;
		float materialKs;
		int lightMode;

		Camera* lightCamera;
		CameraInput * lightCameraInput;
		GameObject* lightObject;
		std::vector<GameObject*> sceneObjects;

		FrameBuffer *shadowFBO;
		bool renderDebugView;
};
