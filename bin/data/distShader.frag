precision highp float; // this will make the default precision high

//we passed this in from our vert shader
varying vec2 v_texCoord;
varying vec2 v_texCoords[9];

//These are variables we set in our ofApp using the ofShader API
uniform sampler2D   tex0;

void apply_dist_transform() {
}

void main(void) {
  apply_dist_tranform();
}
