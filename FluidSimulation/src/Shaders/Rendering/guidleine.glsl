#version 450

layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0, rgba8) uniform writeonly image2D outputImage;

// Scene definition
const int MAX_SPHERES = 4;
const int MAX_BOUNCES = 3;

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
    float reflectivity;
};

// Define spheres in the scene
Sphere spheres[MAX_SPHERES] = {
    // Ground sphere (large)
    Sphere(vec3(0.0, -1000.5, 0.0), 1000.0, vec3(0.8, 0.8, 0.8), 0.1),
    // Red sphere
    Sphere(vec3(0.0, 0.0, -1.0), 0.5, vec3(1.0, 0.2, 0.2), 0.7),
    // Blue sphere
    Sphere(vec3(-1.0, 0.0, -1.0), 0.5, vec3(0.2, 0.2, 0.8), 0.3),
    // Green sphere
    Sphere(vec3(1.0, 0.0, -1.0), 0.5, vec3(0.2, 0.8, 0.2), 0.5)
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitInfo {
    bool didHit;
    float t;
    vec3 hitPoint;
    vec3 normal;
    vec3 color;
    float reflectivity;
};

// Random number generation
uint seed = 0;
uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float randomFloat(inout uint seed) {
    return float(wang_hash(seed)) / 4294967296.0;
}

vec3 randomUnitVector(inout uint seed) {
    float z = randomFloat(seed) * 2.0 - 1.0;
    float a = randomFloat(seed) * 2.0 * 3.14159265359;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return vec3(x, y, z);
}

// Ray-sphere intersection test
HitInfo intersectSphere(Ray ray, Sphere sphere) {
    HitInfo hitInfo;
    hitInfo.didHit = false;
    
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0 * a * c;
    
    if (discriminant > 0.0) {
        float t = (-b - sqrt(discriminant)) / (2.0 * a);
        
        if (t > 0.001) {
            hitInfo.didHit = true;
            hitInfo.t = t;
            hitInfo.hitPoint = ray.origin + t * ray.direction;
            hitInfo.normal = normalize(hitInfo.hitPoint - sphere.center);
            hitInfo.color = sphere.color;
            hitInfo.reflectivity = sphere.reflectivity;
            return hitInfo;
        }
    }
    
    return hitInfo;
}

// Test intersection with all spheres and return closest hit
HitInfo intersectScene(Ray ray) {
    HitInfo closestHit;
    closestHit.didHit = false;
    closestHit.t = 999999.0;
    
    for (int i = 0; i < MAX_SPHERES; i++) {
        HitInfo hit = intersectSphere(ray, spheres[i]);
        if (hit.didHit && hit.t < closestHit.t) {
            closestHit = hit;
        }
    }
    
    return closestHit;
}

// Simple sky color
vec3 getSkyColor(Ray ray) {
    vec3 unitDirection = normalize(ray.direction);
    float t = 0.5 * (unitDirection.y + 1.0);
    return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
}

// Trace ray through the scene
vec3 traceRay(Ray ray, inout uint seed) {
    vec3 accumulatedColor = vec3(1.0);
    vec3 finalColor = vec3(0.0);
    
    for (int bounce = 0; bounce <= MAX_BOUNCES; bounce++) {
        HitInfo hit = intersectScene(ray);
        
        if (hit.didHit) {
            // Calculate diffuse reflection direction
            vec3 diffuseDir = normalize(hit.normal + randomUnitVector(seed));
            
            // Calculate reflective direction
            vec3 reflectDir = reflect(ray.direction, hit.normal);
            
            // Mix between diffuse and reflective based on material
            vec3 newDir = normalize(mix(diffuseDir, reflectDir, hit.reflectivity));
            
            // Update ray for next bounce
            ray.origin = hit.hitPoint;
            ray.direction = newDir;
            
            // Accumulate color and attenuation
            accumulatedColor *= hit.color;
            
            // Light absorption on each bounce
            accumulatedColor *= 0.7;
        } else {
            // Ray hit the sky
            finalColor = accumulatedColor * getSkyColor(ray);
            break;
        }
        
        // If we've reached max bounces without hitting sky, add some ambient light
        if (bounce == MAX_BOUNCES) {
            finalColor = accumulatedColor * vec3(0.1);
        }
    }
    
    return finalColor;
}

void main() {
    // Get the pixel coordinates
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);
    
    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y) {
        return;
    }
    
    // Initialize random seed
    seed = uint(pixelCoords.x * 1973 + pixelCoords.y * 9277 + gl_WorkGroupID.x * 26699);
    
    // Camera setup
    vec3 cameraPos = vec3(0.0, 0.0, 2.0);
    float aspectRatio = float(imageSize.x) / float(imageSize.y);
    float viewportHeight = 2.0;
    float viewportWidth = aspectRatio * viewportHeight;
    float focalLength = 1.0;
    
    vec3 viewportU = vec3(viewportWidth, 0.0, 0.0);
    vec3 viewportV = vec3(0.0, -viewportHeight, 0.0);
    vec3 viewportTopLeft = cameraPos - vec3(viewportWidth/2, -viewportHeight/2, focalLength);
    
    // Anti-aliasing samples
    const int SAMPLES = 4;
    vec3 color = vec3(0.0);
    
    for (int s = 0; s < SAMPLES; s++) {
        // Calculate normalized device coordinates with jittering for anti-aliasing
        float u = (float(pixelCoords.x) + randomFloat(seed)) / float(imageSize.x);
        float v = (float(pixelCoords.y) + randomFloat(seed)) / float(imageSize.y);
        
        // Create a ray from camera to pixel
        Ray ray;
        ray.origin = cameraPos;
        ray.direction = normalize(viewportTopLeft + u * viewportU + v * viewportV - cameraPos);
        
        // Trace the ray and accumulate color
        color += traceRay(ray, seed);
    }
    
    // Average the samples and apply gamma correction
    color = color / float(SAMPLES);
    color = sqrt(color); // Simple gamma correction (gamma 2.0)
    
    // Write to output image
    imageStore(outputImage, pixelCoords, vec4(color, 1.0));
}