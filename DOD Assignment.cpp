// DOD Assignment.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <iostream>
#include <thread>
#include <algorithm>
#include <string>
#include <ctime>
#include <condition_variable>
using namespace tle;

// MAX AMOUNT OF CIRCLES
const int CIRCLE_AMOUNT = 1500;
/*WALL VARIABLES*/
const int MAX_BORDER_X = 4000;
const int MIN_BORDER_X = 0;
const int MAX_BORDER_Y = 4000;
const int MIN_BORDER_Y = 0;
/***************/
/*SPAWNING VARIABLES FOR SPAWN XY AND VELOCITY*/
const int MAX_SPAWN_DIST = 3800; //2000
const int MIN_SPAWN_DIST = 200; //-1000
const int MAX_VELOCITY = 100;
const int MIN_VELOCITY = -50;
/*********************************************/

// CHANGE IF YOU ONLY WANT A CONSOLE APPLICATION
const bool RENDERING_DISPLAY = true;
// CHANGE IF YOU WANT MOVING SPHERES TO COLLIDE
const bool SPHERE_MOVING_COLLISIONS = true;
// CHANGE IF YOU WANT SPEHERE TO NOT DIE
const bool SPHERE_DEATH = true;
// CHANGE IF YOU WANT SPEHERE TO HAVE RAND RADIUS
const bool SPHERE_RAND_RADIUS = false;
// CHANGE IF YOU WANT OUTER WALLS
const bool OUTER_WALLS = true;

/*THREAD VARIABLES, MAX THREADS AND END INDEX FOR EACH THREAD*/
/*FOR EXAMPLE STARTS AT 0 - 2, 2 - 4, 4 - 9, 9 - 10*/
const int MaxNumWorkers = 15;
const int MovementThreadAmount = 2;
const int QuadTreeThreadAmount = 4;
const int CollisionThreadAmount = 9;
const int OutputThreadAmount = 10;

// Keeps count of the messages
int MessageCounter = 0;
// Timer, used for an accurate time for message outputs
float Timer = 0.0f;
// Number of worker threads we are actually using.
int NumWorkers;

// Thread struct to keep track of the everything we need for them
struct WorkerThread
{
	std::thread             Thread;
	std::condition_variable WorkReady;
	std::mutex              Lock;
	bool					Complete;
};

// Array for the worker threads
WorkerThread WorkerThreads[MaxNumWorkers];

// Bool Array to keep track if a circle is static or not
bool StaticArray[CIRCLE_AMOUNT];
// Bool to keep track of the array output
std::string OutputStrings[1000];
// Array for the X,Y,Z Positions
float X_Positions[CIRCLE_AMOUNT];
float Y_Positions[CIRCLE_AMOUNT];
float Z_Positions[CIRCLE_AMOUNT];

// Used for the Quad tress, storages the position (top left corner in 0,1) and the dimensions(H/W)
int Quad_Tree_XY[13][4];

// This will storage the indexs of every circles that is in a quad tree (12 quad trees) and its connected quad tree (Will be better explained in the documentation)
int Combinded_Quad_Tree[13][1000];
// This will store the sizes of the quad tress and connected quad trees so we never have to wipe the array
int Combinded_Quad_Tree_Sizes[13];
// This will store the sizes of one quad tree (so 0 in quad zero, 1 is quad one etc.)
int Quad_Trees_ArrSizes[13];
// This will store the circles indexed for one quad tree
int Quad_Trees[13][1000];
// This will store the pairings for each circle, so index zero is in this will relate to a quad tree index so (slot 0 might be quad tree 11) 
int Quad_Trees_Pairing[CIRCLE_AMOUNT];

// So this helps with knowing what quad tree is connected to one another so we can combine them later.
int Quad_Tree_Square_Linking[13][6] = { { 0,1,5 },    {1,0,2,5},    {2, 1, 3, 6},    {3,2,4,7},     {4,3,7},
										  {5,0,1,6,8,9},			    {6,2,10,5,7},               {7,4,12,3,11,6},
											{ 8,5,9 },    {9,5,10,8},   {10, 9, 6, 11}, {11,10,12,7}, {12, 11, 7} };
