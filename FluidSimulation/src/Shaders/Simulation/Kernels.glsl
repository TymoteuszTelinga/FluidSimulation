
const float PI = 3.14159265358979f;

//Density kernel
//SpikyKernelPow2
float SmoothingKernel(float radius, float dst)
{
	if ( dst <= radius)
    {
        float v = radius - dst;
	    float factor =  15.f / (2.f * PI * pow(radius, 5));
	    return v * v * factor;
    }
    return 0.0f;

}

//Density gradient Kernel
//DerivativeSpikyPow2
float SmoothingKernelDerivative(float radius, float dst)
{
	if ( dst <= radius)
    {
        float v = radius - dst;
        float scale = 15.f / (PI * pow(radius, 5));
        return -v * scale;
    }
    return 0.f;

}

//Viscosity kernel
//SmoothingKernelPoly6
float SmoothingKernel6(float radius, float dst)
{
	if (dst < radius)
	{
		float v = radius * radius - dst * dst;
        float scale = 315.f / (64.f * PI * pow(radius, 9));
		return v * v * v * scale;
	}
	return 0;
}