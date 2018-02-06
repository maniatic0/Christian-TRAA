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
glm::tmat4x4<T, P> FPSCamera<T, P>::UpdateProjection(glm::vec2 window_size_inv)
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
		glm::tvec2<T, P>(0.0, 0.0),
		glm::tvec2<T, P>(0.500000000000000, 0.666666666666667),
		glm::tvec2<T, P>(0.250000000000000, 0.333333333333333),
		glm::tvec2<T, P>(0.750000000000000, 0.222222222222222),
		glm::tvec2<T, P>(0.125000000000000, 0.888888888888889),
		glm::tvec2<T, P>(0.625000000000000, 0.555555555555556),
		glm::tvec2<T, P>(0.375000000000000, 0.111111111111111),
		glm::tvec2<T, P>(0.875000000000000, 0.777777777777778),
		glm::tvec2<T, P>(0.0625000000000000, 0.444444444444444),
		glm::tvec2<T, P>(0.562500000000000, 0.0740740740740741),
		glm::tvec2<T, P>(0.312500000000000, 0.740740740740741),
		glm::tvec2<T, P>(0.812500000000000, 0.407407407407407),
		glm::tvec2<T, P>(0.187500000000000, 0.296296296296296),
		glm::tvec2<T, P>(0.687500000000000, 0.962962962962963),
		glm::tvec2<T, P>(0.437500000000000,	0.629629629629630),
		glm::tvec2<T, P>(0.937500000000000,	0.185185185185185)
	}; //halton from matlab

	   // Get new offset
	frameCount++;
	if (frameCount >= CAMERA_JITTERING_SIZE) frameCount -= CAMERA_JITTERING_SIZE;

	const glm::tvec2<T, P> offset = offsets[frameCount];

	glm::tmat4x4<T, P> jitter(static_cast<T>(0));
	jitter[0][0] = static_cast<T>(1);
	jitter[1][1] = static_cast<T>(1);
	jitter[2][2] = static_cast<T>(1);
	jitter[3][3] = static_cast<T>(1);
	jitter[3][0] = (2.0f * offset.x - 1.0f) * static_cast<float>(window_size_inv.x) * jitterSpread;
	jitter[3][1] = (2.0f * offset.y - 1.0f) * static_cast<float>(window_size_inv.y) * jitterSpread;
	
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
