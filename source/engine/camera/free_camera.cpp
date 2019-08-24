#include "free_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace camera
{

// MEMBERS
FreeCamera::FreeCamera(const glm::vec3& eye, int width, int height, float fov, glm::vec2 range)
{
	d_position = eye;
	d_up = glm::vec3(0.0f, 1.0f, 0.0f);
	d_front = glm::vec3(0.0f, 0.0f, -1.0f);
	setProjection(fov, width, height, range);
	updateCameraVectors();
}

FreeCamera::FreeCamera(const FreeCamera& other)
{
	this->d_position = other.d_position;
	this->d_front = other.d_front;
	this->d_up = other.d_up;
	this->d_right = other.d_right;
	this->d_yaw = other.d_yaw;
	this->d_pitch = other.d_pitch;
	this->d_view = other.d_view;
	this->d_proj = other.d_proj;
	this->d_view_proj = other.d_view_proj;
	this->d_view_inv = other.d_view_inv;
	this->d_proj_inv = other.d_proj_inv;
	this->d_view_proj_inv = other.d_view_proj_inv;
	this->d_pitch = other.d_pitch;
	this->d_yaw = other.d_yaw;

	updateCameraVectors();
}

FreeCamera::FreeCamera(FreeCamera&& other) noexcept
{
	this->d_position = std::move(other.d_position);
	this->d_front = std::move(other.d_front);
	this->d_up = std::move(other.d_up);
	this->d_right = std::move(other.d_right);
	this->d_yaw = std::move(other.d_yaw);
	this->d_pitch = std::move(other.d_pitch);
	this->d_view = std::move(other.d_view);
	this->d_view = std::move(other.d_view);
	this->d_proj = std::move(other.d_proj);
	this->d_view_proj = std::move(other.d_view_proj);
	this->d_view_inv = std::move(other.d_view_inv);
	this->d_proj_inv = std::move(other.d_proj_inv);
	this->d_view_proj_inv = std::move(other.d_view_proj_inv);
	this->d_pitch = std::move(other.d_pitch);
	this->d_yaw = std::move(other.d_yaw);

	updateCameraVectors();
}

void FreeCamera::operator=(const FreeCamera& other)
{
	this->d_position = other.d_position;
	this->d_front = other.d_front;
	this->d_up = other.d_up;
	this->d_right = other.d_right;
	this->d_yaw = other.d_yaw;
	this->d_pitch = other.d_pitch;
	this->d_view = other.d_view;
	this->d_proj = other.d_proj;
	this->d_view_proj = other.d_view_proj;
	this->d_view_inv = other.d_view_inv;
	this->d_proj_inv = other.d_proj_inv;
	this->d_view_proj_inv = other.d_view_proj_inv;
	this->d_pitch = other.d_pitch;
	this->d_yaw = other.d_yaw;

	updateCameraVectors();
}

void FreeCamera::operator=(FreeCamera&& other) noexcept
{
	this->d_position = std::move(other.d_position);
	this->d_front = std::move(other.d_front);
	this->d_up = std::move(other.d_up);
	this->d_right = std::move(other.d_right);
	this->d_yaw = std::move(other.d_yaw);
	this->d_pitch = std::move(other.d_pitch);
	this->d_view = std::move(other.d_view);
	this->d_view = std::move(other.d_view);
	this->d_proj = std::move(other.d_proj);
	this->d_view_proj = std::move(other.d_view_proj);
	this->d_view_inv = std::move(other.d_view_inv);
	this->d_proj_inv = std::move(other.d_proj_inv);
	this->d_view_proj_inv = std::move(other.d_view_proj_inv);
	this->d_pitch = std::move(other.d_pitch);
	this->d_yaw = std::move(other.d_yaw);

	updateCameraVectors();
}

void FreeCamera::setProjection(float fov, int width, int height, glm::vec2 range)
{
	d_proj = glm::perspective(glm::radians(fov), (float)width / (float)height, range.x, range.y);
	d_proj_inv = glm::inverse(d_proj);
	updateCameraVectors();
}

void FreeCamera::translateForward(float deltaMM)
{
	d_position += d_front * deltaMM;
	updateCameraVectors();
}

void FreeCamera::translateRight(float deltaMM)
{
	d_position += d_right * deltaMM;
	updateCameraVectors();
}

void FreeCamera::translateUp(float deltaMM)
{
	d_position += d_up * deltaMM;
	updateCameraVectors();
}

void FreeCamera::pitch(float deltaDegree)
{
	d_pitch += deltaDegree;
	d_pitch = glm::clamp(d_pitch, MIN_PITCH, MAX_PITCH);
	updateCameraVectors();
}

void FreeCamera::yaw(float deltaDegree)
{
	d_yaw += deltaDegree;
	updateCameraVectors();
}

void FreeCamera::roll(float deltaDegree)
{
	//d_qData.roll += deltaDegree;
	throw "not impl.";
}

const glm::mat4& FreeCamera::view() const
{
	return d_view;
}

const glm::mat4& FreeCamera::viewInv() const
{
	return d_view_inv;
}

const glm::mat4& FreeCamera::proj() const
{
	return d_proj;
}

const glm::mat4& FreeCamera::projInv() const
{
	return d_proj_inv;
}

const glm::mat4& FreeCamera::viewProj() const
{
	return d_view_proj;
}

const glm::mat4& FreeCamera::viewProjInv() const
{
	return d_view_proj_inv;
}


const glm::vec3& FreeCamera::position() const
{
	return d_position;
}

const glm::vec3& FreeCamera::front() const
{
	return d_front;
}

const glm::vec3& FreeCamera::up() const
{
	return d_up;
}

const glm::vec3& FreeCamera::right() const
{
	return d_right;
}

bool FreeCamera::isPointInsideFrustum(const glm::vec3& p) const
{
	enum { A = 0, B, C, D };
	for (int i = 0; i < 6; ++i)
	{
		if ((d_planes[i][A] * p.x + d_planes[i][B] * p.y + d_planes[i][C] * p.z + d_planes[i][D]) <= 0.0f)
		{
			return false;
		}
	}
	return true;
}

void FreeCamera::updateCameraVectors()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(glm::radians(d_yaw)) * cos(glm::radians(d_pitch));
	front.y = sin(glm::radians(d_pitch));
	front.z = sin(glm::radians(d_yaw)) * cos(glm::radians(d_pitch));
	d_front = glm::normalize(front);
	// Also re-calculate the Right and Up vector
	d_right = glm::normalize(glm::cross(d_front, WORLD_UP));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	d_up = glm::normalize(glm::cross(d_right, d_front));
	d_view = glm::lookAt(d_position, d_position + d_front, d_up);
	d_view_proj = d_proj * d_view;

	// convert d_view to fusion view 
	d_view_inv = glm::inverse(d_view);
	d_view_proj_inv = glm::inverse(d_view_proj);

	// update planes
	updatePlanes();
}

