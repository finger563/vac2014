precision highp float; // this will make the default precision high

//we passed this in from our vert shader
varying vec2 v_texCoord;
varying vec2 v_texCoords[9];

//These are variables we set in our ofApp using the ofShader API
uniform sampler2D   tex0;

void apply_dist_transform() {
  
  vec3 s0,s1,s2,s3,s4,s5,s6,s7,s8;
  vec3 s[8];

  s[0] = texture2D( tex0, v_texCoords[0]).rgb;
  s[1] = texture2D( tex0, v_texCoords[1]).rgb;
  s[2] = texture2D( tex0, v_texCoords[2]).rgb;
  s[3] = texture2D( tex0, v_texCoords[3]).rgb;
  //s4 = texture2D( tex0, v_texCoords[4]).rgb;
  s[4] = texture2D( tex0, v_texCoords[5]).rgb;
  s[5] = texture2D( tex0, v_texCoords[6]).rgb;
  s[6] = texture2D( tex0, v_texCoords[7]).rgb;
  s[7] = texture2D( tex0, v_texCoords[8]).rgb;

  vec3 max = vec3(0,0,0);
  for (int i=0;i<8;i++) {
    if ( max.r < s[i].r )
      max.r = s[i].r;
    if ( max.g < s[i].g )
      max.g = s[i].g;
    if ( max.b < s[i].b )
      max.b = s[i].b;
  }
  max = max - vec3(0.1,0.1,0.1);
  gl_FragColor = vec4(max,1.0);
}

void main(void) {
  apply_dist_transform();
}
