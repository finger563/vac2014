attribute vec4 position;				// set automatically by OF
attribute vec2 texcoord;				// set automatically by OF 
uniform mat4 modelViewProjectionMatrix; // set automatically by OF 

//our variables
varying vec2 v_texCoord;
varying vec2 v_texCoords[9];

uniform float c_xStep;
uniform float c_yStep;

//const float c_xStep = 1.0/1280.0;
//const float c_yStep = 1.0/720.0;

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


/*
//This is the internal RPi vert shader for reference
precision lowp float;

attribute vec4 position;
attribute vec4 color;
attribute vec4 normal;
attribute vec2 texcoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;

varying vec4 colorVarying;
varying vec2 texCoordVarying;

uniform float usingTexture;
uniform float usingColors;

uniform vec4 globalColor;

void main(){
	gl_Position = modelViewProjectionMatrix * position;
	if(usingTexture>.5) texCoordVarying = (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;
	if(usingColors>.5) colorVarying = color*globalColor;
	else colorVarying = globalColor;
}
*/
