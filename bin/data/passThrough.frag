precision highp float; // this will make the default precision high

varying vec2 v_texCoord;
uniform sampler2D   tex0;

void main(void)
{
  gl_FragColor = texture2D( tex0, v_texCoord);
}

