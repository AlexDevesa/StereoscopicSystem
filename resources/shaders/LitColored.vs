#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);

layout(location = 0) in vec4 Position;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec4 Color;
layout(location = 4) in float Material;

out vec3 vViewNormal;
out vec4 vViewPosition;
out vec4 vColor;

void main() {
    gl_Position = Projection * ModelView * Position;

    // The normal in view space
    vViewNormal = vec4(ModelView * vec4(Normal.xyz, 0)).xyz;

    // The position in view space
    vViewPosition = ModelView * Position;
    vColor = vec4(Normal, 1);
    vColor.r = Material / 5;
    vColor.g = 0;
    vColor.b = 0;
    switch (int(Material)) {
    case 0:
      vColor = vec4(1, 0, 0, 1);
      break;
    case 1:
      vColor = vec4(0, 1, 0, 1);
      break;
    case 2:
      vColor = vec4(0, 0, 1, 1);
      break;
    case 3:
      vColor = vec4(1, 0, 1, 1);
      break;
    case 4:
      vColor = vec4(0, 1, 1, 1);
      break;
    case 5:
      vColor = vec4(1, 1, 0, 1);
      break;
    default:
      vColor = vec4(1, 1, 1, 1);
      break;
    }
}
