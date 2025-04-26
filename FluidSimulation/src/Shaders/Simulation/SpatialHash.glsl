const ivec3 offsets3D[27] =
{
	ivec3(-1, -1, -1),
	ivec3(-1, -1, 0),
	ivec3(-1, -1, 1),
	ivec3(-1, 0, -1),
	ivec3(-1, 0, 0),
	ivec3(-1, 0, 1),
	ivec3(-1, 1, -1),
	ivec3(-1, 1, 0),
	ivec3(-1, 1, 1),
	ivec3(0, -1, -1),
	ivec3(0, -1, 0),
	ivec3(0, -1, 1),
	ivec3(0, 0, -1),
	ivec3(0, 0, 0),
	ivec3(0, 0, 1),
	ivec3(0, 1, -1),
	ivec3(0, 1, 0),
	ivec3(0, 1, 1),
	ivec3(1, -1, -1),
	ivec3(1, -1, 0),
	ivec3(1, -1, 1),
	ivec3(1, 0, -1),
	ivec3(1, 0, 0),
	ivec3(1, 0, 1),
	ivec3(1, 1, -1),
	ivec3(1, 1, 0),
	ivec3(1, 1, 1)
};

//Constants used for hashing
const uint hashK1 = 15823;
const uint hashK2 = 9737333;
const uint hashK3 = 440817757;

// Convert floating point position into an integer cell coordinate
ivec3 GetCell3D(vec3 position, float radius)
{
	return ivec3(floor(position / radius));
}

// Hash cell coordinate to a single unsigned integer
uint HashCell3D(ivec3 cell)
{
	uvec3 uCell = uvec3(cell);
	uint a = uCell.x * hashK1;
	uint b = uCell.y * hashK2;
    uint c = uCell.z * hashK3;
	return (a + b + c);
}

uint KeyFromHash(uint hash, uint tableSize)
{
	return hash % tableSize;
}