// These are the sizes for the above
int Quad_Tree_Square_Linking_Sizes[13] = { 3, 4, 4, 4, 3,
											 6, 4, 6,
										  3, 4, 4, 4, 3 };

// Array to store all the circles so we can move them, this also keeps the indexes the same
IModel* CircleArray[CIRCLE_AMOUNT];
// Array to store all the names
std::string Circle_Names[CIRCLE_AMOUNT];
// Stores all the HPs
int Circle_HP[CIRCLE_AMOUNT];
// Stores all the Radius
int Circle_Radius[CIRCLE_AMOUNT];
// Stores all the Velocity
float Circle_Velocity[CIRCLE_AMOUNT][3];
// Stores all the RGB, (So this was never used but its says to have it in the brief so keep it here)
float Circle_RGB[CIRCLE_AMOUNT][3];

/*USEFUL FUNCTIONS*/
float Dot(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return (x1 * x2) + (y1 * y2) + (z1 * z2);
}

float Dot(float x1, float y1, float x2, float y2)
{
	return (x1 * x2) + (y1 * y2);
}

float LengthSq(float x1, float y1, float z1)
{
	return ((x1 * x1) + (y1 * y1) + (z1 * z1));
}

float Length(float x1, float y1, float z1)
{
	return sqrt((x1 * x1) + (y1 * y1) + (z1 * z1));
}

float Length(float x1, float y1)
{
	return sqrt((x1 * x1) + (y1 * y1));
}

float* Normalized(float x1, float y1, float z1, float* arr)
{
	float len = Length(x1, y1, z1);
	arr[0] = x1 / len;
	arr[1] = y1 / len;
	arr[2] = z1 / len;
	return arr;
}

float* Normalized(float x1, float y1, float* arr)
{
	float len = Length(x1, y1);
	arr[0] = x1 / len;
	arr[1] = y1 / len;
	return arr;
}
/*****************/

// Function used to setup the application
void SceneSetup(IMesh* mCircleMesh)
{
	int StaticCounter = 0;
	// Loop through all the circles
	for (int i = 0; i < CIRCLE_AMOUNT; i++)
	{
		// Set the names
		Circle_Names[i] = "Circle" + std::to_string(StaticCounter);
		// Set up the positions
		X_Positions[StaticCounter] = float(rand() % MAX_SPAWN_DIST + (MIN_SPAWN_DIST));
		Y_Positions[StaticCounter] = float(rand() % MAX_SPAWN_DIST + (MIN_SPAWN_DIST));
		Z_Positions[StaticCounter] = 0.0f;

		// Set up the RGB
		Circle_RGB[i][0] = { float(rand() % 255) };
		Circle_RGB[i][1] = { float(rand() % 255) };
		Circle_RGB[i][2] = { float(rand() % 255) };
		// Set up the radius (rand or not depending on boolean)
		int RandRadius;
		if (SPHERE_RAND_RADIUS)
		{
			RandRadius = rand() % 20 + 10;
			Circle_Radius[i] = RandRadius;
		}
		else
		{
			Circle_Radius[i] = 11;
		}
		/****************************************************/
		// Set up HP
		Circle_HP[i] = 100;
		// If its even then its static, if not then its moving
		if (StaticCounter % 2 == 0)
		{
			StaticArray[StaticCounter] = true;
			Circle_Velocity[i][0] = { 0 };
			Circle_Velocity[i][1] = { 0 };
			Circle_Velocity[i][2] = { 0 };
		}

		else
		{
			StaticArray[StaticCounter] = false;
			// Create a rand velocity for X and Y
			Circle_Velocity[i][0] = float(rand() % MAX_VELOCITY + (MIN_VELOCITY));
			Circle_Velocity[i][1] = float(rand() % MAX_VELOCITY + (MIN_VELOCITY));
			Circle_Velocity[i][2] = 0;
		}
		// If we can rendering then we can create the model
		if (RENDERING_DISPLAY)
		{
			CircleArray[i] = mCircleMesh->CreateModel(X_Positions[StaticCounter], Y_Positions[StaticCounter], Z_Positions[StaticCounter]);
		}

		++StaticCounter;
	}
}

