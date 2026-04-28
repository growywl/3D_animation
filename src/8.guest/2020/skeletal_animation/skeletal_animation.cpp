#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>

#include <iostream>
#include <string>
#include <unordered_set>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// third-person camera (orbit around player)
Camera camera(glm::vec3(0.0f, 2.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

static constexpr float ENEMY_MAX_HP = 3.0f;

enum class PlayerAnimState
{
	Idle,
	Walk,
	Run,
	Attack
};

struct Enemy
{
	glm::vec3 position{ 0.0f, -1.0f, -3.0f };
	float hp = ENEMY_MAX_HP;
	bool alive = true;
	float deathTimer = 0.0f;
};

static glm::vec3 gPlayerPos(0.0f, -1.0f, 0.0f);
static float gPlayerYawDeg = 180.0f;
static PlayerAnimState gPlayerState = PlayerAnimState::Idle;
static float gAttackTimer = 0.0f;
static bool gAttackHitApplied = false;

static float gCamYawDeg = -90.0f;
static float gCamPitchDeg = -15.0f;
static float gCamDistance = 6.0f;

static bool gPrevAttackPressed = false;
static bool gPrevRespawnPressed = false;
static bool gWin = false;
static bool gPrevWin = false;

static float AnimationLengthSeconds(const Animation& anim)
{
	const float ticksPerSecond = anim.GetTicksPerSecond();
	if (ticksPerSecond <= 0.0f) return 0.0f;
	return anim.GetDuration() / ticksPerSecond;
}

static void UpdateThirdPersonCamera()
{
	const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
	const glm::vec3 target = gPlayerPos + glm::vec3(0.0f, 1.2f, 0.0f);

	const float yawRad = glm::radians(gCamYawDeg);
	const float pitchRad = glm::radians(gCamPitchDeg);

	glm::vec3 offset;
	offset.x = gCamDistance * cosf(pitchRad) * cosf(yawRad);
	offset.y = gCamDistance * sinf(pitchRad);
	offset.z = gCamDistance * cosf(pitchRad) * sinf(yawRad);

	camera.Position = target - offset;
	camera.Front = glm::normalize(target - camera.Position);
	camera.Right = glm::normalize(glm::cross(camera.Front, worldUp));
	camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
	camera.Zoom = 45.0f;
}

static void PrintSkinningStats(const char* label, const Model& model)
{
	std::size_t totalVertices = 0;
	std::size_t skinnedVertices = 0;
	for (const auto& mesh : model.meshes)
	{
		for (const auto& v : mesh.vertices)
		{
			totalVertices++;
			bool hasInfluence = false;
			for (int i = 0; i < 4; ++i)
			{
				if (v.m_BoneIDs[i] >= 0 && v.m_Weights[i] > 0.0f)
				{
					hasInfluence = true;
					break;
				}
			}
			if (hasInfluence) skinnedVertices++;
		}
	}
	std::cerr << label << " vertices: " << totalVertices << " | skinned: " << skinnedVertices << "\n";
}

static float ComputeModelMinY(const Model& model, const glm::mat4& preTransform = glm::mat4(1.0f))
{
	bool any = false;
	float minY = 0.0f;
	for (const auto& mesh : model.meshes)
	{
		for (const auto& v : mesh.vertices)
		{
			const glm::vec3 p = glm::vec3(preTransform * glm::vec4(v.Position, 1.0f));
			if (!any)
			{
				minY = p.y;
				any = true;
			}
			else if (p.y < minY)
			{
				minY = p.y;
			}
		}
	}
	return any ? minY : 0.0f;
}

static std::vector<int> CollectBoneIdsByNameSubstring(const Model& model, const std::vector<std::string>& needles)
{
	std::vector<int> out;
	std::unordered_set<int> seen;
	for (const auto& kv : model.GetBoneInfoMap())
	{
		const std::string key = NormalizeAnimBoneKey(kv.first);
		for (const auto& needle : needles)
		{
			if (!needle.empty() && key.find(needle) != std::string::npos)
			{
				const int id = kv.second.id;
				if (id >= 0 && id < 100 && !seen.count(id))
				{
					out.push_back(id);
					seen.insert(id);
				}
				break;
			}
		}
	}
	return out;
}

int main()
{
	// Make sure early logs show up even when stdout is captured/buffered.
	std::cout.setf(std::ios::unitbuf);

	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	const char* kBaseWindowTitle = "LearnOpenGL";
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, kBaseWindowTitle, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader ourShader("anim_model.vs", "anim_model.fs");
	Shader hpShader("hp_bar.vs", "hp_bar.fs");

	// simple ground plane (so the scene isn't empty/black)
	struct GroundVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
		int BoneIDs[4];
		float Weights[4];
	};

	const float groundY = -1.0f;
	const float groundSize = 30.0f;
	GroundVertex groundVerts[4] = {
		{ { -groundSize, groundY, -groundSize }, { 0, 1, 0 }, { 0, 0 }, { 0,0,0 }, { 0,0,0 }, { -1,-1,-1,-1 }, { 0,0,0,0 } },
		{ {  groundSize, groundY, -groundSize }, { 0, 1, 0 }, { 1, 0 }, { 0,0,0 }, { 0,0,0 }, { -1,-1,-1,-1 }, { 0,0,0,0 } },
		{ {  groundSize, groundY,  groundSize }, { 0, 1, 0 }, { 1, 1 }, { 0,0,0 }, { 0,0,0 }, { -1,-1,-1,-1 }, { 0,0,0,0 } },
		{ { -groundSize, groundY,  groundSize }, { 0, 1, 0 }, { 0, 1 }, { 0,0,0 }, { 0,0,0 }, { -1,-1,-1,-1 }, { 0,0,0,0 } },
	};
	unsigned int groundIdx[6] = { 0, 1, 2, 0, 2, 3 };
	unsigned int groundVAO = 0, groundVBO = 0, groundEBO = 0, groundTex = 0;
	glGenVertexArrays(1, &groundVAO);
	glGenBuffers(1, &groundVBO);
	glGenBuffers(1, &groundEBO);
	glBindVertexArray(groundVAO);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVerts), groundVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIdx), groundIdx, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, Position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, Normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, TexCoords));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, Tangent));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, Bitangent));
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 4, GL_INT, sizeof(GroundVertex), (void*)offsetof(GroundVertex, BoneIDs));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)offsetof(GroundVertex, Weights));
	glBindVertexArray(0);

	glGenTextures(1, &groundTex);
	glBindTexture(GL_TEXTURE_2D, groundTex);
	unsigned char groundPixel[4] = { 90, 140, 90, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, groundPixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// --- enemy HP bar (billboard quad) ---
	unsigned int hpVAO = 0, hpVBO = 0;
	glGenVertexArrays(1, &hpVAO);
	glGenBuffers(1, &hpVBO);
	glBindVertexArray(hpVAO);
	glBindBuffer(GL_ARRAY_BUFFER, hpVBO);
	// 6 vertices * vec3 (we'll update per-frame)
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 3, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);


	// load models
	// -----------
	std::cerr << "Controls:\n"
	          << "- WASD: move\n"
	          << "- Left Mouse / Space: attack\n"
	          << "- Shift: run\n"
	          << "- R: respawn enemy\n"
	          << "- Mouse: rotate camera\n"
	          << "- Scroll: zoom\n";

	Model playerModel(FileSystem::getPath("resources/FarmerPack/Character/Farmer/SKM_Farmer_male.fbx"));
	Animation playerIdle(FileSystem::getPath("resources/FarmerPack/Animations/Farmer/anim_Farmer_idle_combat.FBX"), &playerModel);
	Animation playerWalk(FileSystem::getPath("resources/FarmerPack/Animations/Farmer/anim_Farmer_walk.FBX"), &playerModel);
	Animation playerRun(FileSystem::getPath("resources/FarmerPack/Animations/Farmer/anim_Farmer_run.FBX"), &playerModel);
	Animation playerAttack(FileSystem::getPath("resources/FarmerPack/Animations/Farmer/anim_Farmer_attack_A.FBX"), &playerModel);
	Animator playerAnimator(&playerIdle);
	// Optional hack: hide "floating" hand bones if the asset's retargeting is imperfect.
	const std::vector<int> playerHandBoneIds = CollectBoneIdsByNameSubstring(
		playerModel,
		{ "hand", "wrist" }
	);

	Model enemyModel(FileSystem::getPath("resources/objects/vampire/dancing_vampire.dae"));
	Animation enemyDance(FileSystem::getPath("resources/objects/vampire/dancing_vampire.dae"), &enemyModel);
	Animator enemyAnimator(&enemyDance);
	Enemy enemy;

	// Farmer model appears Z-up in this pack; rotate it upright for our Y-up world.
	// Using -90 deg (instead of +90) fixes the "face-down" orientation for this asset.
	const glm::mat4 playerUprightRot =
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	const float playerMinY = ComputeModelMinY(playerModel, playerUprightRot);
	const float enemyMinY  = ComputeModelMinY(enemyModel);

	std::cerr << "PlayerModel bones: " << playerModel.GetBoneCount()
	          << " | idle duration: " << playerIdle.GetDuration()
	          << " ticks @ " << playerIdle.GetTicksPerSecond()
	          << " | channels: " << playerIdle.GetNumAnimatedBones() << "\n";
	std::cerr << "EnemyModel bones: " << enemyModel.GetBoneCount()
	          << " | dance duration: " << enemyDance.GetDuration()
	          << " ticks @ " << enemyDance.GetTicksPerSecond()
	          << " | channels: " << enemyDance.GetNumAnimatedBones() << "\n";
	PrintSkinningStats("Farmer", playerModel);
	PrintSkinningStats("Vampire", enemyModel);
	std::cerr << "Bind-pose minY: Farmer=" << playerMinY << " Vampire=" << enemyMinY << "\n";
	std::cerr << "Tip: if Farmer floats/sinks, adjust playerExtraLift in draw code.\n";
	if (!playerHandBoneIds.empty())
		std::cerr << "Note: hiding Farmer hand bones (ids=" << playerHandBoneIds.size() << ").\n";


	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);
		UpdateThirdPersonCamera();

		// --- update gameplay/animations ---
		const float playerSpeedWalk = 2.0f;
		const float playerSpeedRun = 4.0f;
		const bool runHeld = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

		glm::vec3 forward = glm::vec3(camera.Front.x, 0.0f, camera.Front.z);
		if (glm::length(forward) < 0.0001f)
			forward = glm::vec3(0.0f, 0.0f, -1.0f);
		else
			forward = glm::normalize(forward);
		const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

		glm::vec3 moveDir(0.0f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forward;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forward;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += right;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= right;

		const bool attackPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ||
			(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
		const bool attackJustPressed = attackPressed && !gPrevAttackPressed;
		gPrevAttackPressed = attackPressed;

		const bool respawnPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
		const bool respawnJustPressed = respawnPressed && !gPrevRespawnPressed;
		gPrevRespawnPressed = respawnPressed;

		if (respawnJustPressed)
		{
			enemy.alive = true;
			enemy.hp = ENEMY_MAX_HP;
			enemy.deathTimer = 0.0f;
			enemy.position = gPlayerPos + forward * 3.0f;
			gWin = false;
			glfwSetWindowTitle(window, kBaseWindowTitle);
		}

		if (gPlayerState == PlayerAnimState::Attack)
		{
			gAttackTimer += deltaTime;

			const float attackLen = AnimationLengthSeconds(playerAttack);
			const float t = (attackLen > 0.0f) ? (gAttackTimer / attackLen) : 1.0f;

			// damage window
			if (!gAttackHitApplied && enemy.alive && t >= 0.25f && t <= 0.45f)
			{
				const glm::vec2 p(gPlayerPos.x, gPlayerPos.z);
				const glm::vec2 e(enemy.position.x, enemy.position.z);
				const float dist = glm::length(p - e);
				if (dist <= 1.8f)
				{
					enemy.hp -= 1.0f;
					gAttackHitApplied = true;
					if (enemy.hp <= 0.0f)
					{
						enemy.alive = false;
						enemy.deathTimer = 0.0f;
						gWin = true;
					}
				}
			}

			if (attackLen > 0.0f && gAttackTimer >= attackLen)
			{
				gPlayerState = PlayerAnimState::Idle;
				playerAnimator.PlayAnimation(&playerIdle);
			}
		}
		else
		{
			if (attackJustPressed)
			{
				gPlayerState = PlayerAnimState::Attack;
				gAttackTimer = 0.0f;
				gAttackHitApplied = false;
				playerAnimator.PlayAnimation(&playerAttack);
			}
			else
			{
				const bool moving = glm::length(moveDir) > 0.001f;
				if (moving)
				{
					moveDir = glm::normalize(moveDir);
					const float speed = runHeld ? playerSpeedRun : playerSpeedWalk;
					gPlayerPos += moveDir * speed * deltaTime;
					gPlayerYawDeg = glm::degrees(atan2f(moveDir.x, moveDir.z));

					const PlayerAnimState wanted = runHeld ? PlayerAnimState::Run : PlayerAnimState::Walk;
					if (wanted != gPlayerState)
					{
						gPlayerState = wanted;
						playerAnimator.PlayAnimation(runHeld ? &playerRun : &playerWalk);
					}
				}
				else
				{
					if (gPlayerState != PlayerAnimState::Idle)
					{
						gPlayerState = PlayerAnimState::Idle;
						playerAnimator.PlayAnimation(&playerIdle);
					}
				}
			}
		}

		if (!enemy.alive)
			enemy.deathTimer += deltaTime;

		playerAnimator.UpdateAnimation(deltaTime);
		if (enemy.alive)
			enemyAnimator.UpdateAnimation(deltaTime);

		// render
		// ------
		glClearColor(0.25f, 0.35f, 0.45f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// don't forget to enable shader before setting uniforms
		ourShader.use();

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		// --- draw ground ---
		{
			glm::mat4 identity(1.0f);
			for (int i = 0; i < 100; ++i)
				ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", identity);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, groundTex);
			ourShader.setInt("texture_diffuse1", 0);

			glm::mat4 model = glm::mat4(1.0f);
			ourShader.setMat4("model", model);
			glBindVertexArray(groundVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		// --- draw player (Farmer) ---
		{
			auto transforms = playerAnimator.GetFinalBoneMatrices();
			// If hands are detached due to bone-space mismatch, drop them out of view.
			if (!playerHandBoneIds.empty())
			{
				const glm::mat4 hide = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1000.0f, 0.0f));
				for (int id : playerHandBoneIds)
				{
					if (id >= 0 && id < (int)transforms.size())
						transforms[id] = hide;
				}
			}
			for (int i = 0; i < transforms.size(); ++i)
				ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, gPlayerPos);
			model = glm::rotate(model, glm::radians(gPlayerYawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
			const float playerScale = 0.01f;
			// Tweak if the farmer appears slightly floating/sinking due to animation/root offsets.
			const float playerExtraLift = 0.0f; // in model units
			// Push model up so its lowest bind-pose vertex sits on y = gPlayerPos.y (our "ground").
			model = model * playerUprightRot;
			model = glm::scale(model, glm::vec3(playerScale));
			model = glm::translate(model, glm::vec3(0.0f, -playerMinY + playerExtraLift, 0.0f));
			ourShader.setMat4("model", model);
			playerModel.Draw(ourShader);
		}

		// --- draw enemy (Vampire) ---
		if (enemy.alive || enemy.deathTimer < 1.25f)
		{
			auto transforms = enemyAnimator.GetFinalBoneMatrices();
			for (int i = 0; i < transforms.size(); ++i)
				ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

			float sink = 0.0f;
			float shrink = 1.0f;
			if (!enemy.alive)
			{
				const float t = glm::clamp(enemy.deathTimer / 1.25f, 0.0f, 1.0f);
				sink = -1.0f * t;
				shrink = 1.0f - t;
			}

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, enemy.position + glm::vec3(0.0f, sink, 0.0f));
			// Flip 180 degrees so it faces the camera/player.
			model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			// Default size.
			const float enemyScale = 0.025f * shrink;
			model = glm::scale(model, glm::vec3(enemyScale));
			model = glm::translate(model, glm::vec3(0.0f, -enemyMinY, 0.0f));
			ourShader.setMat4("model", model);
			enemyModel.Draw(ourShader);
		}

		// --- draw enemy HP bar above head (world-space billboard) ---
		if (enemy.alive)
		{
			const float hp01 = glm::clamp(enemy.hp / ENEMY_MAX_HP, 0.0f, 1.0f);

			// Anchor above the enemy. Tune the height if needed.
			const glm::vec3 anchor = enemy.position + glm::vec3(0.0f, 2.0f, 0.0f);

			// Billboard basis from camera.
			const glm::vec3 right = glm::normalize(camera.Right);
			const glm::vec3 up = glm::normalize(camera.Up);

			const float width = 1.2f;
			const float height = 0.12f;

			auto fillVerts = [&](float w01, float zBias) {
				const float w = width * w01;
				const glm::vec3 r = right * (w * 0.5f);
				const glm::vec3 u = up * (height * 0.5f);
				const glm::vec3 c = anchor + camera.Front * zBias; // nudge toward camera to avoid z-fighting

				const glm::vec3 p0 = c - r - u;
				const glm::vec3 p1 = c + r - u;
				const glm::vec3 p2 = c + r + u;
				const glm::vec3 p3 = c - r + u;

				float v[18] = {
					p0.x, p0.y, p0.z,  p1.x, p1.y, p1.z,  p2.x, p2.y, p2.z,
					p0.x, p0.y, p0.z,  p2.x, p2.y, p2.z,  p3.x, p3.y, p3.z
				};
				glBindBuffer(GL_ARRAY_BUFFER, hpVBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
			};

			glDisable(GL_DEPTH_TEST);
			hpShader.use();
			hpShader.setMat4("projection", projection);
			hpShader.setMat4("view", view);

			glBindVertexArray(hpVAO);

			// background
			fillVerts(1.0f, 0.02f);
			hpShader.setVec3("color", glm::vec3(0.05f, 0.05f, 0.05f));
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// fill (left-aligned by shifting center)
			{
				const float w = width * hp01;
				const glm::vec3 shiftedAnchor = anchor - right * ((width - w) * 0.5f);
				const glm::vec3 r = right * (w * 0.5f);
				const glm::vec3 u = up * (height * 0.5f);
				const glm::vec3 c = shiftedAnchor + camera.Front * 0.03f;

				const glm::vec3 p0 = c - r - u;
				const glm::vec3 p1 = c + r - u;
				const glm::vec3 p2 = c + r + u;
				const glm::vec3 p3 = c - r + u;

				float v[18] = {
					p0.x, p0.y, p0.z,  p1.x, p1.y, p1.z,  p2.x, p2.y, p2.z,
					p0.x, p0.y, p0.z,  p2.x, p2.y, p2.z,  p3.x, p3.y, p3.z
				};
				glBindBuffer(GL_ARRAY_BUFFER, hpVBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);

				// green->red
				const glm::vec3 col = glm::mix(glm::vec3(0.85f, 0.20f, 0.18f), glm::vec3(0.20f, 0.85f, 0.25f), hp01);
				hpShader.setVec3("color", col);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			glBindVertexArray(0);
			glEnable(GL_DEPTH_TEST);
		}
		if (gWin && !gPrevWin)
		{
			std::cerr << "\n=== YOU WIN! Press R to restart ===\n";
			glfwSetWindowTitle(window, "YOU WIN! (Press R to restart)");
		}
		gPrevWin = gWin;

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	gCamYawDeg += xoffset * camera.MouseSensitivity;
	gCamPitchDeg += yoffset * camera.MouseSensitivity;
	if (gCamPitchDeg > 35.0f) gCamPitchDeg = 35.0f;
	if (gCamPitchDeg < -60.0f) gCamPitchDeg = -60.0f;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamDistance -= (float)yoffset * 0.35f;
	if (gCamDistance < 2.5f) gCamDistance = 2.5f;
	if (gCamDistance > 14.0f) gCamDistance = 14.0f;
}
