#pragma once
#include <glm/glm.hpp>
#include <array>

namespace camera
{

class FreeCamera
{
public:
	FreeCamera(const glm::vec3& eye, int width, int height, float fov, glm::vec2 range);

	FreeCamera(const FreeCamera& other);
	FreeCamera(FreeCamera&& other) noexcept;
	void operator=(const FreeCamera& other);
	void operator=(FreeCamera&& other) noexcept;

	void setProjection(float fov, int width, int height, glm::vec2 range);

	void translateForward(float deltaMM);
	void translateRight(float deltaMM);
	void translateUp(float deltaMM);

	void pitch(float deltaDegree);
	void roll(float deltaDegree);
	void yaw(float deltaDegree);

	const glm::mat4& view() const;
	const glm::mat4& viewInv() const;
	const glm::mat4& proj() const;
	const glm::mat4& projInv() const;
	const glm::mat4& viewProj() const;
	const glm::mat4& viewProjInv() const;

	const glm::vec3& position() const;
	const glm::vec3& front() const;
	const glm::vec3& up() const;
	const glm::vec3& right() const;

	bool isPointInsideFrustum(const glm::vec3& p) const;


private:

	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float MAX_PITCH = 89.0f;
	const float MIN_PITCH = -89.0f;
	const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
	const float DEFAULT_FOV = 450.0f;

	// Camera Attributes
	glm::vec3 d_position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 d_front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 d_up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 d_right;

	// Euler Angles
	float d_yaw = YAW;
	float d_pitch = PITCH;

	// view
	glm::mat4 d_view = glm::mat4(1.0);
	glm::mat4 d_proj = glm::mat4(1.0);
	glm::mat4 d_view_proj = d_proj * d_view;

	// inverse
	glm::mat4 d_view_inv = glm::inverse(d_view);
	glm::mat4 d_proj_inv = glm::inverse(d_proj);
	glm::mat4 d_view_proj_inv = glm::inverse(d_view_proj);

	// planes for frustrum
	std::array<glm::vec4, 6> d_planes;

	void updateCameraVectors();
	void updatePlanes();
};

using Light = FreeCamera;

} // end namespace fusion
