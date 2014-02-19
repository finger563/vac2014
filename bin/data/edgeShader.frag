precision highp float; // this will make the default precision high

//we passed this in from our vert shader
varying vec2 v_texCoord;
varying vec2 v_texCoords[9];

//These are variables we set in our ofApp using the ofShader API

//our texture reference
//passed in by
//shader.setUniformTexture("tex0", sourceImage.getTextureReference(), sourceImage.getTextureReference().texData.textureID);
uniform sampler2D   tex0;
//uniform sampler2D   drawtex;

uniform float thresh;

//width and height that we are working with
//passed in by
//shader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
//uniform vec2        resolution;

//a changing value to work with
//passed in by
//shader.setUniform1f("time", ofGetElapsedTimef());
//uniform float       time;

const mat3 g0=1.0/(2.0*sqrt(2.0)) * mat3( 1.0, sqrt(2.0), 1.0, 0.0, 0.0, 0.0, -1.0, -sqrt(2.0), -1.0 );
const mat3 g1=1.0/(2.0*sqrt(2.0)) * mat3( 1.0, 0.0, -1.0, sqrt(2.0), 0.0, -sqrt(2.0), 1.0, 0.0, -1.0 );
const mat3 g2=1.0/(2.0*sqrt(2.0)) * mat3( 0.0, -1.0, sqrt(2.0), 1.0, 0.0, -1.0, -sqrt(2.0), 1.0, 0.0 );
const mat3 g3=1.0/(2.0*sqrt(2.0)) * mat3( sqrt(2.0), -1.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, -sqrt(2.0) );
const mat3 g4=1.0/2.0 * mat3( 0.0, 1.0, 0.0, -1.0, 0.0, -1.0, 0.0, 1.0, 0.0 );
const mat3 g5=1.0/2.0 * mat3( -1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0 );
const mat3 g6=1.0/6.0 * mat3( 1.0, -2.0, 1.0, -2.0, 4.0, -2.0, 1.0, -2.0, 1.0 );
const mat3 g7=1.0/6.0 * mat3( -2.0, 1.0, -2.0, 1.0, 4.0, 1.0, -2.0, 1.0, -2.0 );
const mat3 g8=1.0/3.0 * mat3( 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 );

const mat3 matEdge = mat3(-1.0,-1.0,-1.0,-1.0,8.0,-1.0,-1.0,-1.0,-1.0);

const mat4 RGBtoYUV = mat4(0.257,  0.439, -0.148, 0.0,
			   0.504, -0.368, -0.291, 0.0,
			   0.098, -0.071,  0.439, 0.0,
			   0.0625, 0.500,  0.500, 1.0 );

