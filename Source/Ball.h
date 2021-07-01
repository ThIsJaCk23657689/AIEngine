#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Utill.h"
#include "Obstacle.h"

constexpr float BALL_MAX_SPEED = 5.0f;

class Ball {
public:
	Ball(glm::vec3 positon, glm::vec3 velocity, float mass = 1.0f) {
		this->Mass = mass;
		this->Position = positon;
		this->Velocity = velocity;
		this->Acceleration = glm::vec3(0.0f);

		this->Radius = this->Mass * 0.05;
        this->MomentsOfInertia = 2 / 5 * this->Mass * this->Radius * this->Radius;
        this->Angle = glm::vec3(0.0f);
        this->AngularVelocity = glm::vec3(0.0f);
        this->TangentialVelocity = glm::vec3(0.0f);
        this->AngularAcceleration = glm::vec3(0.0f);

		glm::mat4 model(1.0f);
		model = glm::translate(model, positon);
		model = glm::scale(model, glm::vec3(this->Radius));
		this->Model = model;

		this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
		this->Diffuse = glm::vec4(1.0f, 0.25f, 0.25f, 1.0);
		this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
	}

	void Update(float delta_time, float gravity, float dragforce) {
		// this->Force = dragforce * -this->Velocity;

		// Apply the weight of object;
        this->ApplyGravity(gravity);

        this->Acceleration = this->Force / this->Mass;
		this->Velocity = this->Velocity + this->Acceleration * delta_time;
        this->Position = this->Position + this->Velocity * delta_time;
		// this->Velocity = Nexus::Utill::clamp(this->Velocity, 0.0f, BALL_MAX_SPEED);

		this->AngularAcceleration = this->Torque / this->MomentsOfInertia;
		this->AngularVelocity = this->AngularVelocity + this->AngularAcceleration * delta_time;
		this->Angle = this->Angle + this->AngularVelocity * delta_time;

		glm::mat4 model(1.0f);
		model = glm::translate(model, this->Position);
		// model = glm::rotate(model, glm::radians(glm::length(this->AngularVelocity)), glm::normalize(this->AngularVelocity));
		model = glm::scale(model, glm::vec3(this->Radius));
		this->Model = model;

        this->Force = glm::vec3(0.0f);
	}

	void ViewVolumeIncludingTest(Nexus::ViewVolume* view_volume) {
		std::vector<int> sum;
		// Front
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->NearPlaneVertex[0]), view_volume->ViewVolumeNormal[0]));
		// Top
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->NearPlaneVertex[0]), view_volume->ViewVolumeNormal[1]));
		// Right
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->NearPlaneVertex[0]), view_volume->ViewVolumeNormal[2]));
		// Back
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->FarPlaneVertex[1]), view_volume->ViewVolumeNormal[3]));
		// Bottom
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->FarPlaneVertex[1]), view_volume->ViewVolumeNormal[4]));
		// Left
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->FarPlaneVertex[1]), view_volume->ViewVolumeNormal[5]));

		bool IsOutSide = false;
		bool IsIntersection = false;
		for (unsigned int i = 0; i < sum.size(); i++) {
			if (sum[i] == -1) {
				// Outside
				IsOutSide = true;
				IsIntersection = false;
				break;
			}
			
			if(sum[i] == 0) {
				// Intersection
				IsIntersection = true;
			}
		}

		if (IsOutSide) {
            // 6平面只要任一個平面是判定 OutSide 就是OutSide
			this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
			this->Diffuse = glm::vec4(0.25f, 0.25f, 1.0f, 1.0);
			this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
		} else if (IsIntersection) {
            // 6平面只要任一個平面是判定 IsIntersection (且沒有任一平面是OutSide)
			this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
			this->Diffuse = glm::vec4(0.25f, 1.0f, 0.25f, 1.0);
			this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
		} else {
			// Inside
			this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
			this->Diffuse = glm::vec4(1.0f, 0.25f, 0.25f, 1.0);
			this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
		}
	}

	void Edge(float elasticities) {
		if (this->Position.x + this->Radius > 10.0f) {
			this->Position.x = 10.0f - this->Radius;
			this->Velocity = glm::vec3(-this->Velocity.x, this->Velocity.y, this->Velocity.z);
			this->Velocity.x *= elasticities;
		} else if (this->Position.x - Radius < -10.0f) {
			this->Position.x = -10.0f + this->Radius;
			this->Velocity = glm::vec3(-this->Velocity.x, this->Velocity.y, this->Velocity.z);
            this->Velocity.x *= elasticities;
		}
		
		if (this->Position.y + this->Radius > 20.0f) {
			this->Position.y = 20.0f - this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, -this->Velocity.y, this->Velocity.z);
            this->Velocity.y *= elasticities;
		} else if (this->Position.y - Radius < 0.0f) {
			this->Position.y = 0.0f + this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, -this->Velocity.y, this->Velocity.z);
            this->Velocity.y *= elasticities;
		}
		
		if (this->Position.z + this->Radius > 10.0f) {
			this->Position.z = 10.0f - this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, this->Velocity.y, -this->Velocity.z);
            this->Velocity.z *= elasticities;
		} else if (this->Position.z - Radius < -10.0f) {
			this->Position.z = -10.0f + this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, this->Velocity.y, -this->Velocity.z);
            this->Velocity.z *= elasticities;
		}
	}

	void CollisionWithBall(Ball ball, float elasticities) {
		float distance = glm::length(this->Position - ball.Position);
		if (distance < (this->Radius + ball.Radius + 0.01f)) {
			// Collision
			glm::vec3 temp = glm::normalize(ball.Position - this->Position) * (this->Radius * 2 + ball.Radius * 2 + 0.1f);
			ball.Position = this->Position + temp * 2.0f;

			glm::vec3 temp_v = this->Velocity;
            // glm::vec3 temp_v1 = (this->Mass - ball.Mass) / (this->Mass + ball.Mass) * this->Velocity + (2 * this->Mass) / (this->Mass + ball.Mass) * ball.Velocity;
            // glm::vec3 temp_v2 = (2 * this->Mass) / (this->Mass + ball.Mass) * this->Velocity + (this->Mass + ball.Mass) / (this->Mass + ball.Mass) * ball.Velocity;

            this->Velocity = -ball.Velocity;
            ball.Velocity = temp_v;
		}
	}

	void CollisionWithObstacle(Obstacle obstacle, float elasticities) {
		glm::vec3 diff = this->Position - obstacle.GetPosition();
		glm::vec3 aabb_half_extents = obstacle.GetSize() / 2.0f;
		glm::vec3 clamped = glm::clamp(diff, -aabb_half_extents, aabb_half_extents);
		glm::vec3 closest = obstacle.GetPosition() + clamped;
		diff = this->Position - closest;

		if (glm::length(diff) < this->Radius) {
			// Collision
			this->Position = closest + glm::normalize(diff) * (this->Radius + 0.01f);
			this->Velocity = elasticities * -this->Velocity;
		}
	}

	void SetMass(float mass) {
		this->Mass = mass;
	}
	void SetModel(glm::mat4 model) {
		this->Model = model;
	}

	float GetRadius() const { return this->Radius; }
	float GetMass() const { return this->Mass; }
    glm::vec3 GetPosition() const { return this->Position; }
    glm::vec3 GetVelocity() const { return this->Velocity; }
    glm::vec3 GetAcceleration() const { return this->Acceleration; }
    glm::vec3 GetNetForce() const { return this->Force; }
	glm::mat4 GetModel() const { return this->Model; }
	glm::vec4 GetAmbient() const { return this->Ambient; }
	glm::vec4 GetDiffuse() const {
		return this->Diffuse;
	}
	glm::vec4 GetSpecular() const {
		return this->Specular;
	}

	void ApplyForce(const glm::vec3& force) {
	    this->Force += force;
	    ApplyTorque(glm::cross(this->Position, force));
	}

	void ApplyTorque(const glm::vec3& torque) {
	    this->Torque += torque;
	}

	void ApplyGravity(const float& gravity) {
	    this->ApplyForce(this->Mass * glm::vec3(0.0f, -gravity, 0.0f));
	}
	