// Used for reflecting the circles when the collide with the walls
void ReflectVector(int Index, float X, float Y, float Z)
{
	float FacingVectorX, FacingVectorY, FacingVectorZ;
	// Get the facing vectors
	FacingVectorX = X - X_Positions[Index];
	FacingVectorY = Y - Y_Positions[Index];
	FacingVectorZ = Z - Z_Positions[Index];
	// Create a normfacing vector
	float NormFacing[3];
	// Normlize the facing vectors and assign them to the normFacing
	Normalized(FacingVectorX, FacingVectorY, FacingVectorZ, NormFacing);
	// Get the dot product between velocity and the normfacing
	float DotProduct = Dot(Circle_Velocity[Index][0], Circle_Velocity[Index][1], Circle_Velocity[Index][2], NormFacing[0], NormFacing[1], NormFacing[2]);
	float dn = 2 * DotProduct;
	float NewVectorX, NewVectorY, NewVectorZ;
	NewVectorX = (NormFacing[0] * dn);
	NewVectorY = (NormFacing[1] * dn);
	NewVectorZ = (NormFacing[2] * dn);
	// Get the new veclocity and assign it
	NewVectorX = Circle_Velocity[Index][0] - NewVectorX;
	NewVectorY = Circle_Velocity[Index][1] - NewVectorY;
	NewVectorZ = Circle_Velocity[Index][2] - NewVectorZ;
	Circle_Velocity[Index][0] = NewVectorX;
	Circle_Velocity[Index][1] = NewVectorY;
	Circle_Velocity[Index][2] = NewVectorZ;
}

// Same as the above but this is for circle to circle reflections
void ReflectVector(int Index1, int Index2)
{
	float FacingVectorX, FacingVectorY, FacingVectorZ;
	FacingVectorX = X_Positions[Index2] - X_Positions[Index1];
	FacingVectorY = Y_Positions[Index2] - Y_Positions[Index1];
	FacingVectorZ = Z_Positions[Index2] - Z_Positions[Index1];
	float NormFacing[3];
	Normalized(FacingVectorX, FacingVectorY, FacingVectorZ, NormFacing);
	float DotProduct = Dot(Circle_Velocity[Index1][0], Circle_Velocity[Index1][1], Circle_Velocity[Index1][2], NormFacing[0], NormFacing[1], NormFacing[2]);
	float dn = 2 * DotProduct;
	float NewVectorX, NewVectorY, NewVectorZ;
	NewVectorX = (NormFacing[0] * dn);
	NewVectorY = (NormFacing[1] * dn);
	NewVectorZ = (NormFacing[2] * dn);
	NewVectorX = Circle_Velocity[Index1][0] - NewVectorX;
	NewVectorY = Circle_Velocity[Index1][1] - NewVectorY;
	NewVectorZ = Circle_Velocity[Index1][2] - NewVectorZ;
	Circle_Velocity[Index1][0] = NewVectorX;
	Circle_Velocity[Index1][1] = NewVectorY;
	Circle_Velocity[Index1][2] = NewVectorZ;
}

// Sphere to sphere collision
bool SphereToSphere(int Index1, int Index2)
{

	float distX = X_Positions[Index1] - X_Positions[Index2];
	float distY = Y_Positions[Index1] - Y_Positions[Index2];
	float distZ = Z_Positions[Index1] - Z_Positions[Index2];
	float distance = sqrt((distX * distX) + (distY * distY) + (distZ * distZ));
	if (distance <= Circle_Radius[Index1] + Circle_Radius[Index2]) {
		return true;
	}
	return false;

}
// Point to box collisions, is used to see if a circle is within a quad tree
bool PointToBox(int Index, int BoxX, int BoxY, int DimensionW, int DimensionH)
{
	// is the point inside the rectangle's bounds?
	float RightEdge = BoxX - DimensionW;
	float LefEdge = BoxY - DimensionH;
	if (X_Positions[Index] <= BoxX) {        // right of the left edge AND

		if (X_Positions[Index] >= RightEdge) {  // left of the right edge AND

			if (Y_Positions[Index] <= BoxY) {     // below the top AND

				if (Y_Positions[Index] >= LefEdge) {
					return true;
				}
			}
		}
	}
	return false;
}

