#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Calculate distance from the center of the quad (0, 0)
    float dist = length(fragCoord);

    // Create an anti-aliased edge using screen derivatives
    float edgeWidth = fwidth(dist);
    float alpha = 1.0 - smoothstep(1.0 - edgeWidth, 1.0, dist);

    // Discard pixels outside the circle boundary
    if (alpha <= 0.0) {
        discard;
    }

    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
