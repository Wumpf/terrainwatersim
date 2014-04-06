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