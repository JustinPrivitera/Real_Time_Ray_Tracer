#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;

uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
vec3 n = normalize(vertex_normal);
vec3 lp=vec3(10,-20,-100);
vec3 ld = normalize(vertex_pos - lp);
float diffuse = dot(n,ld);

color.rgb = texture(tex, vertex_tex).rgb;

color.rgb *= diffuse*0.7;

vec3 cd = normalize(vertex_pos - campos);
vec3 h = normalize(cd+ld);
float spec = dot(n,h);
spec = clamp(spec,0,1);
spec = pow(spec,20);
color.rgb += vec3(1,1,1)*spec*3;
color.a=1;


}
