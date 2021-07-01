#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Utill.h"

constexpr float MAX_SPEED = 5.0f;

class Obstacle {
public:
	Obstacle(glm::vec3 positon, glm::vec3 velocity = glm::vec3(0.0f)) {
		this->Position = positon;
		this->Velocity = velocity;
		this->Acceleration = glm::vec3(0.0f);
		this->Model = glm::mat4(1.0f);
	}

	void Update(float delta_time) {
		this->Position = this->Position + this->Velocity * delta_time;
		this->Velocity = this->Velocity + this->Acceleration * delta_time;
		Nexus::Utill::limit(this->Velocity, MAX_SPEED);

		this->Model = glm::translate(glm::mat4(1.0f), this->Position);
	}

	void Edge() {
		if (this->Position.x + (Size.x / 2.0f) > 10.0f || this->Position.x - (Size.x / 2.0f) < -10.0f) {
			this->Velocity.x = -this->Velocity.x;
		}
		if (this->Position.y + (Size.y / 2.0f) > 20.0f || this->Position.y - (Size.y / 2.0f) < 0.0f) {
			this->Velocity.y = -this->Velocity.y;
		}
		if (this->Position.z + (Size.z / 2.0f) > 10.0f || this->Position.z - (Size.z / 2.0f) < -10.0f) {
			this->Velocity.z = -this->Velocity.z;
		}
	}

	void SetModel(glm::mat4 model) {
		this->Model = model;
	}

	glm::mat4 GetModel() const {
		return this->Model;
	}

	glm::vec3 GetPosition() const {
		return this->Position;
	}

	glm::vec3 GetSize() const {
		return this->Size;
	}
private:
	glm::vec3 Size = glm::vec3(1.0f);
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 Acceleration;
	glm::mat4 Model;
};