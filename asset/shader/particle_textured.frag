#version 100

precision highp float;

uniform vec4 color;
uniform sampler2D tex;
varying vec2 texCoordFrag;
varying vec4 instanceColorFrag;

void main() {
	gl_FragColor = color * instanceColorFrag * texture2D(tex, texCoordFrag);
}