// Loops through all the circles and displays the output strings to the console
void DisplayMessages(int StartIndex, int EndIndex)
{
	for (int i = StartIndex; i < EndIndex; i++)
	{
		if (OutputStrings[i] != "")
		{
			std::cout << OutputStrings[i] << std::endl;
		}
	}
}

void MoveSphere(float frameTime, int StartIndex, int EndIndex)
{
	// Loop though all the circles in the provided indexs
	for (int i = StartIndex; i < EndIndex; i++)
	{
		// if it isn't static
		if (!StaticArray[i])
		{
			// Create a new X,Y,Zs for the movement
			float NewX = Circle_Velocity[i][0] * frameTime;
			float NewY = Circle_Velocity[i][1] * frameTime;
			float NewZ = Circle_Velocity[i][2] * frameTime;
			// If we are rendering then we can move it and get the axis
			if (RENDERING_DISPLAY)
			{
				CircleArray[i]->Move(NewX, NewY, NewZ);
				X_Positions[i] = CircleArray[i]->GetX();
				Y_Positions[i] = CircleArray[i]->GetY();
				Z_Positions[i] = CircleArray[i]->GetZ();
			}
			// else then we want to assign the positions manual
			else
			{
				X_Positions[i] += NewX;
				Y_Positions[i] += NewY;
				Z_Positions[i] += NewZ;
			}
		}
	}
}
// This function sets up the quad trees (Diagram will be in the report)
void SetUpQuadTrees()
{

	int NewPosX = MAX_BORDER_X;
	int NewPosY = MAX_BORDER_Y;
	// Loop through the first 5
	for (int i = 0; i < 5; i++)
	{
		// If its even then its a normal sized quad
		if (i % 2 == 0)
		{
			// X,Y Pos
			Quad_Tree_XY[i][0] = NewPosX;
			Quad_Tree_XY[i][1] = NewPosY;
			// Height/Width
			Quad_Tree_XY[i][2] = 1400;
			Quad_Tree_XY[i][3] = 2500;
			//Need to setup the new poisition for next quad
			NewPosX = NewPosX - 1400;
		}
		// If its odd then we want a smaller quad tree to sit inbetween the quad trees
		else
		{
			// Same as the above but we want the width to be smaller
			Quad_Tree_XY[i][0] = NewPosX;
			Quad_Tree_XY[i][1] = NewPosY;
			Quad_Tree_XY[i][2] = 200;
			Quad_Tree_XY[i][3] = 2500;
			// Set up the new x pos
			NewPosX = NewPosX - 200;
		}
	}
	// Reset the alter the variables, we've done the first 5 by this point so we need to move down and do the next set
	NewPosX = MAX_BORDER_X;
	// Move down by 2500 as thats the height diemsion
	NewPosY = (MAX_BORDER_Y - 2500);
	// This will create 3 small quad threes to sit between the 2 large sets
	for (int i = 5; i < 8; i++)
	{
		// Want the X to be slightly bigger to cover the large and smaller quad threes above and below it
		Quad_Tree_XY[i][0] = NewPosX;
		Quad_Tree_XY[i][1] = NewPosY;
		Quad_Tree_XY[i][2] = 1600;
		Quad_Tree_XY[i][3] = 200;
		NewPosX = NewPosX - 1600;
	}
	// Reset the alter the variables, we've done the Second 3 by this point so we need to move down and do the next set
	NewPosX = MAX_BORDER_X;
	// Move down by 200 as thats the height diemsion
	NewPosY = (MAX_BORDER_Y - 200);
	// This loops is the same as the first one but on the bottom set of quad trees
	for (int i = 8; i < 13; i++)
	{
		if (i % 2 == 0)
		{
			Quad_Tree_XY[i][0] = NewPosX;
			Quad_Tree_XY[i][1] = NewPosY;
			Quad_Tree_XY[i][2] = 1400;
			Quad_Tree_XY[i][3] = 2500;
			NewPosX = NewPosX - 1400;
		}
		else
		{
			Quad_Tree_XY[i][0] = NewPosX;
			Quad_Tree_XY[i][1] = NewPosY;
			Quad_Tree_XY[i][2] = 200;
			Quad_Tree_XY[i][3] = 2500;
			NewPosX = NewPosX - 200;
		}
	}
}
// This function is used to assign circle indexs to a quad tree
void QuadTreeAssignment(int StartIndex, int EndIndex)
{
	// loop through the given indexs
	for (int i = StartIndex; i < EndIndex; i++)
	{
		// loop through all the quad trees
		for (int j = 0; j < 13; j++)
		{
			// Do point to box on the current quad tree
			if (PointToBox(i, Quad_Tree_XY[j][0], Quad_Tree_XY[j][1], Quad_Tree_XY[j][2], Quad_Tree_XY[j][3]))
			{
				// Assign the pairing the circle index
				Quad_Trees_Pairing[i] = j;
				// Assign the circle to the quad tree
				Quad_Trees[j][Quad_Trees_ArrSizes[j]] = i;
				Quad_Trees_ArrSizes[j]++;
				break;
			}
		}
	}
}

