#version 430

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 18) out;

// input
layout(location = 0) in vec2 vs_out_texcoord[];

// output
layout(location = 0) out vec3 gs_out_direction;

//-X, +X, -Y, +Y, -Z, +Z
const vec3 SliceDirections[6] =
{
	vec3( 1,  0,  0),
	vec3(-1,  0,  0),
	vec3( 0,  1,  0),
	vec3( 0, -1,  0),
	vec3( 0,  0, -1),
	vec3( 0,  0,  1),
};
const vec3 SliceRight[6] =
{
	vec3( 0,  0,  1),
	vec3( 0,  0,  -1),
	vec3( 1,  0,  0),
	vec3( 0,  0,  1),
	vec3( 1,  0, 0),
	vec3( -1,  0,  0),
};
const vec3 SliceUp[6] =
{
	vec3( 0,  1,  0),
	vec3( 0,  1,  0),
	vec3( 0,  0,  1),
	vec3(-1,  0,  0),
	vec3( 0,  1,  0),
	vec3( 0,  1,  0),
};

void main(void)
{


	for(int layer = 0; layer < 6; ++layer)
    {
    	gl_Layer = layer;
        for(int i = 0; i < 3; i++)
        {
        	gl_Position = gl_in[i].gl_Position;
			gs_out_direction = SliceDirections[layer] + gl_Position.x * SliceRight[layer] - gl_Position.y * SliceUp[layer];
			EmitVertex();
		}
        EndPrimitive();
    }
}