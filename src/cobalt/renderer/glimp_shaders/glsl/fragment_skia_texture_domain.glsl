#version 100
precision mediump float;
uniform sampler2D uSampler0_Stage0;
uniform vec4 uTexDom_Stage0;
varying vec4 vColor;
varying vec2 vMatrixCoord_Stage0;

void main() {
  vec4 output_Stage0;
  {
    // Stage 0: TextureDomain
    {
      bvec4 outside;
      outside.xy = lessThan(vMatrixCoord_Stage0, uTexDom_Stage0.xy);
      outside.zw = greaterThan(vMatrixCoord_Stage0, uTexDom_Stage0.zw);
      output_Stage0 = any(outside) ? vec4(0.0, 0.0, 0.0, 0.0) :
          (vColor * texture2D(uSampler0_Stage0, vMatrixCoord_Stage0));
    }
  }

  gl_FragColor = output_Stage0;
}