void ResloveSphereCollisions(int StartIndex, int EndIndex)
{
	// Loop through the given index
	for (int i = StartIndex; i < EndIndex; i++)
	{
		// Get the pairinf index
		int PairingIndex = Quad_Trees_Pairing[i];
		// Loop through all circles in that quad tree
		for (int j = 0; j < Combinded_Quad_Tree_Sizes[PairingIndex]; j++)
		{
			// if we check a circle and it doesn't share the same number then continue (Stops the self collision)
			if (i != Combinded_Quad_Tree[PairingIndex][j])
			{
				// Check to see if both circles are static, don't want to run collisions in the case of static vs static
				if (StaticArray[i] != true)
				{
					// If we are not allowing moving sphere collisions then we need to check if one/both aren't false as this show both are moving
					if (SPHERE_MOVING_COLLISIONS || StaticArray[i] != false || StaticArray[Combinded_Quad_Tree[PairingIndex][j]] != false)
					{
						// Check sphere to sphere collision
						if (SphereToSphere(i, Combinded_Quad_Tree[PairingIndex][j]))
						{
							// Remove HP
							Circle_HP[i] -= 20;

							// Very long output message show the names(static or not) and how much hp they have as well as the time hit.
							OutputStrings[MessageCounter] = Circle_Names[i] + "(" + to_string(StaticArray[i]) + ")" + " Has Hit " + Circle_Names[Combinded_Quad_Tree[PairingIndex][j]] + "(" + to_string(StaticArray[Combinded_Quad_Tree[PairingIndex][j]]) + ") \n" +
								Circle_Names[i] + "(" + to_string(Circle_HP[i]) + ")" + " / " + Circle_Names[Combinded_Quad_Tree[PairingIndex][j]] + "(" + to_string(Circle_HP[i]) + ") \n" +
								"TIMER: " + to_string(Timer) + "\n";
							// After update the message counter
							++MessageCounter;
							// We want to reflect the circles as there has been a collision
							ReflectVector(i, Combinded_Quad_Tree[PairingIndex][j]);
							// if the circle we have collided with is static then we want to remove the HP has it will never get past the sphere to sphere 
							if (StaticArray[Combinded_Quad_Tree[PairingIndex][j]] == true)
							{
								// Reduce the health and check if we are doing sphere death and the hp is below and equal to 0
								Circle_HP[Combinded_Quad_Tree[PairingIndex][j]] -= 20;
								if (SPHERE_DEATH && Circle_HP[Combinded_Quad_Tree[PairingIndex][j]] <= 0)
								{
									// Move to a location well away from the zone so the quad trees don't pick it up
									StaticArray[Combinded_Quad_Tree[PairingIndex][j]] = true;
									X_Positions[Combinded_Quad_Tree[PairingIndex][j]] = 9999;
									Y_Positions[Combinded_Quad_Tree[PairingIndex][j]] = 9999;
									// if we are rendering then just move the circle
									if (RENDERING_DISPLAY)
									{
										CircleArray[Combinded_Quad_Tree[PairingIndex][j]]->Move(X_Positions[i], Y_Positions[i], 0);
									}
								}
							}
							// if we are doing sphere death and hp is below 0 so report before but for the circle we are reflecting
							if (SPHERE_DEATH && Circle_HP[i] <= 0)
							{
								StaticArray[i] = true;
								X_Positions[i] = 9999;
								Y_Positions[i] = 9999;
								if (RENDERING_DISPLAY)
								{
									CircleArray[i]->Move(X_Positions[i], Y_Positions[i], 0);
								}
							}
							break;
						}
					}
				}
			}
		}
		// If we are having outer walls then check the selected poisitions against the max and reflect
		if (OUTER_WALLS)
		{
			if (X_Positions[i] > MAX_BORDER_X)
			{
				ReflectVector(i, MAX_BORDER_X, Y_Positions[i], Z_Positions[i]);
			}
			else if (X_Positions[i] < MIN_BORDER_X)
			{
				ReflectVector(i, MIN_BORDER_X, Y_Positions[i], Z_Positions[i]);
			}
			if (Y_Positions[i] > MAX_BORDER_Y)
			{
				ReflectVector(i, X_Positions[i], MAX_BORDER_Y, Z_Positions[i]);
			}
			else if (Y_Positions[i] < MIN_BORDER_Y)
			{
				ReflectVector(i, X_Positions[i], MIN_BORDER_Y, Z_Positions[i]);
			}
		}
	}
}

