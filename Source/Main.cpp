#include "Application.h"
#include "Logger.h"
#include "FirsrPersonCamera.h"
#include "ThirdPersonCamera.h"
#include "Shader.h"
#include "MatrixStack.h"
#include "Texture2D.h"
#include "Light.h"
#include "Fog.h"

#include "Rectangle.h"
#include "Cube.h"
#include "Sphere.h"
#include "ViewVolume.h"

#include "GameObject.h"
#include "BallObject.h"
#include "AgentObject.h"

// #include "Ball.h"
// #include "Obstacle.h"

#include <random>
#include <map>

std::mt19937_64 rand_generator;
std::uniform_real_distribution<float> unif_ball_position_xz(-8, 8);
std::uniform_real_distribution<float> unif_ball_position_y(2, 18);
std::uniform_real_distribution<float> unif_ball_velocity(-2, 2);
std::uniform_real_distribution<float> unif_ball_mass(0.1f, 5.0f);
std::uniform_real_distribution<float> unif_food_cell(0, 9);

enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT,
    FRONT,
    BACK
};

typedef std::tuple<GLboolean, Direction, glm::vec3> Collision;

class NexusDemo final : public Nexus::Application {
public:
	NexusDemo() {
		Settings.Width = 800;
		Settings.Height = 600;
		Settings.WindowTitle = "Game Engine #3 | Physics Engine";
		Settings.EnableDebugCallback = true;
		Settings.EnableFullScreen = false;

		Settings.EnableGhostMode = true;
		Settings.EnableFaceCulling = true;
		Settings.CullingTypeStr = "Back Face";
		Settings.ShowOriginAnd3Axes = false;
		
		Settings.UseBlinnPhongShading = true;
		Settings.UseLighting = true;
		Settings.UseDiffuseTexture = true;
		Settings.UseSpecularTexture = true;
		Settings.UseEmission = true;
		Settings.UseGamma = true;
		Settings.GammaValue = 1.143f;

		// Projection Settings Initalize
		ProjectionSettings.IsPerspective = true;
		ProjectionSettings.OrthogonalHeight = 5.0f;
		ProjectionSettings.ClippingNear = 0.1f;
		ProjectionSettings.ClippingFar = 500.0f;
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;
	}

