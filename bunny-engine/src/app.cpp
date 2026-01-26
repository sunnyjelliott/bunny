#include "app.h"

#include <vk_mem_alloc.h>

#include "input.h"

void Application::run() {
	initWindow();
	initVulkan();
	initScene();
	mainLoop();
	cleanup();
}

void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(
	    GLFW_RESIZABLE,
	    GLFW_FALSE);  // TODO: Re-enable when window resizing is handled.

	m_window =
	    glfwCreateWindow(WIDTH, HEIGHT, "Bunny Engine", nullptr, nullptr);

	m_inputBackend = new GLFWInputBackend(m_window);

	Input::initialize(m_inputBackend);
	Input::setMouseCaptured(true);
}

void Application::initVulkan() {
	m_context.init(m_window);
	m_swapChain.init(m_context, m_window, WIDTH, HEIGHT);
	m_renderSystem.initialize(m_context, m_swapChain);
}

void Application::initScene() {
	// Camera
	m_activeCamera = m_world.createEntity();
	m_world.addComponent(m_activeCamera,
	                     Transform{.position = glm::vec3(0.0f, 2.0f, 8.0f)});
	m_world.addComponent(
	    m_activeCamera,
	    Camera{.fov = 45.0f, .nearPlane = 0.1f, .farPlane = 100.0f});
	m_cameraSystem.setActiveCamera(m_activeCamera);

	// Front cube (should occlude back cube)
	Entity frontCube = m_world.createEntity();
	m_world.addComponent(frontCube,
	                     Transform{.position = glm::vec3(0.0f, 0.0f, 0.0f),
	                               .scale = glm::vec3(1.0f)});
	m_world.addComponent(frontCube, MeshRenderer{.meshID = 0, .visible = true});

	// Back cube (should be partially hidden)
	Entity backCube = m_world.createEntity();
	m_world.addComponent(
	    backCube, Transform{.position = glm::vec3(0.5f, 0.5f,
	                                              -2.0f),  // Behind and offset
	                        .scale = glm::vec3(1.0f)});
	m_world.addComponent(backCube, MeshRenderer{.meshID = 0, .visible = true});

	std::cout << "Created " << m_world.getEntityCount() << " entities\n";
}

void Application::mainLoop() {
	Entity parent = 0;  // First entity created
	float rotation = 0.0f;

	while (!glfwWindowShouldClose(m_window)) {
		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - m_lastFrameTime;
		m_lastFrameTime = currentTime;

		glfwPollEvents();

		if (Input::isActionPressed(InputAction::ToggleMouseCapture)) {
			Input::setMouseCaptured(!Input::isMouseCaptured());
		}

		m_cameraSystem.updateFreeFly(
		    m_world, deltaTime, Input::isActionHeld(InputAction::MoveForward),
		    Input::isActionHeld(InputAction::MoveBackward),
		    Input::isActionHeld(InputAction::MoveLeft),
		    Input::isActionHeld(InputAction::MoveRight),
		    Input::getActionValue(InputAction::LookHorizontal),
		    Input::getActionValue(InputAction::LookVertical));

		Input::update();

		// Update transforms
		m_transformSystem.update(m_world);

		// Render
		m_renderSystem.drawFrame(m_swapChain, m_world, m_cameraSystem);
	}

	vkDeviceWaitIdle(m_context.getDevice());
}

void Application::cleanup() {
	m_renderSystem.cleanup();
	m_swapChain.cleanup();
	m_context.cleanup();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}