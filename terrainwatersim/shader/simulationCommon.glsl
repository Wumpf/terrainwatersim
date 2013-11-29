layout(binding = 5, shared) uniform SimulationParameters
{
	//float TimeStep;

	// Use pow(friction, TimeStep)
	float FlowFriction_perStep;

	// To be accurate: TimeStep * PipeCrosssectionArea * Gravity / CellDistance
	float WaterAcceleration_perStep;

	// To be accurate: TimeStep / CellDistance²
	float CellAreaInv_timeScaled;
};

/*
	const float TimeStep = 1.0f / 60.0f;
	const float FlowFriction_perStep = 0.9998f;//pow(0.99f, TimeStep);
	const float WaterAccelerationSquare_DivByArea_perStep = 10.0f * TimeStep;
	*/