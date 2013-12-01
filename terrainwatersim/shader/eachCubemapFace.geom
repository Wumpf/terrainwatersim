#version 430

// input
layout(location = 0) in vec2 vs_out_texcoord[];

// output
layout(location = 0) out vec2 gs_out_texcoord;


void main(void)
{
    int i, layer;
    for (layer = 0; layer < 6; layer++)
    {
        gl_Layer = layer;
        for (i = 0; i < 3; i++)
        {
            gl_Position = gl_PositionIn[i];
            gs_out_texcoord = vs_out_texcoord[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}