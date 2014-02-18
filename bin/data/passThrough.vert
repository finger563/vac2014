attribute vec4 position;				// set automatically by OF
attribute vec2 texcoord;				// set automatically by OF 
uniform mat4 modelViewProjectionMatrix; // set automatically by OF 

//our variables
varying vec2 v_texCoord;

void main(){
  gl_Position = modelViewProjectionMatrix * position;
  v_texCoord = texcoord;
}