void CombineQuadSquares(int Index)
{
	// This function is used to get the circles in a quad and its connected quad tree and merge them into one
	int NewSize = 0;
	// Get the size of how many quad trees are linked for this index
	int Size = Quad_Tree_Square_Linking_Sizes[Index];
	// Loop through each quad tree given
	for (int i = 0; i < Size; i++)
	{
		// Get the current index provided
		int CurrentIndex = Quad_Tree_Square_Linking[Index][i];
		// Copy that array into this array
		std::copy(&Quad_Trees[CurrentIndex][0], &Quad_Trees[CurrentIndex][0] + Quad_Trees_ArrSizes[CurrentIndex], &Combinded_Quad_Tree[Index][NewSize]);
		// Add the size onto the variable
		NewSize += Quad_Trees_ArrSizes[CurrentIndex];
	}
	// Set the quad tee size to the new size
	Combinded_Quad_Tree_Sizes[Index] = NewSize;
}

// This function is used to setup the threads for the movement side the program
void MovementThread(uint32_t thread)
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].WorkReady.wait(l, [&]() { return !WorkerThreads[thread].Complete; });
		}
		// We have some work so do it...
		// Run the movesphere function to construct the thread to use that function
		MoveSphere(0, 0, 0);
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].Complete = true;
		}
		WorkerThreads[thread].WorkReady.notify_one();
	}
}

void CollisionThread(uint32_t thread)
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].WorkReady.wait(l, [&]() { return !WorkerThreads[thread].Complete; });
		}
		// We have some work so do it...
		// Run the ResloveSphereCollisions function to construct the thread to use that function
		ResloveSphereCollisions(0, 0);
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].Complete = true;
		}
		WorkerThreads[thread].WorkReady.notify_one();
	}
}

void QuadTreeThread(uint32_t thread)
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].WorkReady.wait(l, [&]() { return !WorkerThreads[thread].Complete; });
		}
		// We have some work so do it...
		// Run the QuadTreeAssignment function to construct the thread to use that function
		QuadTreeAssignment(0, 0);
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].Complete = true;
		}
		WorkerThreads[thread].WorkReady.notify_one();
	}
}


