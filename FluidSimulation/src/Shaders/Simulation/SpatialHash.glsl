const ivec2 offsets2D[9] =
{
	ivec2(-1, 1),
	ivec2(0, 1),
	ivec2(1, 1),
	ivec2(-1, 0),
	ivec2(0, 0),
	ivec2(1, 0),
	ivec2(-1, -1),
	ivec2(0, -1),
	ivec2(1, -1),
};

//Constants used for hashing
const uint hashK1 = 15823;
const uint hashK2 = 9737333;

// Convert floating point position into an integer cell coordinate
ivec2 GetCell2D(vec2 position, float radius)
{
	return ivec2(floor(position / radius));
}

// Hash cell coordinate to a single unsigned integer
uint HashCell2D(ivec2 cell)
{
	uvec2 uCell = uvec2(cell);
	uint a = uCell.x * hashK1;
	uint b = uCell.y * hashK2;
	return (a + b);
}

uint KeyFromHash(uint hash, uint tableSize)
{
	return hash % tableSize;
}