private:
	float Radius = 0.2f;
    glm::mat4 Model;

	float Mass = 1.0f;
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 Acceleration;
    glm::vec3 Force = glm::vec3(0.0f);

	float MomentsOfInertia;
    glm::vec3 Angle;
    glm::vec3 AngularVelocity;
    glm::vec3 TangentialVelocity;
    glm::vec3 AngularAcceleration;
    glm::vec3 Torque = glm::vec3(0.0f);

	glm::vec4 Ambient;
	glm::vec4 Diffuse;
	glm::vec4 Specular;

	int ContainerTestWithAPlane(glm::vec3 position, glm::vec3 normal) {
        // 給定一個過平面的點Q(x, y ,z)和垂直於平面的法向量N(A, B, C)，可以導出平面方程式：
        // Ax + By + Cz + D = d，其中 D 是一個常數；而 d 代表的是點與平面之間的距離。
        // 因為是平面方程式（任一點與平面的距離都是0），所以令 d = 0，可以求出常數 D 為何。
        // 接下來要求點P(x0, y0, z0)到平面最近距離，可以套用公式 d = | Ax0 + By0 + Cz0 + D | / (A^2 + B^2 + C^2)^0.5
        // 因為法向量N事先有正規化成單位向量，所以長度為 1，所以可以直接把點P帶入公式去算
        // 因為這邊要去分辨點P是在平面的外側還是內側，所以我們可以把絕對值去掉
        // 所以如果算出來是正值代表是外側（與法向量夾角小於90度）；反之算出來是負值代表是內側（與法向量夾角大於90度）
        // 而因為我們是計算球體與平面的距離，這要考量到球體的半徑長度，並非只考慮球體的位置，不過好家在它是球體
        // 所以只要距離是介於 r ~ -r 之間都算是相交！
        // PS: 記住一個觀念，假設球體位置為P，而我們要計算的距離d，其實就是向量QP投影到法向量上，所以等於 ||QP|| * cos(theta) 的概念。
		
		glm::vec3 temp_1 = glm::normalize(normal);
		glm::vec3 temp_2 = this->Position - position;
		float distance = glm::dot(temp_1, temp_2);
		if(distance >= this->Radius) {
			// outside
			return -1;
		}

		if (distance <= -this->Radius) {
			// inside
			return 1;
		}

		// intersection
		return 0;
	}
};