void apply_frei_chen() {
  mat3 I;
  float cnv[9];
  float dp3;
  float r,g,b;
  float M,S;

  vec3 s0,s1,s2,s3,s4,s5,s6,s7,s8;
  //vec4 gray = texture2D( tex0, v_texCoord);
  //float img_gray = length(gray.rgb);

  s0 = texture2D( tex0, v_texCoords[0]).rgb;
  s1 = texture2D( tex0, v_texCoords[1]).rgb;
  s2 = texture2D( tex0, v_texCoords[2]).rgb;
  s3 = texture2D( tex0, v_texCoords[3]).rgb;
  s4 = texture2D( tex0, v_texCoords[4]).rgb;
  s5 = texture2D( tex0, v_texCoords[5]).rgb;
  s6 = texture2D( tex0, v_texCoords[6]).rgb;
  s7 = texture2D( tex0, v_texCoords[7]).rgb;
  s8 = texture2D( tex0, v_texCoords[8]).rgb;
  // fetch the 3x3 neighbourhood and use the RGB vector's length as intensity value
  I[0][0] = s0.r;
  I[0][1] = s1.r;
  I[0][2] = s2.r;
  I[1][0] = s3.r;
  I[1][1] = s4.r;
  I[1][2] = s5.r;
  I[2][0] = s6.r;
  I[2][1] = s7.r;
  I[2][2] = s8.r;
		   
  // calculate the convolution values for all the masks
  dp3=dot(g0[0], I[0]) + dot(g0[1], I[1]) + dot(g0[2], I[2]);
  cnv[0] = dp3*dp3;

  dp3=dot(g1[0], I[0]) + dot(g1[1], I[1]) + dot(g1[2], I[2]);
  cnv[1] = dp3*dp3;

  dp3=dot(g2[0], I[0]) + dot(g2[1], I[1]) + dot(g2[2], I[2]);
  cnv[2] = dp3*dp3;

  dp3=dot(g3[0], I[0]) + dot(g3[1], I[1]) + dot(g3[2], I[2]);
  cnv[3] = dp3*dp3;

  dp3=dot(g4[0], I[0]) + dot(g4[1], I[1]) + dot(g4[2], I[2]);
  cnv[4] = dp3*dp3;

  dp3=dot(g5[0], I[0]) + dot(g5[1], I[1]) + dot(g5[2], I[2]);
  cnv[5] = dp3*dp3;

  dp3=dot(g6[0], I[0]) + dot(g6[1], I[1]) + dot(g6[2], I[2]);
  cnv[6] = dp3*dp3;

  dp3=dot(g7[0], I[0]) + dot(g7[1], I[1]) + dot(g7[2], I[2]);
  cnv[7] = dp3*dp3;

  dp3=dot(g8[0], I[0]) + dot(g8[1], I[1]) + dot(g8[2], I[2]);
  cnv[8] = dp3*dp3;

  M = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]);
  S = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]) + (cnv[8] + M);
  r = sqrt(M/S);

  if ( r > thresh )
    r = 1.0;
  else
    r = 0.0;

  I[0][0] = s0.g;
  I[0][1] = s1.g;
  I[0][2] = s2.g;
  I[1][0] = s3.g;
  I[1][1] = s4.g;
  I[1][2] = s5.g;
  I[2][0] = s6.g;
  I[2][1] = s7.g;
  I[2][2] = s8.g;
		   
  // calculate the convolution values for all the masks
  dp3=dot(g0[0], I[0]) + dot(g0[1], I[1]) + dot(g0[2], I[2]);
  cnv[0] = dp3*dp3;

  dp3=dot(g1[0], I[0]) + dot(g1[1], I[1]) + dot(g1[2], I[2]);
  cnv[1] = dp3*dp3;

  dp3=dot(g2[0], I[0]) + dot(g2[1], I[1]) + dot(g2[2], I[2]);
  cnv[2] = dp3*dp3;

  dp3=dot(g3[0], I[0]) + dot(g3[1], I[1]) + dot(g3[2], I[2]);
  cnv[3] = dp3*dp3;

  dp3=dot(g4[0], I[0]) + dot(g4[1], I[1]) + dot(g4[2], I[2]);
  cnv[4] = dp3*dp3;

  dp3=dot(g5[0], I[0]) + dot(g5[1], I[1]) + dot(g5[2], I[2]);
  cnv[5] = dp3*dp3;

  dp3=dot(g6[0], I[0]) + dot(g6[1], I[1]) + dot(g6[2], I[2]);
  cnv[6] = dp3*dp3;

  dp3=dot(g7[0], I[0]) + dot(g7[1], I[1]) + dot(g7[2], I[2]);
  cnv[7] = dp3*dp3;

  dp3=dot(g8[0], I[0]) + dot(g8[1], I[1]) + dot(g8[2], I[2]);
  cnv[8] = dp3*dp3;

  M = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]);
  S = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]) + (cnv[8] + M);
  g = sqrt(M/S);

  if (g > thresh) 
    g = 1.0;
  else 
    g = 0.0;

  I[0][0] = s0.b;
  I[0][1] = s1.b;
  I[0][2] = s2.b;
  I[1][0] = s3.b;
  I[1][1] = s4.b;
  I[1][2] = s5.b;
  I[2][0] = s6.b;
  I[2][1] = s7.b;
  I[2][2] = s8.b;
		   
  // calculate the convolution values for all the masks
  dp3=dot(g0[0], I[0]) + dot(g0[1], I[1]) + dot(g0[2], I[2]);
  cnv[0] = dp3*dp3;

  dp3=dot(g1[0], I[0]) + dot(g1[1], I[1]) + dot(g1[2], I[2]);
  cnv[1] = dp3*dp3;

  dp3=dot(g2[0], I[0]) + dot(g2[1], I[1]) + dot(g2[2], I[2]);
  cnv[2] = dp3*dp3;

  dp3=dot(g3[0], I[0]) + dot(g3[1], I[1]) + dot(g3[2], I[2]);
  cnv[3] = dp3*dp3;

  dp3=dot(g4[0], I[0]) + dot(g4[1], I[1]) + dot(g4[2], I[2]);
  cnv[4] = dp3*dp3;

  dp3=dot(g5[0], I[0]) + dot(g5[1], I[1]) + dot(g5[2], I[2]);
  cnv[5] = dp3*dp3;

  dp3=dot(g6[0], I[0]) + dot(g6[1], I[1]) + dot(g6[2], I[2]);
  cnv[6] = dp3*dp3;

  dp3=dot(g7[0], I[0]) + dot(g7[1], I[1]) + dot(g7[2], I[2]);
  cnv[7] = dp3*dp3;

  dp3=dot(g8[0], I[0]) + dot(g8[1], I[1]) + dot(g8[2], I[2]);
  cnv[8] = dp3*dp3;

  M = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]);
  S = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]) + (cnv[8] + M);
  b = sqrt(M/S);

  if (b > thresh) 
    b = 1.0;
  else 
    b = 0.0;

  float alpha = 1.0;
  if (r == 1.0) {
    alpha=alpha - 0.45;
  }
  if (b == 1.0) {
    alpha = alpha - 0.25;
  }
  if (g == 1.0 ) {
    alpha = alpha - 0.3;
  }

  //vec4 rgb = gray;
  //vec4 rgb = vec4(r,g,img_gray,1);
  //vec4 rgb = (gray.rgb,alpha);
  //gray.w = alpha;
  //gl_FragColor=gray;
  
  vec4 rgb = vec4(r,g,b,1);
  gl_FragColor = rgb;
  //vec4 yuv = RGBtoYUV * rgb;
  //gl_FragColor = yuv;
}

void apply_blur(){
}

void apply_dist_trans() {
}

void main(void)
{
  apply_blur();
  apply_frei_chen();
  apply_dist_trans();
}