	void Initialize() override {
		// Setting OpenGL
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Create shader program
		myShader = std::make_unique<Nexus::Shader>("Shaders/lighting.vert", "Shaders/lighting.frag");
		// myShader = std::make_unique<Nexus::Shader>("Shaders/lighting_shadow.vert", "Shaders/lighting_shadow.frag");
		// simpleDepthShader = std::make_unique<Nexus::Shader>("Shaders/simple_depth_shader.vert", "Shaders/simple_depth_shader.frag");
		// debugDepthQuad = std::make_unique<Nexus::Shader>("Shaders/debug_quad.vert", "Shaders/debug_quad_depth.frag");
		normalShader = std::make_unique<Nexus::Shader>("Shaders/normal_visualization.vs", "Shaders/normal_visualization.fs", "Shaders/normal_visualization.gs");
		
		// Create Camera
		first_camera = std::make_unique<Nexus::FirstPersonCamera>(glm::vec3(0.0f, 2.0f, 5.0f));
		third_camera = std::make_unique<Nexus::ThirdPersonCamera>(glm::vec3(0.0f, 0.0f, 5.0f));
		first_camera->SetRestrict(true);
		first_camera->SetRestrictValue(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(10.0f, 20.0f, 10.0f));
		third_camera->SetRestrict(true);
		third_camera->SetRestrictValue(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(10.0f, 20.0f, 10.0f));
		
		// Create Matrix Stack
		model = std::make_unique<Nexus::MatrixStack>();

		// Create object data
		floor = std::make_unique<Nexus::Rectangle>(20.0f, 20.0f, 10.0f, Nexus::POS_Y);
		cube = std::make_unique<Nexus::Cube>();
		sphere = std::make_unique<Nexus::Sphere>();

		view_volume = std::make_unique<Nexus::ViewVolume>();

		// Loading textures
		texture_checkerboard = Nexus::Texture2D::CreateFromFile("Resource/Textures/chessboard-metal.png", true);
		texture_checkerboard->SetWrappingParams(GL_REPEAT, GL_REPEAT);

		// Initial Light Setting
		DirLights = {
			new Nexus::DirectionalLight(glm::vec3(3.0f, -4.0f, -2.0f), true)
		};
		PointLights = {
			new Nexus::PointLight(glm::vec3(-7.1f, 14.2f, -1.4f), false),
			new Nexus::PointLight(glm::vec3(7.1f, 17.1f, 1.4f), false),
			new Nexus::PointLight(glm::vec3(-5.7f, 4.2f, -5.7f), false),
			new Nexus::PointLight(glm::vec3(2.0f, 25.0f, 2.0f), false),
		};
		SpotLights = {
			new Nexus::SpotLight(third_camera->GetPosition(), third_camera->GetFront(), false),
			new Nexus::SpotLight(first_camera->GetPosition(), first_camera->GetFront(), false)
		};
		SpotLights[0]->SetCutoff(10.0f);
		SpotLights[0]->SetOuterCutoff(25.0f);
		SpotLights[1]->SetCutoff(6.0f);
		SpotLights[1]->SetOuterCutoff(30.0f);

		// Shadow
		/*
		glGenFramebuffers(1, &depth_map_fbo);
		glGenTextures(1, &depth_map);
		glBindTexture(GL_TEXTURE_2D, depth_map);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		*/

		// Fog
		fog = std::make_unique<Nexus::Fog>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), true, 0.1f, 100.0f);
		fog->SetDensity(0.01f);

        // Red Balls
        for (unsigned int i = 0; i < 5; i++) {
            glm::vec3 ball_position = glm::vec3(unif_ball_position_xz(rand_generator), 0.4f, unif_ball_position_xz(rand_generator));
            glm::vec3 ball_velocity = glm::vec3(unif_ball_velocity(rand_generator), 0.0f, unif_ball_velocity(rand_generator));
            AgentObject temp_ball(ball_position, ball_velocity, 0.425f);
            temp_ball.Diffuse = glm::vec4(1.0f, 0.25f, 0.25f, 1.0);
            RedBalls.push_back(temp_ball);
        }

        // Green Balls
        for (unsigned int i = 0; i < 5; i++) {
            glm::vec3 ball_position = glm::vec3(unif_ball_position_xz(rand_generator), 0.4f, unif_ball_position_xz(rand_generator));
            glm::vec3 ball_velocity = glm::vec3(unif_ball_velocity(rand_generator), 0.0f, unif_ball_velocity(rand_generator));
            AgentObject temp_ball(ball_position, ball_velocity, 0.425f);
            temp_ball.Diffuse = glm::vec4(0.25f, 1.0f, 0.25f, 1.0);
            GreenBalls.push_back(temp_ball);
        }

		// Blue Balls
        BlueBalls = {
                BallObject(glm::vec3(-9.0, 0.2f, -9.0f), glm::vec3(0.0f), 0.2f),
                // BallObject(glm::vec3(1.0, 0.2f, -9.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(7.0, 0.2f, -9.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(-3.0, 0.2f, -5.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(3.0, 0.2f, -5.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(-5.0, 0.2f, 3.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(9.0, 0.2f, 3.0f), glm::vec3(0.0f), 0.2f),
                // BallObject(glm::vec3(-9.0, 0.2f, 9.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(-5.0, 0.2f, 7.0f), glm::vec3(0.0f), 0.2f),
                BallObject(glm::vec3(3.0, 0.2f, 9.0f), glm::vec3(0.0f), 0.2f),
                // BallObject(glm::vec3(9.0, 0.2f, 9.0f), glm::vec3(0.0f), 0.2f),
        };
		for (unsigned int i = 0; i < BlueBalls.size(); i++) {
            BlueBalls[i].Diffuse = glm::vec4(0.25f, 0.3f, 1.0f, 1.0);
            BlueBalls[i].IsSolid = GL_TRUE;
		}
		currnet_ball = &RedBalls[0];


		// Obstacle
		Obstacles = {
			GameObject(glm::vec3(-9.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(-7.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(-3.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(1.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(3.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(5.0, 1.01f, -7.0f)),
            GameObject(glm::vec3(7.0, 1.01f, -7.0f)),

            GameObject(glm::vec3(5.0, 1.01f, -9.0f)),
            GameObject(glm::vec3(5.0, 1.01f, -5.0f)),
            GameObject(glm::vec3(5.0, 1.01f, -3.0f)),
            GameObject(glm::vec3(5.0, 1.01f, -1.0f)),
            GameObject(glm::vec3(5.0, 1.01f, 1.0f)),
            GameObject(glm::vec3(5.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(5.0, 1.01f, 9.0f)),

            GameObject(glm::vec3(-7.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(-5.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(-3.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(3.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(7.0, 1.01f, 5.0f)),
            GameObject(glm::vec3(9.0, 1.01f, 5.0f)),

            GameObject(glm::vec3(-1.0, 1.01f, -5.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, -3.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, -1.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, 1.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, 7.0f)),
            GameObject(glm::vec3(-1.0, 1.01f, 9.0f)),

            GameObject(glm::vec3(-7.0, 1.01f, -3.0f)),
            GameObject(glm::vec3(-3.0, 1.01f, -3.0f)),
            GameObject(glm::vec3(3.0, 1.01f, -3.0f)),
            GameObject(glm::vec3(9.0, 1.01f, -3.0f)),

            GameObject(glm::vec3(1.0, 1.01f, 1.0f)),
            GameObject(glm::vec3(7.0, 1.01f, 1.0f)),
            GameObject(glm::vec3(-7.0, 1.01f, -1.0f)),
            GameObject(glm::vec3(-7.0, 1.01f, 1.0f)),
            GameObject(glm::vec3(-7.0, 1.01f, 3.0f)),
            GameObject(glm::vec3(-7.0, 1.01f, 7.0f)),
		};

        food_cell[4][0] = 1; food_cell[7][0] = 1;
        food_cell[1][1] = 1; food_cell[4][1] = 1;
        food_cell[1][2] = 1; food_cell[2][2] = 1; food_cell[3][2] = 1; food_cell[4][2] = 1; food_cell[6][2] = 1; food_cell[7][2] = 1; food_cell[8][2] = 1; food_cell[9][2] = 1;
        food_cell[1][3] = 1;
        food_cell[1][4] = 1; food_cell[4][4] = 1; food_cell[5][4] = 1; food_cell[7][4] = 1; food_cell[8][4] = 1; food_cell[9][4] = 1;
        food_cell[1][5] = 1; food_cell[4][5] = 1; food_cell[7][5] = 1;
        food_cell[1][6] = 1; food_cell[3][6] = 1; food_cell[4][6] = 1; food_cell[6][6] = 1; food_cell[7][6] = 1; food_cell[9][6] = 1;
        food_cell[4][7] = 1; food_cell[7][7] = 1;
        food_cell[0][8] = 1; food_cell[1][8] = 1; food_cell[3][8] = 1; food_cell[4][8] = 1; food_cell[5][8] = 1; food_cell[6][8] = 1; food_cell[7][8] = 1; food_cell[8][8] = 1;
        food_cell[8][9] = 1;

        GLint coord_temp = -9;
        for (GLuint i = 0; i <= 9; i++) {
            x_map.insert(std::pair<GLuint, GLint>(i, coord_temp));
            coord_temp += 2;
        }

        coord_temp = 9;
        for (GLuint i = 0; i <= 9; i++) {
            z_map.insert(std::pair<GLuint, GLint>(i, coord_temp));
            coord_temp -= 2;
        }
	}
	
	void Update() override {

	    // Every N sec generate food
	    if (static_cast<GLint>(this->CurrentTime) % 10 == 0) {
            GLint cell_index_x = static_cast<GLint>(unif_food_cell(rand_generator));
            GLint cell_index_z = static_cast<GLint>(unif_food_cell(rand_generator));
            if (food_cell[cell_index_x][cell_index_z] != 1) {
                auto coord_x = x_map.find(cell_index_x);
                auto coord_z = z_map.find(cell_index_z);
                BlueBalls.push_back(BallObject(glm::vec3(coord_x->second, 0.2f, coord_z->second), glm::vec3(0.0f), 0.2f));
                // std::cout << "Generate food" << std::endl;
                food_cell[cell_index_x][cell_index_z] = 1;
            }
	    }

	    // Camera setting;
        third_camera->SetTarget(currnet_ball->Position);

        // RedBalls
        for (unsigned int i = 0; i < this->RedBalls.size(); i++) {
            this->RedBalls[i].Edge(-10, 10, 0, 20, -10, 10);

            for(unsigned int j = 0; j < this->BlueBalls.size(); j++) {
                this->RedBalls[i].SearchFood(this->BlueBalls[j]);
                Collision collision = CollisionWithObstacle(this->RedBalls[i], this->BlueBalls[j]);
                if (std::get<0>(collision)) {
                    // Collision Resolution
                    if (this->BlueBalls[j].Destroyed == GL_FALSE) {
                        if (this->RedBalls[i].GrowUp()) {
                            this->RedBalls[i].LastFoodFrame = this->CurrentTime;
                            this->BlueBalls[j].Destroyed = GL_TRUE;
                        }
                    }
                }
            }

            for(unsigned int j = 0; j < this->GreenBalls.size(); j++) {
                this->RedBalls[i].Runaway(this->GreenBalls[j]);
                Collision collision = CollisionWithObstacle(this->RedBalls[i], this->GreenBalls[j]);
                if (std::get<0>(collision)) {
                    // Collision Resolution
                    if (this->BlueBalls[j].Destroyed == GL_FALSE) {
                        if (this->RedBalls[i].GrowUp()) {
                            this->RedBalls[i].LastFoodFrame = this->CurrentTime;
                            this->BlueBalls[j].Destroyed = GL_TRUE;
                        }
                    }
                }
            }

            for(unsigned int j = 0; j < Obstacles.size(); j++) {
                Collision collision = CollisionWithObstacle(this->RedBalls[i], Obstacles[j]);
                if (std::get<0>(collision)) {
                    // Collision Resolution
                    Direction dir = std::get<1>(collision);
                    glm::vec3 diff_vector = std::get<2>(collision);
                    if (dir == LEFT || dir == RIGHT) {
                        this->RedBalls[i].Velocity.x = -this->RedBalls[i].Velocity.x;
                        GLfloat penetration = this->RedBalls[i].Radius - std::abs(diff_vector.x);
                        if (dir == LEFT) {
                            this->RedBalls[i].Position.x -= penetration;
                        } else {
                            this->RedBalls[i].Position.x += penetration;
                        }
                    } else if (dir == UP || dir == DOWN) {
                        this->RedBalls[i].Velocity.y = -this->RedBalls[i].Velocity.y;
                        GLfloat penetration = this->RedBalls[i].Radius - std::abs(diff_vector.y);
                        if (dir == DOWN) {
                            this->RedBalls[i].Position.y -= penetration;
                        } else {
                            this->RedBalls[i].Position.y += penetration;
                        }
                    } else if (dir == FRONT || dir == BACK) {
                        this->RedBalls[i].Velocity.z = -this->RedBalls[i].Velocity.z;
                        GLfloat penetration = this->RedBalls[i].Radius - std::abs(diff_vector.z);
                        if (dir == BACK) {
                            this->RedBalls[i].Position.z -= penetration;
                        } else {
                            this->RedBalls[i].Position.z += penetration;
                        }
                    }
                }
            }

            if (this->CurrentTime - this->RedBalls[i].LastFoodFrame > 15.0f) {
                this->RedBalls[i].Hungry();
                this->RedBalls[i].LastFoodFrame = this->CurrentTime;
            }

            this->RedBalls[i].Update(DeltaTime);
        }

        // GreenBalls
        for (unsigned int i = 0; i < this->GreenBalls.size(); i++) {
            this->GreenBalls[i].Edge(-10, 10, 0, 20, -10, 10);
            /*
            for(unsigned int j = (i + 1); j < this->BlueBalls.size(); j++) {
                this->BlueBalls[i].CollisionWithBall(this->BlueBalls[j], elasticities);
            }
            */
            for(unsigned int j = 0; j < Obstacles.size(); j++) {
                Collision collision = CollisionWithObstacle(this->GreenBalls[i], Obstacles[j]);
                if (std::get<0>(collision)) {
                    // Collision Resolution
                    Direction dir = std::get<1>(collision);
                    glm::vec3 diff_vector = std::get<2>(collision);
                    if (dir == LEFT || dir == RIGHT) {
                        this->GreenBalls[i].Velocity.x = -this->GreenBalls[i].Velocity.x;
                        GLfloat penetration = this->GreenBalls[i].Radius - std::abs(diff_vector.x);
                        if (dir == LEFT) {
                            this->GreenBalls[i].Position.x -= penetration;
                        } else {
                            this->GreenBalls[i].Position.x += penetration;
                        }
                    } else if (dir == UP || dir == DOWN) {
                        this->GreenBalls[i].Velocity.y = -this->GreenBalls[i].Velocity.y;
                        GLfloat penetration = this->GreenBalls[i].Radius - std::abs(diff_vector.y);
                        if (dir == DOWN) {
                            this->GreenBalls[i].Position.y -= penetration;
                        } else {
                            this->GreenBalls[i].Position.y += penetration;
                        }
                    } else if (dir == FRONT || dir == BACK) {
                        this->GreenBalls[i].Velocity.z = -this->GreenBalls[i].Velocity.z;
                        GLfloat penetration = this->GreenBalls[i].Radius - std::abs(diff_vector.z);
                        if (dir == BACK) {
                            this->GreenBalls[i].Position.z -= penetration;
                        } else {
                            this->GreenBalls[i].Position.z += penetration;
                        }
                    }
                }
            }
            this->GreenBalls[i].Update(DeltaTime);
        }

        // View Volume Vertices Updated
        SetViewMatrix(Nexus::DISPLAY_MODE_DEFAULT);
        view_volume->UpdateVertices(
                Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV(),
                ProjectionSettings.IsPerspective,
                Settings.Width,
                Settings.Height,
                ProjectionSettings.ClippingNear,
                ProjectionSettings.ClippingFar,
                ProjectionSettings.OrthogonalHeight,
                view
        );

        std::cout << this->CurrentTime << std::endl;
	}

	void Render(Nexus::DisplayMode monitor_type) override {

	    // ========= Shadow Rendering ========
        /*
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        // 1. Rendering the depth first (For shadow mapping)
        float light_near_plane = 1.0f, light_far_plane = 20.0f;
        glm::mat4 light_view = glm::lookAt(DirLights[0]->GetDirection() * -1.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 light_projection = GetOrthoProjMatrix(-10.0f, 10.0f, -10.0f, 10.0f, light_near_plane, light_far_plane);
        glm::mat4 light_space_matrix = light_projection * light_view;
        simpleDepthShader->Use();
        simpleDepthShader->SetMat4("lightSpaceMatrix", light_space_matrix);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderSceneForDepth(simpleDepthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        */

        /*
        SetViewMatrix(monitor_type);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        debugDepthQuad->Use();
        debugDepthQuad->SetInt("depthMap", 0);
        debugDepthQuad->SetFloat("nearPlane", light_near_plane);
        debugDepthQuad->SetFloat("farPlane", light_far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth_map);
        RenderQuad();
        */
        // ========= Shadow Rendering ========


	    if (Settings.EnableFaceCulling) {
            // CW => Clockwise is the front face
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CW);
            if (Settings.CullingTypeStr == "Back Face") {
                glCullFace(GL_BACK);
            } else {
                glCullFace(GL_FRONT);
            }
        } else {
            glDisable(GL_CULL_FACE);
        }

	    // Setting Matrices
        SetViewMatrix(monitor_type);
        SetProjectionMatrix(monitor_type);
        SetViewport(monitor_type);

        // Shader parameter settings
        myShader->Use();
        myShader->SetBool("enableCulling", false);
        myShader->SetInt("material.diffuse_texture", 0);
        myShader->SetInt("material.specular_texture", 1);
        myShader->SetInt("material.emission_texture", 2);
        // myShader->SetInt("shadowMap", 4);
        myShader->SetInt("skybox", 3);

        myShader->SetMat4("view", view);
        myShader->SetMat4("projection", projection);
        myShader->SetVec3("viewPos", Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition());
        // myShader->SetMat4("lightSpaceMatrix", light_space_matrix);

        // glActiveTexture(GL_TEXTURE4);
        // glBindTexture(GL_TEXTURE_2D, depth_map);

        myShader->SetBool("useBlinnPhong", Settings.UseBlinnPhongShading);
        myShader->SetBool("useLighting", Settings.UseLighting);
        myShader->SetBool("useDiffuseTexture", Settings.UseDiffuseTexture);
        myShader->SetBool("useSpecularTexture", Settings.UseSpecularTexture);
        myShader->SetBool("useEmission", Settings.UseEmission);
        myShader->SetBool("useGamma", Settings.UseGamma);
        myShader->SetFloat("GammaValue", Settings.GammaValue);

        for (unsigned int i = 0; i < DirLights.size(); i++) {
            myShader->SetVec3("lights[" + std::to_string(i) + "].direction", DirLights[i]->GetDirection());
            myShader->SetVec3("lights[" + std::to_string(i) + "].ambient", DirLights[i]->GetAmbient());
            myShader->SetVec3("lights[" + std::to_string(i) + "].diffuse", DirLights[i]->GetDiffuse());
            myShader->SetVec3("lights[" + std::to_string(i) + "].specular", DirLights[i]->GetSpecular());
            myShader->SetBool("lights[" + std::to_string(i) + "].enable", DirLights[i]->GetEnable());
            myShader->SetInt("lights[" + std::to_string(i) + "].caster", DirLights[i]->GetCaster());
        }

        for (unsigned int i = 0; i < PointLights.size(); i++) {
            myShader->SetVec3("lights[" + std::to_string(i + 1) + "].position", PointLights[i]->GetPosition());
            myShader->SetVec3("lights[" + std::to_string(i + 1) + "].ambient", PointLights[i]->GetAmbient());
            myShader->SetVec3("lights[" + std::to_string(i + 1) + "].diffuse", PointLights[i]->GetDiffuse());
            myShader->SetVec3("lights[" + std::to_string(i + 1) + "].specular", PointLights[i]->GetSpecular());
            myShader->SetFloat("lights[" + std::to_string(i + 1) + "].constant", PointLights[i]->GetConstant());
            myShader->SetFloat("lights[" + std::to_string(i + 1) + "].linear", PointLights[i]->GetLinear());
            myShader->SetFloat("lights[" + std::to_string(i + 1) + "].quadratic", PointLights[i]->GetQuadratic());
            myShader->SetFloat("lights[" + std::to_string(i + 1) + "].enable", PointLights[i]->GetEnable());
            myShader->SetInt("lights[" + std::to_string(i + 1) + "].caster", PointLights[i]->GetCaster());
        }

        SpotLights[0]->SetPosition(third_camera->GetPosition());
        SpotLights[0]->SetDirection(third_camera->GetFront());
        SpotLights[1]->SetPosition(first_camera->GetPosition());
        SpotLights[1]->SetDirection(first_camera->GetFront());
        for (unsigned int i = 0; i < SpotLights.size(); i++) {
            myShader->SetVec3("lights[" + std::to_string(i + 5) + "].position", SpotLights[i]->GetPosition());
            myShader->SetVec3("lights[" + std::to_string(i + 5) + "].direction", SpotLights[i]->GetDirection());
            myShader->SetVec3("lights[" + std::to_string(i + 5) + "].ambient", SpotLights[i]->GetAmbient());
            myShader->SetVec3("lights[" + std::to_string(i + 5) + "].diffuse", SpotLights[i]->GetDiffuse());
            myShader->SetVec3("lights[" + std::to_string(i + 5) + "].specular", SpotLights[i]->GetSpecular());
            myShader->SetFloat("lights[" + std::to_string(i + 5) + "].constant", SpotLights[i]->GetConstant());
            myShader->SetFloat("lights[" + std::to_string(i + 5) + "].linear", SpotLights[i]->GetLinear());
            myShader->SetFloat("lights[" + std::to_string(i + 5) + "].quadratic", SpotLights[i]->GetQuadratic());
            myShader->SetFloat("lights[" + std::to_string(i + 5) + "].cutoff", glm::cos(glm::radians(SpotLights[i]->GetCutoff())));
            myShader->SetFloat("lights[" + std::to_string(i + 5) + "].outerCutoff", glm::cos(glm::radians(SpotLights[i]->GetOuterCutoff())));
            myShader->SetBool("lights[" + std::to_string(i + 5) + "].enable", SpotLights[i]->GetEnable());
            myShader->SetInt("lights[" + std::to_string(i + 5) + "].caster", SpotLights[i]->GetCaster());
        }

        myShader->SetVec4("fog.color", fog->GetColor());
        myShader->SetFloat("fog.density", fog->GetDensity());
        myShader->SetInt("fog.mode", fog->GetMode());
        myShader->SetInt("fog.depthType", fog->GetDepthType());
        myShader->SetBool("fog.enable", fog->GetEnable());
        myShader->SetFloat("fog.f_start", fog->GetFogStart());
        myShader->SetFloat("fog.f_end", fog->GetFogEnd());

        for (unsigned int i = 0; i < 6; i++) {
            if (i <= 2) {
                myShader->SetVec3("clippingPlanes[" + std::to_string(i) + "].position", view_volume->NearPlaneVertex[0]);
            } else {
                myShader->SetVec3("clippingPlanes[" + std::to_string(i) + "].position", view_volume->FarPlaneVertex[1]);
            }
            myShader->SetVec3("clippingPlanes[" + std::to_string(i) + "].normal", view_volume->ViewVolumeNormal[i]);
        }

        // RenderScene(myShader);

        // ==================== Draw origin and 3 axes ====================
        if (Settings.ShowOriginAnd3Axes) {
            this->DrawOriginAnd3Axes(myShader.get());
        }

        // ==================== Draw a room ====================
        myShader->SetBool("material.enableDiffuseTexture", true);
        myShader->SetBool("material.enableSpecularTexture", false);
        myShader->SetBool("material.enableEmission", false);
        myShader->SetBool("material.enableEmissionTexture", false);
        myShader->SetFloat("material.shininess", 64.0f);
        model->Push();
        floor->BindTexture(0, texture_checkerboard.get());
        floor->Draw(myShader.get(), model->Top());

        model->Push();
        model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 20.0f, 0.0f)));
        model->Save(glm::rotate(model->Top(), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        floor->BindTexture(0, texture_checkerboard.get());
        floor->Draw(myShader.get(), model->Top());
        model->Pop();

        model->Push();
        model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 10.0f, -10.0f)));
        model->Save(glm::rotate(model->Top(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        floor->BindTexture(0, texture_checkerboard.get());
        floor->Draw(myShader.get(), model->Top());
        model->Pop();

        model->Push();
        model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 10.0f, 10.0f)));
        model->Save(glm::rotate(model->Top(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        floor->BindTexture(0, texture_checkerboard.get());
        floor->Draw(myShader.get(), model->Top());
        model->Pop();

        model->Push();
        model->Save(glm::translate(model->Top(), glm::vec3(10.0f, 10.0f, 0.0f)));
        model->Save(glm::rotate(model->Top(), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        floor->BindTexture(0, texture_checkerboard.get());
        floor->Draw(myShader.get(), model->Top());
        model->Pop();

        model->Push();
        model->Save(glm::translate(model->Top(), glm::vec3(-10.0f, 10.0f, 0.0f)));
        model->Save(glm::rotate(model->Top(), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        floor->BindTexture(0, texture_checkerboard.get());
        floor->Draw(myShader.get(), model->Top());
        model->Pop();
        model->Pop();

        // ==================== Draw Ball ====================
        myShader->SetBool("enableCulling", enalbe_ball_culling);
        myShader->SetBool("material.enableDiffuseTexture", false);
        myShader->SetBool("material.enableSpecularTexture", false);
        myShader->SetBool("material.enableEmission", false);
        myShader->SetBool("material.enableEmissionTexture", false);
        myShader->SetFloat("material.shininess", 32.0f);
        for (unsigned int i = 0; i < RedBalls.size(); i++) {
            myShader->SetVec4("material.ambient", RedBalls[i].Ambient);
            myShader->SetVec4("material.diffuse", RedBalls[i].Diffuse);
            myShader->SetVec4("material.specular", RedBalls[i].Specular);
            model->Push();
            model->Save(RedBalls[i].Model);
            sphere->Draw(myShader.get(), model->Top());
            model->Pop();
        }
        for (unsigned int i = 0; i < GreenBalls.size(); i++) {
            myShader->SetVec4("material.ambient", GreenBalls[i].Ambient);
            myShader->SetVec4("material.diffuse", GreenBalls[i].Diffuse);
            myShader->SetVec4("material.specular", GreenBalls[i].Specular);
            model->Push();
            model->Save(GreenBalls[i].Model);
            sphere->Draw(myShader.get(), model->Top());
            model->Pop();
        }
        for (unsigned int i = 0; i < BlueBalls.size(); i++) {
            if (!BlueBalls[i].Destroyed) {
                myShader->SetVec4("material.ambient", BlueBalls[i].Ambient);
                myShader->SetVec4("material.diffuse", BlueBalls[i].Diffuse);
                myShader->SetVec4("material.specular", BlueBalls[i].Specular);
                model->Push();
                model->Save(BlueBalls[i].Model);
                sphere->Draw(myShader.get(), model->Top());
                model->Pop();
            }
        }

        // ==================== Draw a cube ====================
        myShader->SetBool("enableCulling", false);
        myShader->SetBool("material.enableDiffuseTexture", false);
        myShader->SetBool("material.enableSpecularTexture", false);
        myShader->SetBool("material.enableEmission", false);
        myShader->SetBool("material.enableEmissionTexture", false);
        myShader->SetFloat("material.shininess", 16.0f);
        for (unsigned int i = 0; i < Obstacles.size(); i++) {
            // obstacles[i].Edge();
            // obstacles[i].Update(DeltaTime);
            myShader->SetVec4("material.ambient", Obstacles[i].Ambient);
            myShader->SetVec4("material.diffuse", Obstacles[i].Diffuse);
            myShader->SetVec4("material.specular", Obstacles[i].Specular);
            model->Push();
            model->Save(glm::translate(model->Top(), Obstacles[i].Position));
            model->Save(glm::scale(model->Top(), glm::vec3(2.0f)));
            cube->Draw(myShader.get(), model->Top());
            model->Pop();
        }

        // ==================== Draw View Volume ====================
        model->Push();
        myShader->SetVec4("material.ambient", glm::vec4(0.2f, 0.2f, 0.2f, 0.6f));
        myShader->SetVec4("material.diffuse", glm::vec4(0.6f, 0.6f, 0.6f, 0.6f));
        myShader->SetVec4("material.specular", glm::vec4(0.0f, 0.0, 0.0, 0.6f));
        myShader->SetFloat("material.shininess", 32.0f);
        myShader->SetMat4("model", model->Top());
        view_volume->Draw(myShader.get(), model->Top());
        model->Pop();

        // ==================== Draw Light Balls ====================
        myShader->Use();
        myShader->SetBool("material.enableDiffuseTexture", false);
        myShader->SetBool("material.enableSpecularTexture", false);
        myShader->SetBool("material.enableEmission", true);
        myShader->SetBool("material.enableEmissionTexture", false);
        for(unsigned int i = 0; i < PointLights.size(); i++) {
            if (!PointLights[i]->GetEnable()) {
                continue;
            }
            model->Push();
            model->Save(glm::translate(model->Top(), PointLights[i]->GetPosition()));
            model->Save(glm::scale(model->Top(), glm::vec3(0.5f)));
            myShader->SetVec4("material.ambient", glm::vec4(PointLights[i]->GetAmbient(), 1.0f));
            myShader->SetVec4("material.diffuse", glm::vec4(PointLights[i]->GetDiffuse(), 1.0f));
            myShader->SetVec4("material.specular", glm::vec4(PointLights[i]->GetSpecular(), 1.0f));
            myShader->SetFloat("material.shininess", 32.0f);
            sphere->Draw(myShader.get(), model->Top());
            model->Pop();
        }
        myShader->SetBool("material.enableEmission", false);

        // ImGui::ShowDemoWindow();

	}

	void RenderSceneForDepth(const std::unique_ptr<Nexus::Shader>& shader) {
		// cubes
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 1.5f, 0.0)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.5f)));
		cube->Draw(shader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(2.0f, 0.5f, 1.0)));
		cube->Draw(shader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(-1.0f, 0.8f, 2.0)));
		model->Save(glm::rotate(model->Top(), glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))));
		model->Save(glm::scale(model->Top(), glm::vec3(0.25f)));
		cube->Draw(shader.get(), model->Top());
		model->Pop();
	}

	void RenderScene(const std::unique_ptr<Nexus::Shader> &shader) {
		// floor
		shader->SetBool("material.enableDiffuseTexture", true);
		shader->SetBool("material.enableSpecularTexture", false);
		shader->SetBool("material.enableEmission", false);
		shader->SetBool("material.enableEmissionTexture", false);
		shader->SetFloat("material.shininess", 64.0f);
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(shader.get(), model->Top());

		// cubes
		shader->SetBool("material.enableDiffuseTexture", false);
		shader->SetBool("material.enableSpecularTexture", false);
		shader->SetBool("material.enableEmission", false);
		shader->SetBool("material.enableEmissionTexture", false);
		shader->SetVec4("material.ambient", glm::vec4(0.02f, 0.02f, 0.02f, 1.0));
		shader->SetVec4("material.diffuse", glm::vec4(0.1f, 0.35f, 0.1f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.45f, 0.55f, 0.45f, 1.0));
		shader->SetFloat("material.shininess", 16.0f);
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 1.5f, 0.0)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.5f)));
		cube->Draw(shader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(2.0f, 0.5f, 1.0)));
		cube->Draw(shader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(-1.0f, 0.8f, 2.0)));
		model->Save(glm::rotate(model->Top(), glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))));
		model->Save(glm::scale(model->Top(), glm::vec3(0.25f)));
		cube->Draw(shader.get(), model->Top());
		model->Pop();
	}

	void RenderQuad() {
		if (quadVAO == 0) {
			float quadVertices[] = {
				// positions			// texture Coords
				-1.0f,  1.0f, 0.0f,		0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f,		1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			};
			// Setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void ShowDebugUI() override {
		ImGui::Begin("Control Panel");
		ImGuiTabBarFlags tab_bar_flags = ImGuiBackendFlags_None;

		if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {

			if (ImGui::BeginTabItem("Camera")) {
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				if (Settings.EnableGhostMode) {
					first_camera->ShowDebugUI("Person Person Camera");
				} else {
					third_camera->ShowDebugUI("Third Person Camera");
					ImGui::BulletText("Distance: %.2f", third_camera->GetDistance());
				}
				if (ImGui::Button("Reset Position")) {
					first_camera->SetPosition(glm::vec3(0.0f, 3.0f, 5.0f));
					first_camera->SetPitch(0.0f);
					first_camera->SetYaw(0.0f);
				}
				ImGui::EndTabItem();
			}
			

			if (ImGui::BeginTabItem("Ball")) {
				ImGui::Text("Ball amount: %d", BlueBalls.size());
				ImGui::SliderFloat3("Position", glm::value_ptr(current_generate_position), -10.0, 10.0);
				ImGui::SliderFloat3("Velocity", glm::value_ptr(current_generate_velocity), -5.0, 5.0);
				ImGui::SliderFloat("Radius", &current_generate_mass, 0.1, 5.0);
				if (ImGui::Button("Generate")) {
					BallObject temp_ball(current_generate_position, current_generate_velocity, current_generate_mass);
                    BlueBalls.push_back(temp_ball);
					// currnet_ball = &BlueBalls[0];
				}
				ImGui::SameLine();
				if (ImGui::Button("Add 10")) {
					for (unsigned int i = 0; i < 10; i++) {
						glm::vec3 ball_position = glm::vec3(unif_ball_position_xz(rand_generator), unif_ball_position_y(rand_generator), unif_ball_position_xz(rand_generator));
						glm::vec3 ball_velocity = glm::vec3(unif_ball_velocity(rand_generator), unif_ball_velocity(rand_generator), unif_ball_velocity(rand_generator));
						BallObject temp_ball(ball_position, ball_velocity, unif_ball_mass(rand_generator));
                        BlueBalls.push_back(temp_ball);
					}
				}
				if(!BlueBalls.empty()) {
					if (ImGui::Button("Delete")) {
					    if (BlueBalls.size() == 1) {
                            currnet_ball = nullptr;
					    }
                        BlueBalls.pop_back();
					}
                    ImGui::SameLine();
					if(BlueBalls.size() >= 10) {
						if (ImGui::Button("Delete 10")) {
							for (unsigned int i = 0; i < 10; i++) {
                                BlueBalls.pop_back();
							}
						}
					}
                    ImGui::SameLine();
					if (ImGui::Button("Delete All")) {
                        currnet_ball = nullptr;
                        BlueBalls.clear();
					}
				}
				ImGui::SliderFloat("Gravity", &gravity, 0.0f, 10.0f, "%2.3f m/s^2");
				ImGui::SliderFloat("Elasticities", &elasticities, 0.0f, 1.0f, "%2.4f");
				ImGui::SliderFloat("Drag Force", &dragforce, 0.0f, 1.0f, "%2.3f");

				if (!Settings.EnableGhostMode && currnet_ball != nullptr) {
                    if (ImGui::TreeNode("Select Ball Information")) {
                        glm::vec3 p = currnet_ball->Position;
                        glm::vec3 v = currnet_ball->Velocity;
                        glm::vec3 a = currnet_ball->Acceleration;
                        glm::vec3 f = currnet_ball->Force;
                        ImGui::BulletText("Radius: %.2f m", currnet_ball->Radius);
                        ImGui::BulletText("Mass: %.2f kg", currnet_ball->Mass);
                        ImGui::BulletText("Position: (%.2f, %.2f, %.2f)", p.x, p.y, p.z);
                        ImGui::BulletText("Velocity: (%.2f, %.2f, %.2f) | %.2f m/s", v.x, v.y, v.z, glm::length(v));
                        ImGui::BulletText("Acceleration: (%.2f, %.2f, %.2f)| %.2f m/s^2", a.x, a.y, a.z, glm::length(a));
                        ImGui::BulletText("Net Force: (%.2f, %.2f, %.2f) | %.2f N", f.x, f.y, f.z, glm::length(f));

                        ImGui::BulletText("Distance: (%.2f, %.2f, %.2f, %.2f)", currnet_ball->DistanceMemberFunc["very close"], currnet_ball->DistanceMemberFunc["close"], currnet_ball->DistanceMemberFunc["far"], currnet_ball->DistanceMemberFunc["very far"]);
                        ImGui::BulletText("Speed: (%.2f, %.2f, %.2f)", currnet_ball->SpeedMemberFunc["slow"], currnet_ball->SpeedMemberFunc["moderate"], currnet_ball->SpeedMemberFunc["fast"]);
                        ImGui::BulletText("Hold: (%.2f)", currnet_ball->Action["hold"]);
                        ImGui::BulletText("Walk: (%.2f)", currnet_ball->Action["walk"]);
                        ImGui::BulletText("Run: (%.2f)", currnet_ball->Action["run"]);
                    }
                    ImGui::TreePop();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Projection")) {

				ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), (ProjectionSettings.IsPerspective) ? "Perspective Projection" : "Orthogonal Projection");
				ImGui::Text("Parameters");
				ImGui::BulletText("FoV = %.2f deg, Aspect = %.2f", Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV(), ProjectionSettings.Aspect);
				if (!ProjectionSettings.IsPerspective) {
					ImGui::SliderFloat("Length", &ProjectionSettings.OrthogonalHeight, 1.0f, 100.0f);
				}
				ImGui::BulletText("left: %.2f, right: %.2f ", view_volume->ClippingParameters[2], view_volume->ClippingParameters[3]);
				ImGui::BulletText("bottom: %.2f, top: %.2f ", view_volume->ClippingParameters[0], view_volume->ClippingParameters[1]);
				ImGui::DragFloatRange2("Near & Far", &ProjectionSettings.ClippingNear, &ProjectionSettings.ClippingFar, 0.1f, 0.1f, 500.0f);
				ImGui::Spacing();

				if (ImGui::TreeNode("Projection Matrix")) {
					SetProjectionMatrix(Nexus::DISPLAY_MODE_DEFAULT);
					glm::mat4 proj = projection;

					ImGui::Columns(4, "mycolumns");
					ImGui::Separator();
					for (int i = 0; i < 4; i++) {
						ImGui::Text("%.2f", proj[0][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[1][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[2][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[3][i]); ImGui::NextColumn();
						ImGui::Separator();
						
					}
					ImGui::Columns(1);

					ImGui::TreePop();
				}
				ImGui::Spacing();
				
				if (ImGui::TreeNode("View Volume Vertices")) {
					view_volume->ShowViewVolumeVerticesImGUI();
					ImGui::TreePop();
				}
				ImGui::Spacing();

				if (ImGui::TreeNode("View Volume Normals")) {
					view_volume->ShowViewVolumeNormalsImGUI();
					ImGui::TreePop();
				}
				ImGui::Spacing();

				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Lightning")) {

				ImGui::Text("Lightning Model: %s", Settings.UseBlinnPhongShading ? "Blinn-Phong" : "Phong");
				ImGui::Checkbox("Blinn Phong Shading", &Settings.UseBlinnPhongShading);
				ImGui::Checkbox("Lightning", &Settings.UseLighting);
				ImGui::Checkbox("Diffuse Texture", &Settings.UseDiffuseTexture);
				ImGui::Checkbox("Specular Texture", &Settings.UseSpecularTexture);
				ImGui::Checkbox("Emission", &Settings.UseEmission);
				ImGui::Checkbox("Gamma Correction", &Settings.UseGamma);
				ImGui::SliderFloat("Gamma Value", &Settings.GammaValue, 1.0f / 2.2f, 2.2f);
				ImGui::Spacing();

				for (unsigned int i = 0; i < DirLights.size(); i++) {
					if (ImGui::TreeNode(std::string("Directional Light " + std::to_string(i)).c_str())) {
						DirLights[i]->GenerateDebugUI();
						ImGui::TreePop();
					}
					ImGui::Spacing();
				}

				for (unsigned int i = 0; i < PointLights.size(); i++) {
					if (ImGui::TreeNode(std::string("Point Light " + std::to_string(i)).c_str())) {
						PointLights[i]->GenerateDebugUI();
						ImGui::TreePop();
					}
					ImGui::Spacing();
				}

				for (unsigned int i = 0; i < SpotLights.size(); i++) {
					if (ImGui::TreeNode(std::string("Spot Light " + std::to_string(i)).c_str())) {
						SpotLights[i]->GenerateDebugUI();
						ImGui::TreePop();
					}
					ImGui::Spacing();
				}

				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Fog")) {
				fog->GenerateDebugUI();
				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Illustration")) {
				ImGui::Text("Current Screen: %d", Settings.CurrentDisplyMode);
				ImGui::Text("Showing Axes: %s", Settings.ShowOriginAnd3Axes ? "True" : "false");
				ImGui::Checkbox("Face Culling", &Settings.EnableFaceCulling);
				if (ImGui::BeginCombo("Culling Type", Settings.CullingTypeStr.c_str())) {
					for (int n = 0; n < Settings.CullingTypes.size(); n++) {
						bool is_selected = (Settings.CullingTypeStr == Settings.CullingTypes[n]);
						if (ImGui::Selectable(Settings.CullingTypes[n].c_str(), is_selected)) {
							Settings.CullingTypeStr = Settings.CullingTypes[n];
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::Checkbox("Enable Ball Culling", &enalbe_ball_culling);
				// ImGui::Text("Full Screen:  %s", isfullscreen ? "True" : "false");
				ImGui::Spacing();

				ImGui::EndTabItem();
			}
			
			ImGui::EndTabBar();
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::End();
	}

    Collision CollisionWithObstacle(BallObject ball, GameObject obstacle) {
        glm::vec3 diff = ball.Position - obstacle.Position;
        // 障礙物的長寬高是2*2*2
        glm::vec3 aabb_half_extents = glm::vec3(1.0f);
        glm::vec3 clamped = glm::clamp(diff, -aabb_half_extents, aabb_half_extents);
        glm::vec3 closest = obstacle.Position + clamped;
        diff = ball.Position - closest;

        if (glm::length(diff) < ball.Radius) {
            return std::make_tuple(GL_TRUE, VectorDirection(diff), diff);
        } else {
            return std::make_tuple(GL_FALSE, UP, glm::vec3(0.0f));
        }
    }

    Collision CollisionWithObstacle(AgentObject ball, GameObject obstacle) {
        glm::vec3 diff = ball.Position - obstacle.Position;
        // 障礙物的長寬高是2*2*2
        glm::vec3 aabb_half_extents = glm::vec3(1.0f);
        glm::vec3 clamped = glm::clamp(diff, -aabb_half_extents, aabb_half_extents);
        glm::vec3 closest = obstacle.Position + clamped;
        diff = ball.Position - closest;

        if (glm::length(diff) < ball.Radius) {
            return std::make_tuple(GL_TRUE, VectorDirection(diff), diff);
        } else {
            return std::make_tuple(GL_FALSE, UP, glm::vec3(0.0f));
        }
    }

    Collision CollisionWithObstacle(AgentObject one, BallObject two) {
        glm::vec3 diff = one.Position - two.Position;
        GLfloat distance = glm::length(diff);
        if (distance <= (one.Radius + two.Radius)) {
            return std::make_tuple(GL_TRUE, VectorDirection(diff), diff);
        } else {
            return std::make_tuple(GL_FALSE, UP, glm::vec3(0.0f));
        }
    }

    Direction VectorDirection(glm::vec3 target) {
        glm::vec3 compass[] = {
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f),
                glm::vec3(0.0f, 0.0f, -1.0f),
        };

        GLfloat max = 0.0f;
        GLuint best_match = -1;
        for (GLuint i = 0; i < 6; i++) {
            GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
            if (dot_product > max) {
                max = dot_product;
                best_match = i;
            }
        }
        return static_cast<Direction>(best_match);
    }

	void DrawOriginAnd3Axes(Nexus::Shader* shader) const {
		shader->SetBool("material.enableDiffuseTexture", false);
		shader->SetBool("material.enableSpecularTexture", false);
		shader->SetBool("material.enableEmission", true);
		shader->SetBool("material.enableEmissionTexture", false);
		
		// Draw the origin (0, 0, 0)
		model->Push();
		model->Save(glm::scale(model->Top(), glm::vec3(0.1f, 0.1f, 0.1f)));
		shader->SetVec4("material.ambient", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		shader->SetVec4("material.specular", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
		shader->SetFloat("material.shininess", 64.0f);
		sphere->Draw(shader, model->Top());
		model->Pop();

		// Draw x, y ,z axes.
		model->Push();
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(5.0f, 0.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(10.0f, 0.05f, 0.05f)));
		shader->SetVec4("material.ambient", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(1.0f, 0.0f, 0.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(1.0f, 0.0f, 0.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 5.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.05f, 10.0f, 0.05f)));
		shader->SetVec4("material.ambient", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.0f, 1.0f, 0.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.0f, 1.0f, 0.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 0.0f, 5.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.05f, 0.05f, 10.0f)));
		shader->SetVec4("material.ambient", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.0f, 0.0f, 1.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.0f, 0.0f, 1.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();
		model->Pop();
	}

	void SetViewMatrix(Nexus::DisplayMode monitor_type) {
		glm::vec3 camera_position = Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition();
		switch (monitor_type) {
			case Nexus::DISPLAY_MODE_ORTHOGONAL_X:
				view = glm::lookAt(glm::vec3(10.0f, 10.0f, 0.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
				break;
			case Nexus::DISPLAY_MODE_ORTHOGONAL_Y:
				view = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0, 0.0, -1.0));
				break;
			case Nexus::DISPLAY_MODE_ORTHOGONAL_Z:
				view = glm::lookAt(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
				break;
			case Nexus::DISPLAY_MODE_DEFAULT:
				view = Settings.EnableGhostMode ? first_camera->GetViewMatrix() : third_camera->GetViewMatrix();
				break;
		}
	}

	void SetProjectionMatrix(Nexus::DisplayMode monitor_type) {
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;

		if (monitor_type == Nexus::DISPLAY_MODE_DEFAULT) {
			if (ProjectionSettings.IsPerspective) {
				projection = GetPerspectiveProjMatrix(glm::radians(Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV()), ProjectionSettings.Aspect, ProjectionSettings.ClippingNear, ProjectionSettings.ClippingFar);
			} else {
				projection = GetOrthoProjMatrix(-ProjectionSettings.OrthogonalHeight * ProjectionSettings.Aspect, ProjectionSettings.OrthogonalHeight * ProjectionSettings.Aspect, -ProjectionSettings.OrthogonalHeight, ProjectionSettings.OrthogonalHeight, ProjectionSettings.ClippingNear, ProjectionSettings.ClippingFar);
			}
		} else {
			projection = GetOrthoProjMatrix(-11.0 * ProjectionSettings.Aspect, 11.0 * ProjectionSettings.Aspect, -11.0, 11.0, 0.01f, 500.0f);
		}
	}

	void SetViewport(Nexus::DisplayMode monitor_type) override {
		if (Settings.CurrentDisplyMode == Nexus::DISPLAY_MODE_3O1P) {
			switch (monitor_type) {
				case Nexus::DISPLAY_MODE_ORTHOGONAL_X:
					glViewport(0, Settings.Height / 2, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_ORTHOGONAL_Y:
					glViewport(Settings.Width / 2, Settings.Height / 2, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_ORTHOGONAL_Z:
					glViewport(0, 0, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_DEFAULT:
					glViewport(Settings.Width / 2, 0, Settings.Width / 2, Settings.Height / 2);
					break;
			}
		} else {
			glViewport(0, 0, Settings.Width, Settings.Height);
		}
	}
	
	void OnWindowResize() override {
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;
	}
	
	void OnProcessInput(int key) override {
		if (Settings.EnableGhostMode) {
			if (key == GLFW_KEY_W) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_FORWARD, DeltaTime);
			}
			if (key == GLFW_KEY_S) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_BACKWARD, DeltaTime);
			}
			if (key == GLFW_KEY_A) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_LEFT, DeltaTime);
			}
			if (key == GLFW_KEY_D) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_RIGHT, DeltaTime);
			}
		}
	}
	
	void OnKeyPress(int key) override {
		if (key == GLFW_KEY_LEFT_SHIFT) {
			if (Settings.EnableGhostMode) {
				first_camera->SetMovementSpeed(50.0f);
			}
		}

		if (key == GLFW_KEY_X) {
			if (Settings.ShowOriginAnd3Axes) {
				Settings.ShowOriginAnd3Axes = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "World coordinate origin and 3 axes: [Hide].");
			} else {
				Settings.ShowOriginAnd3Axes = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "World coordinate origin and 3 axes: [Show].");
			}
		}

		if (key == GLFW_KEY_P) {
			if (ProjectionSettings.IsPerspective) {
				ProjectionSettings.IsPerspective = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Projection Mode: Orthogonal");
			} else {
				ProjectionSettings.IsPerspective = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Projection Mode: Perspective");
			}
		}

		if (key == GLFW_KEY_G) {
			if (Settings.EnableGhostMode) {
				Settings.EnableGhostMode = false;
				SpotLights[0]->SetEnable(false);
				SpotLights[1]->SetEnable(false);
				Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: Third Person");
			} else {
				Settings.EnableGhostMode = true;
				SpotLights[0]->SetEnable(false);
				SpotLights[1]->SetEnable(false);
				Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: First Person");
			}
		}

		// 手電筒開關
		if (key == GLFW_KEY_F) {
			if (Settings.EnableGhostMode) {
				if (SpotLights[1]->GetEnable()) {
					SpotLights[1]->SetEnable(false);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 1 is turn off.");
				} else {
					SpotLights[1]->SetEnable(true);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 1 is turn on.");
				}
			} else {
				if (SpotLights[0]->GetEnable()) {
					SpotLights[0]->SetEnable(false);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 0 is turn off.");
				}
				else {
					SpotLights[0]->SetEnable(true);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 0 is turn on.");
				}
			}
		}

		// 分鏡切換
		if (key == GLFW_KEY_1) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_X;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal X.");
		}
		if (key == GLFW_KEY_2) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_Y;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal Y.");
		}
		if (key == GLFW_KEY_3) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_Z;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal Z.");
		}
		if (key == GLFW_KEY_4) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_DEFAULT;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Default Camera.");
		}
		if (key == GLFW_KEY_5) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_3O1P;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to All Screen.");
		}
	}
	
	void OnKeyRelease(int key) override {
		if (key == GLFW_KEY_LEFT_SHIFT) {
			if (Settings.EnableGhostMode) {
				first_camera->SetMovementSpeed(10.0f);
			}
		}
	}
	
	void OnMouseMove(int xoffset, int yoffset) override {
		if (!Settings.EnableCursor) {
			if (Settings.EnableGhostMode) {
				first_camera->ProcessMouseMovement(xoffset, yoffset);
			} else {
				third_camera->ProcessMouseMovement(xoffset, yoffset);
			}
		}
	}
	
	void OnMouseButtonPress(int button) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			SetCursorDisable(true);
			Settings.EnableCursor = false;
		} 
	}
	
	void OnMouseButtonRelease(int button) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			SetCursorDisable(false);
			Settings.EnableCursor = true;
		}
	}
	
	void OnMouseScroll(int yoffset) override {
		if (ProjectionSettings.IsPerspective) {
			if (Settings.EnableGhostMode) {
				first_camera->ProcessMouseScroll(yoffset);
			} else {
				third_camera->AdjustDistance(yoffset, 1.0f, 20.0f, 1.0f);
			}
		} else {
			AdjustOrthogonalProjectionWidth(yoffset);
		}
	}

	void AdjustOrthogonalProjectionWidth(float yoffset) {
		if (ProjectionSettings.OrthogonalHeight >= 1.0f && ProjectionSettings.OrthogonalHeight <= 100.0f) {
			ProjectionSettings.OrthogonalHeight -= (float)yoffset * 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight < 1.0f) {
			ProjectionSettings.OrthogonalHeight = 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight > 100.0f) {
			ProjectionSettings.OrthogonalHeight = 100.0f;
		}
	}
	
