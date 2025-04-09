
const float PI = 3.14159265358979f;

//Density kernel
float SmoothingKernel(float radius, float dst)
{
	if ( dst <= radius)
    {
        float v = radius - dst;
	    float factor =  6.f / (PI * pow(radius, 4));
	    return v * v * factor;
    }
    return 0.0f;

}

//Density gradient Kernel
float SmoothingKernelDerivative(float radius, float dst)
{
	if ( dst <= radius)
    {
        float v = radius - dst;
        float scale = 12.f / (PI * pow(radius, 4));
        return -v * scale;
    }
    return 0.f;

}

//Viscosity kernel
float SmoothingKernel6(float radius, float dst)
{
	if (dst < radius)
	{
		float v = radius * radius - dst * dst;
        float scale = 4.f / (PI * pow(radius, 8));
		return v * v * v * scale;
	}
	return 0;
}