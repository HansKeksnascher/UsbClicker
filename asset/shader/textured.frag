#version 100

precision highp float;

uniform vec4 color;
uniform sampler2D tex;
varying vec2 texCoordFrag;

void main() {
	gl_FragColor = color * texture2D(tex, texCoordFrag);
}
