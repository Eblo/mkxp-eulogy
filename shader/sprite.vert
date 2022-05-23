
uniform mat4 projMat;

uniform mat4 spriteMat;

uniform vec2 texSizeInv;
uniform vec2 patternSizeInv;
uniform vec2 patternScroll;
uniform vec2 patternZoom;
uniform bool renderPattern;
uniform bool patternTile;

attribute vec2 position;
attribute vec2 texCoord;

varying vec2 v_texCoord;
varying vec2 v_patCoord;

uniform int transformationType;
uniform int transformationPhase;
uniform float transformationSpeed;
uniform vec2 transformationAmplitude;
uniform float transformationFrequency;
varying float transformationDisplace;
varying vec2 v_transformationAmplitude;
varying vec2 v_transformationFactor;
varying float transformationTime;

void main()
{
	gl_Position = projMat * spriteMat * vec4(position, 0, 1);
    
    v_texCoord = texCoord * texSizeInv;
    
    if (renderPattern) {
        if (patternTile) {
            vec2 scroll = patternScroll * (patternSizeInv / texSizeInv);
            v_patCoord = (texCoord * (patternSizeInv / patternZoom)) - (scroll * patternSizeInv);
        }
        else {
            vec2 scroll = patternScroll * (patternSizeInv / texSizeInv);
            v_patCoord = (texCoord * (texSizeInv / patternZoom)) - (scroll * texSizeInv);
        }
    }

    if (transformationType) {
        transformationDisplace = transformationPhase * (transformationSpeed * 0.005555556f);
        v_transformationAmplitude = transformationAmplitude * texSizeInv;
        v_transformationFactor = texCoord * 6.2831853;
        transformationTime = transformationFrequency / transformationSpeed;
    }

}
