attribute vec4 position;				// set automatically by OF
attribute vec2 texcoord;				// set automatically by OF 
uniform mat4 modelViewProjectionMatrix; // set automatically by OF 

//our variables
varying vec2 v_texCoord;
varying vec2 v_texCoords[9];

uniform float c_xStep;
uniform float c_yStep;

void main()
{
	//boilerplate code somewhat new to Open GL ES 2 (and later)
	gl_Position = modelViewProjectionMatrix * position;
	
	//we copy the internal texcoords so we can manipulate them
	//this is essentially the internal structure of the image
	v_texCoord = texcoord;
 
	v_texCoords[ 0] = v_texCoord + vec2(-c_xStep, -c_yStep);
	v_texCoords[ 1] = v_texCoord + vec2( 0.0, -c_yStep);
	v_texCoords[ 2] = v_texCoord + vec2( c_xStep, -c_yStep);
	v_texCoords[ 3] = v_texCoord + vec2(-c_xStep, 0.0);
	v_texCoords[ 4] = v_texCoord;
	v_texCoords[ 5] = v_texCoord + vec2( c_xStep, 0.0);
	v_texCoords[ 6] = v_texCoord + vec2(-c_xStep, c_yStep);
	v_texCoords[ 7] = v_texCoord + vec2( 0.0, c_yStep);
	v_texCoords[ 8] = v_texCoord + vec2( c_xStep, c_yStep);
}