private:
	std::unique_ptr<Nexus::Shader> myShader = nullptr;
	std::unique_ptr<Nexus::Shader> normalShader = nullptr;
	std::unique_ptr<Nexus::Shader> simpleDepthShader = nullptr;
	std::unique_ptr<Nexus::Shader> debugDepthQuad = nullptr;
	
	std::unique_ptr<Nexus::FirstPersonCamera> first_camera = nullptr;
	std::unique_ptr<Nexus::ThirdPersonCamera> third_camera = nullptr;
	
	std::unique_ptr<Nexus::MatrixStack> model = nullptr;
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	std::unique_ptr<Nexus::Rectangle> floor = nullptr;
	std::unique_ptr<Nexus::Cube> cube = nullptr;
	std::unique_ptr<Nexus::Sphere> sphere = nullptr;
	std::unique_ptr<Nexus::ViewVolume> view_volume = nullptr;

	std::unique_ptr<Nexus::Texture2D> texture_checkerboard = nullptr;

	std::vector<Nexus::DirectionalLight*> DirLights;
	std::vector<Nexus::PointLight*> PointLights;
	std::vector<Nexus::SpotLight*> SpotLights;

	GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
	GLuint depth_map_fbo;
	GLuint depth_map;
	
	unsigned int quadVAO = 0;
	unsigned int quadVBO;

	std::unique_ptr<Nexus::Fog> fog;

	std::vector<AgentObject> RedBalls;
    std::vector<AgentObject> GreenBalls;
    std::vector<BallObject> BlueBalls;
	std::vector<GameObject> Obstacles;
	bool enalbe_ball_culling = false;

	float gravity = 9.81f;
	float elasticities = 0.2f;
	float dragforce = 0.2f;

    int food_cell[10][10];
    std::map<GLuint, GLint> x_map;
    std::map<GLuint, GLint> z_map;

	glm::vec3 current_generate_position = glm::vec3(0.0f);
	glm::vec3 current_generate_velocity = glm::vec3(0.0f);
	float current_generate_mass = 1.0f;
    AgentObject* currnet_ball = nullptr;
};

int main() {
	NexusDemo app;
	return app.Run();
}