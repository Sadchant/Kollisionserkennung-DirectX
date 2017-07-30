
// The pixel shader draws each pixel on the polygons that will be rendered to the screen. In this pixel shader it uses PixelInputType as input and returns
// a float4 as output which represents the final pixel color. This pixel shader program is very simple as we just tell it to color the pixel the same as
// the input value of the color. Note that the pixel shader gets its input from the vertex shader output.



//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    return input.color/0.5;
}
