// procedural grid and photoshop overlay blend
// Photoshop Overlay Algorithm copied from:
// http://renderingpipeline.com/2012/06/photoshop-blendmodi-glsl/

uniform float gGridSize;
uniform sampler2D gTexDepth;
uniform vec2 gOffset;
varying vec2 gUV;

void main(void)
{
	float cBase = texture2D(gTexDepth, gUV).x;
	vec2 cCoords = gl_FragCoord.xy+vec2(gOffset.x,0.0);
	float cOverlay = ( int(mod(cCoords.x, gGridSize))==0||int(mod(cCoords.y,gGridSize))==0 ) ? 0.25:0.95;

	float cFinal = 0.0;
	if(cBase < 0.7)
		cFinal = 2.0*cBase*cOverlay;
	else
		cFinal = 1.0-2.0*(1.0-cOverlay)*(1.0-cBase);
	
	gl_FragColor = vec4(vec3(cFinal), 1.0);
}