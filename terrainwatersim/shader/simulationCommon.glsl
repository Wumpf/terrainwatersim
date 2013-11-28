// Todo: Uniformbuffer
const float TimeStep = 1.0f / 60.0f;
const float FlowFriction_perStep = 0.998f;//pow(0.99f, TimeStep);
const float WaterAccelerationSquare_DivByArea_perStep = 100.0f * TimeStep;