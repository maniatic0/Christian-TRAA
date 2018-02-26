template<typename T, glm::precision P>
FPSCamera<T, P>::FPSCamera(T fovy, T aspect, T nnear, T nfar) : mWorld(), mMovementSpeed(1), mMouseSensitivity(1), mFov(fovy), mAspect(aspect), mNear(nnear), mFar(nfar), mProjection(), mProjectionInverse(), mRotation(glm::tvec3<T, P>(0.0f)), mMousePosition(glm::tvec2<T, P>(0.0f))
{
	SetProjection(fovy, aspect, nnear, nfar);
}

template<typename T, glm::precision P>
FPSCamera<T, P>::~FPSCamera()
{
}

template<typename T, glm::precision P>
void FPSCamera<T, P>::SetProjection(T fovy, T aspect, T nnear, T nfar)
{
	mFov = fovy;
	mAspect = aspect;
	mNear = nnear;
	mFar = nfar;
	mProjection = glm::perspective(fovy, aspect, nnear, nfar);
	mProjectionInverse = glm::inverse(mProjection);
	frameCount = -1;
	mProjectionJitter = mProjection;
	mProjectionInverse = glm::inverse(mProjectionJitter);
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::UpdateProjection(glm::vec2 window_size_inv, int sample_max)
{
	if (!jitterProjection)
	{
		// Identity
		glm::tmat4x4<T, P> jitter(static_cast<T>(0));
		jitter[0][0] = static_cast<T>(1);
		jitter[1][1] = static_cast<T>(1);
		jitter[2][2] = static_cast<T>(1);
		jitter[3][3] = static_cast<T>(1);

		mProjectionJitter = mProjection;
		mProjectionInverse = glm::inverse(mProjectionJitter);
		return (jitter);
	}


	static const glm::tvec2<T, P> offsets[] = {
		glm::tvec2<T, P>(0, 0),
		glm::tvec2<T, P>(5.000000e-01, 6.666667e-01),
		glm::tvec2<T, P>(2.500000e-01, 3.333333e-01),
		glm::tvec2<T, P>(7.500000e-01, 2.222222e-01),
		glm::tvec2<T, P>(1.250000e-01, 8.888889e-01),
		glm::tvec2<T, P>(6.250000e-01, 5.555556e-01),
		glm::tvec2<T, P>(3.750000e-01, 1.111111e-01),
		glm::tvec2<T, P>(8.750000e-01, 7.777778e-01),
		glm::tvec2<T, P>(6.250000e-02, 4.444444e-01),
		glm::tvec2<T, P>(5.625000e-01, 7.407407e-02),
		glm::tvec2<T, P>(3.125000e-01, 7.407407e-01),
		glm::tvec2<T, P>(8.125000e-01, 4.074074e-01),
		glm::tvec2<T, P>(1.875000e-01, 2.962963e-01),
		glm::tvec2<T, P>(6.875000e-01, 9.629630e-01),
		glm::tvec2<T, P>(4.375000e-01, 6.296296e-01),
		glm::tvec2<T, P>(9.375000e-01, 1.851852e-01),
		glm::tvec2<T, P>(3.125000e-02, 8.518519e-01),
		glm::tvec2<T, P>(5.312500e-01, 5.185185e-01),
		glm::tvec2<T, P>(2.812500e-01, 3.703704e-02),
		glm::tvec2<T, P>(7.812500e-01, 7.037037e-01),
		glm::tvec2<T, P>(1.562500e-01, 3.703704e-01),
		glm::tvec2<T, P>(6.562500e-01, 2.592593e-01),
		glm::tvec2<T, P>(4.062500e-01, 9.259259e-01),
		glm::tvec2<T, P>(9.062500e-01, 5.925926e-01),
		glm::tvec2<T, P>(9.375000e-02, 1.481481e-01),
		glm::tvec2<T, P>(5.937500e-01, 8.148148e-01),
		glm::tvec2<T, P>(3.437500e-01, 4.814815e-01),
		glm::tvec2<T, P>(8.437500e-01, 2.469136e-02),
		glm::tvec2<T, P>(2.187500e-01, 6.913580e-01),
		glm::tvec2<T, P>(7.187500e-01, 3.580247e-01),
		glm::tvec2<T, P>(4.687500e-01, 2.469136e-01),
		glm::tvec2<T, P>(9.687500e-01, 9.135802e-01),
		glm::tvec2<T, P>(1.562500e-02, 5.802469e-01),
		glm::tvec2<T, P>(5.156250e-01, 1.358025e-01),
		glm::tvec2<T, P>(2.656250e-01, 8.024691e-01),
		glm::tvec2<T, P>(7.656250e-01, 4.691358e-01),
		glm::tvec2<T, P>(1.406250e-01, 9.876543e-02),
		glm::tvec2<T, P>(6.406250e-01, 7.654321e-01),
		glm::tvec2<T, P>(3.906250e-01, 4.320988e-01),
		glm::tvec2<T, P>(8.906250e-01, 3.209877e-01),
		glm::tvec2<T, P>(7.812500e-02, 9.876543e-01),
		glm::tvec2<T, P>(5.781250e-01, 6.543210e-01),
		glm::tvec2<T, P>(3.281250e-01, 2.098765e-01),
		glm::tvec2<T, P>(8.281250e-01, 8.765432e-01),
		glm::tvec2<T, P>(2.031250e-01, 5.432099e-01),
		glm::tvec2<T, P>(7.031250e-01, 6.172840e-02),
		glm::tvec2<T, P>(4.531250e-01, 7.283951e-01),
		glm::tvec2<T, P>(9.531250e-01, 3.950617e-01),
		glm::tvec2<T, P>(4.687500e-02, 2.839506e-01),
		glm::tvec2<T, P>(5.468750e-01, 9.506173e-01),
		glm::tvec2<T, P>(2.968750e-01, 6.172840e-01),
		glm::tvec2<T, P>(7.968750e-01, 1.728395e-01),
		glm::tvec2<T, P>(1.718750e-01, 8.395062e-01),
		glm::tvec2<T, P>(6.718750e-01, 5.061728e-01),
		glm::tvec2<T, P>(4.218750e-01, 1.234568e-02),
		glm::tvec2<T, P>(9.218750e-01, 6.790123e-01),
		glm::tvec2<T, P>(1.093750e-01, 3.456790e-01),
		glm::tvec2<T, P>(6.093750e-01, 2.345679e-01),
		glm::tvec2<T, P>(3.593750e-01, 9.012346e-01),
		glm::tvec2<T, P>(8.593750e-01, 5.679012e-01),
		glm::tvec2<T, P>(2.343750e-01, 1.234568e-01),
		glm::tvec2<T, P>(7.343750e-01, 7.901235e-01),
		glm::tvec2<T, P>(4.843750e-01, 4.567901e-01),
		glm::tvec2<T, P>(9.843750e-01, 8.641975e-02),
		glm::tvec2<T, P>(7.812500e-03, 7.530864e-01),
		glm::tvec2<T, P>(5.078125e-01, 4.197531e-01),
		glm::tvec2<T, P>(2.578125e-01, 3.086420e-01),
		glm::tvec2<T, P>(7.578125e-01, 9.753086e-01),
		glm::tvec2<T, P>(1.328125e-01, 6.419753e-01),
		glm::tvec2<T, P>(6.328125e-01, 1.975309e-01),
		glm::tvec2<T, P>(3.828125e-01, 8.641975e-01),
		glm::tvec2<T, P>(8.828125e-01, 5.308642e-01),
		glm::tvec2<T, P>(7.031250e-02, 4.938272e-02),
		glm::tvec2<T, P>(5.703125e-01, 7.160494e-01),
		glm::tvec2<T, P>(3.203125e-01, 3.827160e-01),
		glm::tvec2<T, P>(8.203125e-01, 2.716049e-01),
		glm::tvec2<T, P>(1.953125e-01, 9.382716e-01),
		glm::tvec2<T, P>(6.953125e-01, 6.049383e-01),
		glm::tvec2<T, P>(4.453125e-01, 1.604938e-01),
		glm::tvec2<T, P>(9.453125e-01, 8.271605e-01),
		glm::tvec2<T, P>(3.906250e-02, 4.938272e-01),
		glm::tvec2<T, P>(5.390625e-01, 8.230453e-03),
		glm::tvec2<T, P>(2.890625e-01, 6.748971e-01),
		glm::tvec2<T, P>(7.890625e-01, 3.415638e-01),
		glm::tvec2<T, P>(1.640625e-01, 2.304527e-01),
		glm::tvec2<T, P>(6.640625e-01, 8.971193e-01),
		glm::tvec2<T, P>(4.140625e-01, 5.637860e-01),
		glm::tvec2<T, P>(9.140625e-01, 1.193416e-01),
		glm::tvec2<T, P>(1.015625e-01, 7.860082e-01),
		glm::tvec2<T, P>(6.015625e-01, 4.526749e-01),
		glm::tvec2<T, P>(3.515625e-01, 8.230453e-02),
		glm::tvec2<T, P>(8.515625e-01, 7.489712e-01),
		glm::tvec2<T, P>(2.265625e-01, 4.156379e-01),
		glm::tvec2<T, P>(7.265625e-01, 3.045267e-01),
		glm::tvec2<T, P>(4.765625e-01, 9.711934e-01),
		glm::tvec2<T, P>(9.765625e-01, 6.378601e-01),
		glm::tvec2<T, P>(2.343750e-02, 1.934156e-01),
		glm::tvec2<T, P>(5.234375e-01, 8.600823e-01),
		glm::tvec2<T, P>(2.734375e-01, 5.267490e-01),
		glm::tvec2<T, P>(7.734375e-01, 4.526749e-02),
		glm::tvec2<T, P>(1.484375e-01, 7.119342e-01),
		glm::tvec2<T, P>(6.484375e-01, 3.786008e-01),
		glm::tvec2<T, P>(3.984375e-01, 2.674897e-01),
		glm::tvec2<T, P>(8.984375e-01, 9.341564e-01),
		glm::tvec2<T, P>(8.593750e-02, 6.008230e-01),
		glm::tvec2<T, P>(5.859375e-01, 1.563786e-01),
		glm::tvec2<T, P>(3.359375e-01, 8.230453e-01),
		glm::tvec2<T, P>(8.359375e-01, 4.897119e-01),
		glm::tvec2<T, P>(2.109375e-01, 3.292181e-02),
		glm::tvec2<T, P>(7.109375e-01, 6.995885e-01),
		glm::tvec2<T, P>(4.609375e-01, 3.662551e-01),
		glm::tvec2<T, P>(9.609375e-01, 2.551440e-01),
		glm::tvec2<T, P>(5.468750e-02, 9.218107e-01),
		glm::tvec2<T, P>(5.546875e-01, 5.884774e-01),
		glm::tvec2<T, P>(3.046875e-01, 1.440329e-01),
		glm::tvec2<T, P>(8.046875e-01, 8.106996e-01),
		glm::tvec2<T, P>(1.796875e-01, 4.773663e-01),
		glm::tvec2<T, P>(6.796875e-01, 1.069959e-01),
		glm::tvec2<T, P>(4.296875e-01, 7.736626e-01),
		glm::tvec2<T, P>(9.296875e-01, 4.403292e-01),
		glm::tvec2<T, P>(1.171875e-01, 3.292181e-01),
		glm::tvec2<T, P>(6.171875e-01, 9.958848e-01),
		glm::tvec2<T, P>(3.671875e-01, 6.625514e-01),
		glm::tvec2<T, P>(8.671875e-01, 2.181070e-01),
		glm::tvec2<T, P>(2.421875e-01, 8.847737e-01),
		glm::tvec2<T, P>(7.421875e-01, 5.514403e-01),
		glm::tvec2<T, P>(4.921875e-01, 6.995885e-02),
		glm::tvec2<T, P>(9.921875e-01, 7.366255e-01)
	}; //halton from matlab

	   // Get new offset
	frameCount++;
	if (frameCount >= std::min(sample_max, CAMERA_JITTERING_SIZE)) frameCount -= std::min(sample_max, CAMERA_JITTERING_SIZE);

	const glm::tvec2<T, P> offset = offsets[frameCount];


	// Access mat4[col][row]
	glm::tmat4x4<T, P> jitter(static_cast<T>(0));
	jitter[0][0] = static_cast<T>(1);
	jitter[1][1] = static_cast<T>(1);
	jitter[2][2] = static_cast<T>(1);
	jitter[3][3] = static_cast<T>(1);
	// jitterSpread should be a positive number near one
	// takes offset, centers it around pixel with a maximun distance to the pixel center of jitterSpread
	jitter[3][0] = - (2.0f * offset.x - 1.0f) * static_cast<float>(window_size_inv.x) * jitterSpread ; // This is in NDC thanks to simplifications
	jitter[3][1] = - (2.0f * offset.y - 1.0f) * static_cast<float>(window_size_inv.y) * jitterSpread ; // This is in NDC thanks to simplifications

	mProjectionJitter = jitter * mProjection;
	mProjectionInverse = glm::inverse(mProjectionJitter);
	return (jitter);
}

template<typename T, glm::precision P>
void FPSCamera<T, P>::SetFov(T fovy)
{
	SetProjection(fovy, mAspect, mNear, mFar);
}

template<typename T, glm::precision P>
T FPSCamera<T, P>::GetFov()
{
	return mFov;
}

template<typename T, glm::precision P>
void FPSCamera<T, P>::SetAspect(T a)
{
	SetProjection(mFov, a, mNear, mFar);
}

template<typename T, glm::precision P>
T FPSCamera<T, P>::GetAspect()
{
	return mAspect;
}


template<typename T, glm::precision P>
void FPSCamera<T, P>::Update(double dt, InputHandler &ih)
{
	glm::tvec2<T, P> newMousePosition = glm::tvec2<T, P>(ih.GetMousePosition().x, ih.GetMousePosition().y);
	glm::tvec2<T, P> mouse_diff = newMousePosition - mMousePosition;
	mouse_diff.y = -mouse_diff.y;
	mMousePosition = newMousePosition;
	mouse_diff *= mMouseSensitivity;

	if (!ih.IsKeyboardCapturedByUI())
	{
		if ((ih.GetKeycodeState(GLFW_KEY_Z) & PRESSED)) mRotation.z -= 0.02f;
		if ((ih.GetKeycodeState(GLFW_KEY_X) & PRESSED)) mRotation.z += 0.02f;

	}

	if (!ih.IsMouseCapturedByUI() && (ih.GetMouseState(GLFW_MOUSE_BUTTON_LEFT) & PRESSED)) {
		mRotation.x -= mouse_diff.x;
		mRotation.y += mouse_diff.y;
	}

	mWorld.SetRotateX(mRotation.y);
	mWorld.RotateY(mRotation.x);
	mWorld.RotateZ(mRotation.z);

	T movementModifier = ((ih.GetKeycodeState(GLFW_MOD_SHIFT) & PRESSED)) ? 0.25f : ((ih.GetKeycodeState(GLFW_MOD_CONTROL) & PRESSED)) ? 4.0f : 1.0f;
	T movement = movementModifier * T(dt) * mMovementSpeed;

	T move = 0.0f, strafe = 0.0f, levitate = 0.0f;
	if (!ih.IsKeyboardCapturedByUI()) {
		if ((ih.GetKeycodeState(GLFW_KEY_W) & PRESSED)) move += movement;
		if ((ih.GetKeycodeState(GLFW_KEY_S) & PRESSED)) move -= movement;
		if ((ih.GetKeycodeState(GLFW_KEY_A) & PRESSED)) strafe -= movement;
		if ((ih.GetKeycodeState(GLFW_KEY_D) & PRESSED)) strafe += movement;
		if ((ih.GetKeycodeState(GLFW_KEY_Q) & PRESSED)) levitate -= movement;
		if ((ih.GetKeycodeState(GLFW_KEY_E) & PRESSED)) levitate += movement;
	}

	mWorld.Translate(mWorld.GetFront() * move);
	mWorld.Translate(mWorld.GetRight() * strafe);
	mWorld.Translate(mWorld.GetUp() * levitate);
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetViewToWorldMatrix()
{
	return mWorld.GetMatrix();
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetWorldToViewMatrix()
{
	return mWorld.GetMatrixInverse();
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetClipToWorldMatrix()
{
	return GetViewToWorldMatrix() * mProjectionInverse;
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetWorldToClipMatrix()
{
	return mProjectionJitter * GetWorldToViewMatrix();
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetWorldToClipMatrixUnjittered()
{
	return mProjection * GetWorldToViewMatrix();
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetClipToViewMatrix()
{
	return mProjectionInverse;
}

template<typename T, glm::precision P>
glm::tmat4x4<T, P> FPSCamera<T, P>::GetViewToClipMatrix()
{
	return mProjection;
}

template<typename T, glm::precision P>
glm::tvec3<T, P> FPSCamera<T, P>::GetClipToWorld(glm::tvec3<T, P> xyw)
{
	glm::tvec4<T, P> vv = GetClipToView(xyw).xyz1();
	glm::tvec3<T, P> wv = mWorld.GetMatrix().affineMul(vv);
	return wv;
}

template<typename T, glm::precision P>
glm::tvec3<T, P> FPSCamera<T, P>::GetClipToView(glm::tvec3<T, P> xyw)
{
	glm::tvec3<T, P> vv;
	vv.x = mProjectionInverse.M[0][0] * xyw.x;
	vv.y = mProjectionInverse.M[1][1] * xyw.y;
	vv.z = -xyw.w;
	return vv;
}
