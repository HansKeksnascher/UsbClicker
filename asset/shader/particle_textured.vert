#version 100

precision highp float;

attribute vec2 position;
attribute vec2 texCoord;
attribute vec2 instanceOffset;
attribute vec4 instanceColor;
uniform mat4 projection;
varying vec2 texCoordFrag;
varying vec4 instanceColorFrag;

void main() {
	gl_Position = projection * vec4(position + instanceOffset, 1.0, 1.0);
	texCoordFrag = texCoord;
	instanceColorFrag = instanceColor;
}