void OutputThread(uint32_t thread)
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].WorkReady.wait(l, [&]() { return !WorkerThreads[thread].Complete; });
		}
		// We have some work so do it...
		// Run the DisplayMessages function to construct the thread to use that function
		DisplayMessages(0, 0);
		{
			std::unique_lock<std::mutex> l(WorkerThreads[thread].Lock);
			WorkerThreads[thread].Complete = true;
		}
		WorkerThreads[thread].WorkReady.notify_one();
	}
}

void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine(kTLX); //kTLX
	if (RENDERING_DISPLAY)
	{
		myEngine->StartWindowed(1920, 1080);
		// Add default folder for meshes and other media
		myEngine->AddMediaFolder("C:\\ProgramData\\TL-Engine\\Media");
	}

	IMesh* circleMesh;
	/**** Set up your scene here ****/
	// if we are rendering then we want to load in the meshes and models
	if (RENDERING_DISPLAY)
	{
		circleMesh = myEngine->LoadMesh("Sphere.x");
		IMesh* skyBoxMesh = myEngine->LoadMesh("Skybox.x");

		IModel* skyBox = skyBoxMesh->CreateModel(2000.0f, 0.0f, 0.0f);

		skyBox->Scale(3.0f);
		ICamera* MyCamera;
		MyCamera = myEngine->CreateCamera(kFPS, 2000, 2000, -500);
		MyCamera->RotateY(0);
	}
	{
		NumWorkers = std::thread::hardware_concurrency(); // Gives a hint about level of thread concurrency supported by system (0 means no hint given)
		NumWorkers = 10;
		for (uint32_t i = 0; i < MovementThreadAmount; ++i)
		{
			// Start each worker thread running the MovementThread method
			WorkerThreads[i].Thread = std::thread(MovementThread, i);
		}
	}
	{
		for (uint32_t i = MovementThreadAmount; i < QuadTreeThreadAmount; ++i)
		{
			// Start each worker thread running the QuadTreeThread method
			WorkerThreads[i].Thread = std::thread(QuadTreeThread, i);
		}
	}
	{
		for (uint32_t i = QuadTreeThreadAmount; i < CollisionThreadAmount; ++i)
		{
			// Start each worker thread running the CollisionThread method
			WorkerThreads[i].Thread = std::thread(CollisionThread, i);
		}
	}
	{
		for (uint32_t i = CollisionThreadAmount; i < OutputThreadAmount; ++i)
		{
			// Start each worker thread running the OutputThread method
			WorkerThreads[i].Thread = std::thread(OutputThread, i);
		}
	}
	// Sets up the scene
	SceneSetup(circleMesh);
	// Sets up the quad trees
	SetUpQuadTrees();

	bool GameLoop = true;
	if (RENDERING_DISPLAY)
	{
		GameLoop = false;
	}
	float LastTick = 0; // Used for frametime when we aren't rendering
	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning() || GameLoop)
	{
		// Draw the scene
		/* This will get an accruate timer using ctime */
		clock_t TickCount = clock();
		Timer = float(TickCount) / CLOCKS_PER_SEC;
		/************************************************/
		float frameTime;
		// If we are rendering then draw the scene and get frame time
		if (RENDERING_DISPLAY)
		{
			myEngine->DrawScene();

			frameTime = myEngine->Timer();
		}
		// else we want to get a some what accurate frametime, I do this by messaure the tick time this frame and substracting from the last.
		else
		{
			clock_t TickCount = clock();
			float test = float(TickCount) / CLOCKS_PER_SEC;
			frameTime = test - LastTick;
			LastTick = test;
		}
		/**** Update your scene each frame here ****/
		// This splits the amount up so the threads can process it, most functions use 2 threads
		int Amount = CIRCLE_AMOUNT / 2;
		{
			for (int i = 0; i < MovementThreadAmount; i++)
			{
				// Flag the work as not yet complete
				{
					// Guard every access to shared variable "work.complete" with a mutex (see BlockSpritesThread comments)
					std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
					WorkerThreads[i].Complete = false;
				}

				// Notify the worker thread via a condition variable - this will wake the worker thread up
				WorkerThreads[i].WorkReady.notify_one();

				MoveSphere(frameTime, Amount * i, (i + 1) * Amount);
			}
			// Wait for all the workers to finish
			for (uint32_t i = 0; i < MovementThreadAmount; ++i)
			{
				// Wait for a signal via a condition variable indicating that the worker is complete
				// See comments in BlockSpritesThread regarding the mutex and the wait method
				std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
				WorkerThreads[i].WorkReady.wait(l, [&]() { return WorkerThreads[i].Complete; });
			}
		}
		{
			for (int i = MovementThreadAmount; i < QuadTreeThreadAmount; i++)
			{
				// Flag the work as not yet complete
				{
					// Guard every access to shared variable "work.complete" with a mutex (see BlockSpritesThread comments)
					std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
					WorkerThreads[i].Complete = false;
				}

				// Notify the worker thread via a condition variable - this will wake the worker thread up
				WorkerThreads[i].WorkReady.notify_one();
				int newAmount = i - MovementThreadAmount;
				QuadTreeAssignment(Amount * (newAmount), ((newAmount)+1) * Amount);
			}
			// Wait for all the workers to finish
			for (uint32_t i = MovementThreadAmount; i < QuadTreeThreadAmount; ++i)
			{
				// Wait for a signal via a condition variable indicating that the worker is complete
				// See comments in BlockSpritesThread regarding the mutex and the wait method
				std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
				WorkerThreads[i].WorkReady.wait(l, [&]() { return WorkerThreads[i].Complete; });
			}
		}
		// We want to loop through all the quad trees and combind certain ones togther
		for (int i = 0; i < 13; i++)
		{
			CombineQuadSquares(i);
		}
		// We use 5 threads for the collision threads as its the most taxing one
		Amount = CIRCLE_AMOUNT / 5;
		{
			for (int i = QuadTreeThreadAmount; i < CollisionThreadAmount; i++)
			{
				// Flag the work as not yet complete
				{
					// Guard every access to shared variable "work.complete" with a mutex (see BlockSpritesThread comments)
					std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
					WorkerThreads[i].Complete = false;
				}

				// Notify the worker thread via a condition variable - this will wake the worker thread up
				WorkerThreads[i].WorkReady.notify_one();

				int newAmount = i - QuadTreeThreadAmount;
				ResloveSphereCollisions(Amount * (newAmount), ((newAmount)+1) * Amount);
			}
			// Wait for all the workers to finish
			for (int i = QuadTreeThreadAmount; i < CollisionThreadAmount; i++)
			{
				// Wait for a signal via a condition variable indicating that the worker is complete
				// See comments in BlockSpritesThread regarding the mutex and the wait method
				std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
				WorkerThreads[i].WorkReady.wait(l, [&]() { return WorkerThreads[i].Complete; });
			}
		}
		{
			for (int i = CollisionThreadAmount; i < OutputThreadAmount; ++i)
			{
				// Flag the work as not yet complete
				{
					// Guard every access to shared variable "work.complete" with a mutex (see BlockSpritesThread comments)
					std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
					WorkerThreads[i].Complete = false;
				}

				// Notify the worker thread via a condition variable - this will wake the worker thread up
				WorkerThreads[i].WorkReady.notify_one();

				DisplayMessages(0, MessageCounter);
			}
			// Wait for all the workers to finish
			for (int i = CollisionThreadAmount; i < OutputThreadAmount; ++i)
			{
				// Wait for a signal via a condition variable indicating that the worker is complete
				// See comments in BlockSpritesThread regarding the mutex and the wait method
				std::unique_lock<std::mutex> l(WorkerThreads[i].Lock);
				WorkerThreads[i].WorkReady.wait(l, [&]() { return WorkerThreads[i].Complete; });
			}
		}
		// Reset the sizes
		for (int j = 0; j < 13; j++)
		{
			Quad_Trees_ArrSizes[j] = 0;
		}
		// reset the message counter
		MessageCounter = 0;
	}
	// Loop through and detach
	for (uint32_t i = 0; i < NumWorkers; ++i)
	{
		WorkerThreads[i].Thread.detach();
	}
	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}