void FreeCamera::updatePlanes()
{
	enum { A = 0, B, C, D };
	const float* const m = glm::value_ptr(d_view_proj);

	d_planes[0][A] = m[3] - m[0];
	d_planes[0][B] = m[7] - m[4];
	d_planes[0][C] = m[11] - m[8];
	d_planes[0][D] = m[15] - m[12];
	d_planes[0] = glm::normalize(d_planes[0]);
	d_planes[1][A] = m[3] + m[0];
	d_planes[1][B] = m[7] + m[4];
	d_planes[1][C] = m[11] + m[8];
	d_planes[1][D] = m[15] + m[12];
	d_planes[1] = glm::normalize(d_planes[1]);
	d_planes[2][A] = m[3] + m[1];
	d_planes[2][B] = m[7] + m[5];
	d_planes[2][C] = m[11] + m[9];
	d_planes[2][D] = m[15] + m[13];
	d_planes[2] = glm::normalize(d_planes[2]);
	d_planes[3][A] = m[3] - m[1];
	d_planes[3][B] = m[7] - m[5];
	d_planes[3][C] = m[11] - m[9];
	d_planes[3][D] = m[15] - m[13];
	d_planes[3] = glm::normalize(d_planes[3]);
	d_planes[4][A] = m[3] - m[2];
	d_planes[4][B] = m[7] - m[6];
	d_planes[4][C] = m[11] - m[10];
	d_planes[4][D] = m[15] - m[14];
	d_planes[4] = glm::normalize(d_planes[4]);
	d_planes[5][A] = m[3] + m[2];
	d_planes[5][B] = m[7] + m[6];
	d_planes[5][C] = m[11] + m[10];
	d_planes[5][D] = m[15] + m[14];
	d_planes[5] = glm::normalize(d_planes[5]);

}


} // end namespace fusion
