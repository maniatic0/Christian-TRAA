#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	float t = fmin(fmax(x, 0.0f), 1.0f);
	//! \todo Implement this function
	return (1.0f - x) * p0 + x * p1;
}

glm::vec3
interpolation::evalLERP(const glm::vec3* ps, unsigned int n, float const x)
{
	if (n < 2)
	{
		return evalLERP(ps[0], ps[n - 1], x);
	}
	glm::vec3 interpolation = evalLERP(ps[0], ps[1], x);
	for (size_t i = 2; i < n; i++)
	{
		interpolation = evalLERP(interpolation, ps[i], x);
	}
	return interpolation;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function

	glm::vec3 ans = p1;
	ans += x * (-t * p0 + t * p2);
	ans += x * x * (2.0f * t * p0 + (t - 3.0f) * p1 + (3.0f - 2.0f * t) * p2 - t * p3);
	ans += x * x * x * (-t * p0 + (2.0f - t) * p1 + (t - 2.0f) * p2 + t * p3);
	return ans;
}


