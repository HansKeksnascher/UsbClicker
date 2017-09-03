#version 100

precision highp float;

uniform vec4 color;
varying vec4 instanceColorFrag;

void main() {
	gl_FragColor = color * instanceColorFrag;
}
