$input v_color0, v_texcoord0

#include "common.sh"

SAMPLERCUBE(u_texColor, 0);

void main()
{
	int index = int(v_texcoord0.w*4.0 + 0.5);
	vec3 dx3 = dFdx(v_texcoord0.xyz);
	vec3 dy3 = dFdy(v_texcoord0.xyz);
	vec3 decal = 0.166667 * dx3;
	vec3 sampleLeft = v_texcoord0.xyz - decal;
	vec3 sampleRight = v_texcoord0.xyz + decal;

	float left_dist = textureCube(u_texColor, sampleLeft).zyxw[index];
	float right_dist = textureCube(u_texColor, sampleRight).zyxw[index];

	float dist = 0.5 * (left_dist + right_dist);

	float dx = length(dx3);
	float dy = length(dy3);
	float w = 16.0*0.5*(dx+dy);

	vec3 sub_color = smoothstep(0.5 - w, 0.5 + w, vec3(left_dist, dist, right_dist));
	gl_FragColor.xyz = sub_color*v_color0.w;
	gl_FragColor.w = dist * v_color0.w;
}

/*
$input v_color0, v_texcoord0

#include "common.sh"

SAMPLERCUBE(u_texColor, 0);

void main()
{
	vec4 color = textureCube(u_texColor, v_texcoord0.xyz);
	int index = int(v_texcoord0.w*4.0 + 0.5);
	float rgba[4];
	rgba[0] = color.z;
	rgba[1] = color.y;
	rgba[2] = color.x;
	rgba[3] = color.w;
	float alpha = rgba[index];
	gl_FragColor = vec4(v_color0.xyz, v_color0.a * alpha);
}
*/
