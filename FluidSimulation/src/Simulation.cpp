#include "Simulation.h"
#include "Core/Input.h"
#include "Utilities/Random.h"
#include "Utilities/GUIControls.h"
#include "Utilities/FileDialog.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui/imgui.h>

#include <yaml-cpp/yaml.h>
#include "Utilities/yaml-glm.h"
#include <fstream>

static void DecomposeTransform(const glm::mat4& Transform, glm::vec3& Translation, glm::vec3& Rotation, glm::vec3& Scale)
{
	// From glm::decompose in matrix_decompose.inl

	using namespace glm;
	using T = float;

	mat4 LocalMatrix(Transform);

	// Normalize the matrix.
	if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
		return;

	// First, isolate perspective.  This is the messiest.
	if (
		epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
		epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
		epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
	{
		// Clear the perspective partition
		LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
		LocalMatrix[3][3] = static_cast<T>(1);
	}

	// Next take care of translation (easy).
	Translation = vec3(LocalMatrix[3]);
	LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

	vec3 Row[3], Pdum3;

	// Now get scale and shear.
	for (length_t i = 0; i < 3; ++i)
		for (length_t j = 0; j < 3; ++j)
			Row[i][j] = LocalMatrix[i][j];

	// Compute X scale factor and normalize first row.
	Scale.x = length(Row[0]);
	Row[0] = detail::scale(Row[0], static_cast<T>(1));
	Scale.y = length(Row[1]);
	Row[1] = detail::scale(Row[1], static_cast<T>(1));
	Scale.z = length(Row[2]);
	Row[2] = detail::scale(Row[2], static_cast<T>(1));

	// At this point, the matrix (in rows[]) is orthonormal.
	// Check for a coordinate system flip.  If the determinant
	// is -1, then negate the matrix and the scaling factors.

	vec3 rotation;
	rotation.y = asin(-Row[0][2]);
	if (cos(rotation.y) != 0) {
		rotation.x = atan2(Row[1][2], Row[2][2]);
		rotation.z = atan2(Row[0][1], Row[0][0]);
	}
	else {
		rotation.x = atan2(-Row[2][0], Row[1][1]);
		rotation.z = 0;
	}

	Rotation = rotation;
	//vec3 deltaRoation = rotation - m_Rotation;
	//m_Rotation += deltaRoation;
}

Simulation::Simulation(const ApplicationSpecification& spec)
	:Application(spec)
{
	m_Camera = CreateScope<Camera>(Camera::ProjectionType::Perspective);
	//m_Camera->SetViewportSize(32 + 2, (4.5f * 6.f) + 2);
	m_Camera->SetPosition(glm::vec3(0.f, 0.f, -40.f));

	//Objects
	
	//white Cube
	AddObject(glm::vec3(0.f, 4.f, 0.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 13.f, 1.f), glm::vec3(1.f), true);
	//floor
	AddObject(glm::vec3( 8.f, 13.f,  8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(8.f, 0.1f, 8.f), glm::vec3(1.f, 1.f, 0.f));
	AddObject(glm::vec3( 8.f, 13.f, -8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(8.f, 0.1f, 8.f), glm::vec3(1.f, 0.f, 0.f));
	AddObject(glm::vec3(-8.f, 13.f,  8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(8.f, 0.1f, 8.f), glm::vec3(0.f, 1.f, 0.f));
	AddObject(glm::vec3(-8.f, 13.f, -8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(8.f, 0.1f, 8.f), glm::vec3(0.f, 0.f, 1.f));

	m_Renderer->UpdateSceneObjects(m_Objects);

	//Spawners
	m_Spawners.clear();
	{
		SpawnCube spawnCube;
		spawnCube.center = glm::vec3(8.f, 8.f, 0.f);
		spawnCube.size = glm::vec3(9.f, 9.f, 9.f);
		m_Spawners.push_back(spawnCube);
	}

	{
		SpawnCube spawnCube;
		spawnCube.center = glm::vec3(-8.f, 8.f, 0.f);
		spawnCube.size = glm::vec3(9.f, 9.f, 9.f);
		m_Spawners.push_back(spawnCube);
	}

	SpawnParticles();


	//SaveConfiguration("test.yaml");
	//LoadConfiguration("test.yaml");
}

Simulation::~Simulation()
{
}

void Simulation::OnEvent(Event& e)
{
	EventDispatcher dispacher(e);
	dispacher.Dispatch<WindowResizeEvent>(BIND_EVENT(Simulation::OnResize));
	dispacher.Dispatch<KeyPressedEvent>(BIND_EVENT(Simulation::OnKeyPressed));
}

void Simulation::OnUpdate(float deltaTime)
{
	if (bMinimized)
		return;

	UpdateCamera(deltaTime);

	m_Renderer->UpdateCamera(*m_Camera);
	if (!bPoused)
	{
		m_Renderer->SimulationStep(bSlowmo? deltaTime*0.2f : deltaTime);
	}
	else if (Input::IsKeyDown(GLFW_KEY_RIGHT))
	{
		m_Renderer->SimulationStep(deltaTime);
	}
}

void Simulation::OnRender()
{
	if (bMinimized)
		return;

	m_Renderer->RenderFrame(0.1f);
}

void Simulation::OnGUIRender()
{
	ImGui::Begin("Controls");
	if (ImGui::CollapsingHeader("General"))
	{
		ImGui::Checkbox("Slow Motion", &bSlowmo);

		if (ImGui::Button("Open"))
		{
			std::string filepath = FileDialog::OpenFile("Scene (*.yaml)\0*.yaml\0");
			if (!filepath.empty())
				LoadConfiguration(filepath);
		}
		ImGui::SameLine();

		if (ImGui::Button("Save"))
		{
			std::string path = FileDialog::SaveFile("Scene (*.yaml)\0*.yaml\0");
			if (!path.empty())
			{
				SaveConfiguration(path);
			}
		}

		if (ImGui::Button(bPoused ? "Play" : "Stop"))
		{
			bPoused = !bPoused;
		}

		ImGui::SameLine();
		if (ImGui::Button("Restart"))
		{
			m_Renderer->RestartSimulation();
		}


		ImGui::SeparatorText("Camera");

		glm::vec3 CameraPos = m_Camera->GetPosition();
		if (ImGui::DragFloat3("Camera position", &CameraPos.x))
		{
			LOG(DEBUG, "Camera Update");
			m_Camera->SetPosition(CameraPos);
		}

		glm::vec3 CameraForward = m_Camera->GetForward();
		if (ImGui::DragFloat3("Camera forward", &CameraForward.x))
		{
			m_Camera->SetForward(CameraForward);
		}

		float FOV = m_Camera->GetFOV();
		if (ImGui::DragFloat("FOV", &FOV))
		{
			FOV = std::max(1.f, FOV);
			m_Camera->SetFOV(FOV);
		}
	}


	//static float stepSize = 0.02f;
	//static bool bDynamicCalculations = true;
	//static float sampling = 0.1f;

	//ImGui::DragFloat("StepSize", &stepSize, 0.001f, 0.001f, 0.1f);
	//ImGui::Checkbox("Dynamic Calculations", &bDynamicCalculations);
	//ImGui::DragFloat("Sampling", &sampling, 0.001f, 0.001f, 1.0f);

	//m_Renderer->UpdateRenderSetings(stepSize, bDynamicCalculations, sampling);
	GUI_RendererSpec();
	GUI_SimulationSpec();
	GUI_ObjectList();
	GUI_SpawnerList();

	ImGui::End();

	//ImGui::ShowDemoWindow();

}

bool Simulation::OnResize(WindowResizeEvent& e)
{
	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		bMinimized = true;
		return true;
	}

	bMinimized = false;

	LOG(INFO, "Window size (%u x %u)", e.GetWidth(), e.GetHeight())

	return true;
}

bool Simulation::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == GLFW_KEY_SPACE && !e.IsReapeted())
	{
		bPoused = !bPoused;
		LOG(DEBUG, "Simulation state: %s", bPoused ? "Poused" : "played");
	}

	if (e.GetKeyCode() == GLFW_KEY_R)
	{
		SpawnParticles();
		bPoused = true;
	}

	if (e.GetKeyCode() == GLFW_KEY_Z)
	{
		bSlowmo = !bSlowmo;
		LOG(DEBUG, "Slowmo state: %s", bSlowmo ? "Active" : "Disable");
	}

	return true;
}

void Simulation::UpdateCamera(float deltaTime)
{
	glm::vec2 MousePos = Input::GetMousePosition();
	glm::vec2 delta = (MousePos - m_LastMousePosition) * 0.02f;
	m_LastMousePosition = MousePos;

	if (!Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
	{
		Input::SetCursorMode(GLFW_CURSOR_NORMAL);
		return;
	}
	Input::SetCursorMode(GLFW_CURSOR_DISABLED);

	if (delta.x != 0.f || delta.y != 0.f)
	{
		float PitchDelta = -delta.y * 0.3;
		float YawDelta = delta.x * 0.3;

		glm::quat q = glm::normalize(glm::cross(glm::angleAxis(PitchDelta, m_Camera->GetRight()), glm::angleAxis(YawDelta, glm::vec3(0.f, 1.f, 0.f))));
		m_Camera->SetForward(glm::rotate(q, m_Camera->GetForward()));
	}

	float Speed = 10.f;
	bool bMoved = false;
	glm::vec3 CameraPos = m_Camera->GetPosition();

	//Frorward
	if (Input::IsKeyDown(GLFW_KEY_W))
	{
		CameraPos -= m_Camera->GetForward() * Speed * deltaTime;
		bMoved = true;
	}
	//backward
	if (Input::IsKeyDown(GLFW_KEY_S))
	{
		CameraPos += m_Camera->GetForward() * Speed * deltaTime;
		bMoved = true;
	}
	//Right
	if (Input::IsKeyDown(GLFW_KEY_D))
	{
		CameraPos += m_Camera->GetRight() * Speed * deltaTime;
		bMoved = true;
	}
	//Left
	if (Input::IsKeyDown(GLFW_KEY_A))
	{
		CameraPos -= m_Camera->GetRight() * Speed * deltaTime;
		bMoved = true;
	}
	//Up
	if (Input::IsKeyDown(GLFW_KEY_Q))
	{
		CameraPos -= glm::vec3(0.f, 1.f, 0.f) * Speed * deltaTime;
		bMoved = true;
	}
	//Down
	if (Input::IsKeyDown(GLFW_KEY_E))
	{
		CameraPos += glm::vec3(0.f, 1.f, 0.f) * Speed * deltaTime;
		bMoved = true;
	}

	if (bMoved)
	{
		m_Camera->SetPosition(CameraPos);
		//LOG(DEBUG, "camera position: %.2f, %.2f, %.2f", CameraPos.x, CameraPos.y, CameraPos.z);
	}
}

void Simulation::ToggleSlowmo()
{
	bSlowmo = !bSlowmo;
}

void Simulation::AddObject(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec3 color, bool bCollision)
{
	glm::mat4 Translate = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 Scale = glm::scale(glm::mat4(1.0f), scale);

	glm::quat quatRotation = glm::quat(glm::radians(rotation));
	glm::mat4 Rotation = glm::toMat4(quatRotation);

	Object object;
	object.transform = Translate * Rotation * Scale;
	object.Color = color;
	object.useCollision = bCollision;
	m_Objects.push_back(object);
}

void Simulation::SpawnParticles()
{
	std::vector<Particle> particles;
	particles.reserve(m_ActiveFluid.particlesCount);

	/*
	for (size_t i = 0; i < 256*128; i++)
	{
		//Sphere
		const float radius = 5.f;
		const glm::vec3 origin(0.f, 0.f, 0.f);
		float u = Random::randf();
		float x = Random::randf(-1.f, 1.f);
		float y = Random::randf(-1.f, 1.f);
		float z = Random::randf(-1.f, 1.f);

		glm::vec3 direction(x, y, z);
		direction = glm::normalize(direction);
		direction *= std::cbrt(u) * radius;

		Particle p;
		p.position = direction + origin;
		p.predictedPosition = direction + origin;
		p.density = 0.0f;
		p.velocity = glm::vec3(0.f, 0.f, 0.f);
		particles.push_back(p);
	}
	*/
	std::vector<float> weights;
	float TotalWeight = 0.f;
	for (const SpawnCube& spawn : m_Spawners)
	{
		float weight = spawn.size.x * spawn.size.y * spawn.size.z;
		TotalWeight += weight;
		weights.push_back(weight);
	}

	for (size_t i = 0; i < m_ActiveFluid.particlesCount; i++)
	{
		//uint32_t spawnCubeID = Random::rand(0, m_Spawners.size());
		float weight = Random::randf(0, TotalWeight);
		uint32_t spawnCubeID = 0;
		for (; spawnCubeID < m_Spawners.size(); spawnCubeID++)
		{
			weight -= weights[spawnCubeID];
			if (weight <= 0.f)
			{
				break;
			}
		}

		SpawnCube& spawnCube = m_Spawners[spawnCubeID];

		glm::vec3 extend = spawnCube.size / 2.f;
		glm::vec3 position = Random::rand3f(-extend, extend);

		Particle p;
		p.position = position + spawnCube.center;
		p.predictedPosition = position + spawnCube.center;
		p.density = 0.0f;
		p.velocity = glm::vec3(0.f, 0.f, 0.f);
		particles.push_back(p);
	}

	m_Renderer->InitializeParticles(particles);
}

void Simulation::GUI_ObjectList()
{
	if (!ImGui::CollapsingHeader("Objects"))
		return;

	bool bObjectChage = false;
	int32_t deletedID = -1;

	for (uint32_t i = 0; i < m_Objects.size(); i++)
	{
		ImGui::PushID(i | 1<<5);
		if (ImGui::TreeNode("", "Cube %d", i))
		{
			Object& cube = m_Objects[i];

			glm::vec3 scale, rotation, translation;
			DecomposeTransform(cube.transform, translation, rotation, scale);

			bObjectChage |= GUIControls::DragVec3("Position", translation);
			rotation = glm::degrees(rotation);
			bObjectChage |= GUIControls::DragVec3("Rotation", rotation);
			bObjectChage |= GUIControls::DragVec3("Scale", scale, 1.f);

			scale = glm::max(scale, glm::vec3(0.001f));
			glm::mat4 Translate = glm::translate(glm::mat4(1.0f), translation);
			glm::mat4 Scale = glm::scale(glm::mat4(1.0f), scale);

			glm::quat quatRotation = glm::quat(glm::radians(rotation));
			glm::mat4 Rotation = glm::toMat4(quatRotation);
			cube.transform = Translate * Rotation * Scale;
		
			bObjectChage |= ImGui::ColorEdit3("Color", &cube.Color.r);

			bool bUseForCollision = cube.useCollision;
			bObjectChage |= ImGui::Checkbox("Collision", &bUseForCollision);
			cube.useCollision = bUseForCollision;

			if (ImGui::Button("Delete cube"))
			{
				bObjectChage = true;
				deletedID = i;
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	if (ImGui::Button("AddObject"))
	{
		AddObject(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f), glm::vec3(1.f));
		bObjectChage = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("AddRandom"))
	{
		glm::vec3 Position = Random::rand3f(glm::vec3(-10.f), glm::vec3(10.f));
		glm::vec3 Rotation = Random::rand3f(glm::vec3(0.f), glm::vec3(180.f));
		glm::vec3 Scale = Random::rand3f(glm::vec3(0.1f), glm::vec3(5.f));
		glm::vec3 Color = Random::rand3f(glm::vec3(0.0f), glm::vec3(1.f));
		AddObject(Position, Rotation, Scale, Color, true);
		bObjectChage = true;
	}


	if (deletedID != -1)
	{
		m_Objects.erase(m_Objects.begin() + deletedID);
	}

	if (bObjectChage)
	{
		m_Renderer->UpdateSceneObjects(m_Objects);
	}
}

void Simulation::GUI_SpawnerList()
{
	//ImGui::Begin("Spawners");
	if (!ImGui::CollapsingHeader("Spawners"))
	{
		return;
	}

	int32_t deletedID = -1;

	for (uint32_t i = 0; i < m_Spawners.size(); i++)
	{
		ImGui::PushID(i);
		if (ImGui::TreeNode("", "Spawner %d", i))
		{
			SpawnCube& spawnCube = m_Spawners[i];


			GUIControls::DragVec3("Position", spawnCube.center);
			GUIControls::DragVec3("size", spawnCube.size, 1.f);

			if (ImGui::Button("Delete spawner"))
			{
				deletedID = i;
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	if (ImGui::Button("Add spawner"))
	{
		SpawnCube spawnCube;
		spawnCube.center = glm::vec3(0.f, 0.f, 0.f);
		spawnCube.size = glm::vec3(9.f, 9.f, 9.f);
		m_Spawners.push_back(spawnCube);
	}

	//ImGui::End();

	if (deletedID != -1)
	{
		m_Spawners.erase(m_Spawners.begin() + deletedID);
	}
}

void Simulation::GUI_SimulationSpec()
{
	if (!ImGui::CollapsingHeader("Simulation"))
	{
		return;
	}
	//ImGui::Begin("Simulation controls");

	bool bUpdateSetings = false;

	bUpdateSetings |= GUIControls::DragVec3("Extend", m_ActiveFluid.SimulationExtend, 1.f);
	bUpdateSetings |= ImGui::DragFloat("CollisionDamping", &m_ActiveFluid.collisionDamping, 0.001f, 0.f, 1.f);
	bUpdateSetings |= ImGui::DragFloat("KernelRadius", &m_ActiveFluid.kernelRadius, 0.001f);
	bUpdateSetings |= ImGui::DragFloat("TargetDensity", &m_ActiveFluid.targetDensity);
	bUpdateSetings |= ImGui::DragFloat("PressureMultiplayer", &m_ActiveFluid.pressureMultiplier);
	bUpdateSetings |= ImGui::DragFloat("ViscosityStrength", &m_ActiveFluid.viscosityStrength, 0.001f);
	bUpdateSetings |= ImGui::DragScalar("ParticlesCount", ImGuiDataType_U32, &m_ActiveFluid.particlesCount, 128);

	if (bUpdateSetings)
	{
		m_Renderer->UpdateSimulatonSetings(m_ActiveFluid);
	}

	//ImGui::End();
}

void Simulation::GUI_RendererSpec()
{
	//ImGui::Begin("Rendering controls");
	if (!ImGui::CollapsingHeader("Rendering"))
	{
		return;
	}

	bool bUpdateSetings = false;

	bUpdateSetings |= ImGui::DragFloat("StepSize", &m_ActiveRenderer.StepSize, 0.001f, 0.001f, FLT_MAX);
	bUpdateSetings |= ImGui::DragScalar("Bounces", ImGuiDataType_U32, &m_ActiveRenderer.Bounces, 0.2);
	bUpdateSetings |= ImGui::DragFloat3("ExtincionCoeff", &m_ActiveRenderer.ExtinctionCoeff.r, 0.1f, 0.f, FLT_MAX);
	bUpdateSetings |= ImGui::DragFloat3("Sun", &m_ActiveRenderer.SunDirection.r, 0.1f);
	bUpdateSetings |= ImGui::DragFloat("IOR", &m_ActiveRenderer.IOR, 0.001f, 0.001f, FLT_MAX);

	if (bUpdateSetings)
	{
		m_Renderer->UpdateRenderSetings(m_ActiveRenderer);
	}

	//ImGui::End();
}

void Simulation::SaveConfiguration(const std::string& path)
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	//rendering setings
	out << YAML::Key << "Rendering" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Camera" << YAML::Value << m_Camera->GetPosition();
	out << YAML::Key << "StepSize" << YAML::Value << m_ActiveRenderer.StepSize;
	out << YAML::Key << "Bounces" << YAML::Value << m_ActiveRenderer.Bounces;
	out << YAML::Key << "ExtinctionCoeff" << YAML::Value << m_ActiveRenderer.ExtinctionCoeff;
	out << YAML::Key << "Sun" << YAML::Value << m_ActiveRenderer.SunDirection;
	out << YAML::Key << "IOR" << YAML::Value << m_ActiveRenderer.IOR;
	out << YAML::EndMap;
	//Simulation Specification
	out << YAML::Key << "SimulationSpec" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Extend" << YAML::Value << m_ActiveFluid.SimulationExtend;
	out << YAML::Key << "CollisionDamping" << YAML::Value << m_ActiveFluid.collisionDamping;
	out << YAML::Key << "KernelRadius" << YAML::Value << m_ActiveFluid.kernelRadius;
	out << YAML::Key << "TargetDensity" << YAML::Value << m_ActiveFluid.targetDensity;
	out << YAML::Key << "PressureMultiplayer" << YAML::Value << m_ActiveFluid.pressureMultiplier;
	out << YAML::Key << "ViscosityStrength" << YAML::Value << m_ActiveFluid.viscosityStrength;
	out << YAML::Key << "ParticlesCout" << YAML::Value << m_ActiveFluid.particlesCount;
	out << YAML::EndMap;
	//Obstacles
	out << YAML::Key << "Obstacles" << YAML::Value << YAML::BeginSeq;
	for(const Object& object : m_Objects)
	{
		glm::vec3 scale, rotation, translation;
		DecomposeTransform(object.transform, translation, rotation, scale);

		out << YAML::BeginMap;
		out << YAML::Key << "Position" << YAML::Value << translation;
		out << YAML::Key << "Rotation" << YAML::Value << rotation;
		out << YAML::Key << "Scale" << YAML::Value << scale;
		out << YAML::Key << "Color" << YAML::Value << object.Color;
		out << YAML::Key << "UseCollision" << YAML::Value << (bool)object.useCollision;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	//Spawners
	out << YAML::Key << "Spawners" << YAML::Value << YAML::BeginSeq;
	for (const SpawnCube& spawnCube : m_Spawners)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Center" << YAML::Value << spawnCube.center;
		out << YAML::Key << "Size" << YAML::Value << spawnCube.size;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::ofstream fout(path);
	fout << out.c_str();
}

void Simulation::LoadConfiguration(const std::string& path)
{
	YAML::Node data = YAML::LoadFile(path);
	//rendering setings
	auto renderer = data["Rendering"];
	m_ActiveRenderer.cameraPos = renderer["Camera"].as<glm::vec3>();
	m_ActiveRenderer.StepSize  = renderer["StepSize"].as<float>();
	m_ActiveRenderer.Bounces = renderer["Bounces"].as<uint32_t>();
	m_ActiveRenderer.ExtinctionCoeff = renderer["ExtinctionCoeff"].as<glm::vec3>();
	m_ActiveRenderer.SunDirection = renderer["Sun"].as<glm::vec3>();
	m_ActiveRenderer.IOR = renderer["IOR"].as<float>();

	//Simulation Specification
	auto fluid = data["SimulationSpec"];
	m_ActiveFluid.SimulationExtend	 = fluid["Extend"].as<glm::vec3>();
	m_ActiveFluid.collisionDamping	 = fluid["CollisionDamping"].as<float>();
	m_ActiveFluid.kernelRadius		 = fluid["KernelRadius"].as<float>();
	m_ActiveFluid.targetDensity		 = fluid["TargetDensity"].as<float>();
	m_ActiveFluid.pressureMultiplier = fluid["PressureMultiplayer"].as<float>();
	m_ActiveFluid.viscosityStrength  = fluid["ViscosityStrength"].as<float>();
	m_ActiveFluid.particlesCount	 = fluid["ParticlesCout"].as<uint32_t>();

	//Obstacles
	m_Objects.clear();
	auto obstacles = data["Obstacles"];
	for (auto const& obstacle : obstacles)
	{
		glm::vec3 position = obstacle["Position"].as<glm::vec3>();
		glm::vec3 rotation = obstacle["Rotation"].as<glm::vec3>();
		glm::vec3 scale = obstacle["Scale"].as<glm::vec3>();
		glm::vec3 color = obstacle["Color"].as<glm::vec3>();
		bool bUseCollision = obstacle["UseCollision"].as<bool>();

		AddObject(position, rotation, scale, color, bUseCollision);
	}

	//Spawners
	m_Spawners.clear();
	auto spawners = data["Spawners"];
	for (auto const& spawner : spawners)
	{
		SpawnCube spawnCube;
		spawnCube.center = spawner["Center"].as<glm::vec3>();
		spawnCube.size = spawner["Size"].as<glm::vec3>();
		m_Spawners.push_back(spawnCube);
	}

	m_Renderer->UpdateSceneObjects(m_Objects);
	m_Renderer->UpdateSimulatonSetings(m_ActiveFluid);
	SpawnParticles();
}
