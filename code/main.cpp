#include <math.h>
#include <stdio.h>
#include "raylib.h"

#if 0
void
SetDepthLines()
{
	float MaxDepth = CameraHeight/
}
#endif

struct depth_line
{
	float Depth;
	float Scale;
};

void
ZeroSize(void *InitDest, int Size)
{
	char *Dest = (char  *)InitDest;
	
	while(Size--)
	{
		*Dest++ = 0;
	}
}

int
main()
{
	// Initialization
	//---------------------------------------------------------
	int ScreenWidth = 800;
	int ScreenHeight = 450;
	
	float fScreenWidth  = 800.0f;
	float fScreenHeight = 450.0f;
	
	bool AudioIsInitialised = false;
	
	InitWindow(ScreenWidth, ScreenHeight, "raylib");
	SetTargetFPS(60);
	
	//---------------------------------------------------------
	
	//NOTE(moritz): Depth related
	int DepthLineCount    = ScreenHeight/2;
	float fDepthLineCount = (float)DepthLineCount;
	float MaxDistance     = (float)DepthLineCount;
	float CameraHeight    = 1.0f;
	//NOTE(moritz): Init Depth map: http://www.extentofthejam.com/pseudo/
	depth_line *DepthLines = (depth_line *)malloc(sizeof(depth_line)*DepthLineCount);
	ZeroSize(DepthLines, sizeof(depth_line)*DepthLineCount);
	
	float MaxDepth = 1.0f;
	float MinDepth = CameraHeight/fDepthLineCount;
	
	for(int DepthLineIndex = 0;
		DepthLineIndex < DepthLineCount;
		++DepthLineIndex)
	{
		float fDepthLineIndex = (float)DepthLineIndex;
		
		DepthLines[DepthLineIndex].Depth = -CameraHeight/(fDepthLineIndex - fDepthLineCount);
		//NOTE(moritz): Normalising scaling to make things simpler
		DepthLines[DepthLineIndex].Scale = (1.0f/DepthLines[DepthLineIndex].Depth)*MinDepth; 
	}
	
	//---------------------------------------------------------
	
	//NOTE(moritz): Player
	float PlayerSpeed = 10.0f;
	float PlayerP     = 0.0f;
	
	//---------------------------------------------------------
	
	//NOTE(moritz): Main loop
	//TODO(moritz): Mind what is said about main loops for wasm apps...
	while(!WindowShouldClose())
	{
		//NOTE(moritz): Audio stuff has to get initialised like this,
		//Otherwise the browser (Chrome) complains... Audio init after user input
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			if(!AudioIsInitialised)
			{
				AudioIsInitialised = true;
				InitAudioDevice();
				
				//TODO(moritz): Load music and sfx here
			}
		}
		
		float dtForFrame = GetFrameTime();
		
		//NOTE(moritz): Update player position
		float dPlayerP = PlayerSpeed*dtForFrame;
		//TODO(moritz): Floating point precision for PlayerP
		PlayerP += dPlayerP;
		
		
		
		BeginDrawing();
		
		ClearBackground(WHITE);
		DrawFPS(10, 10);
		
		//NOTE(moritz): Draw road
		float fCurrentCenterX     = fScreenWidth*0.5f + 0.5f; //NOTE(moritz): Road center line 
		float BaseRoadHalfWidth   = fScreenWidth*0.5f;
		float BaseStripeHalfWidth = 10.0f;
		
		float Offset = PlayerP;
		if(Offset > 8.0f)
			Offset -= 8.0f;
		
		for(int DepthLineIndex = 0;
			DepthLineIndex < DepthLineCount;
			++DepthLineIndex)
		{
			float fDepthLineIndex = (float)DepthLineIndex;
			
			float fRoadWidth   = BaseRoadHalfWidth*DepthLines[DepthLineIndex].Scale;
			float fStripeWidth = BaseStripeHalfWidth*DepthLines[DepthLineIndex].Scale;
			
			float RoadWorldZ = DepthLines[DepthLineIndex].Depth*MaxDistance + Offset;
			
			Color GrassColor = GREEN;
			Color RoadColor  = DARKGRAY;
			
			if(fmod(RoadWorldZ, 8.0f) > 4.0f)
			{
				GrassColor = DARKGREEN;
				RoadColor  = GRAY;
			}
			
			//NOTE(moritz):Draw grass line first... draw road on top... 
			Vector2 GrassStart = {0.0f, fScreenHeight - fDepthLineIndex - 0.5f};
			Vector2 GrassEnd   = {fScreenWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			DrawLineV(GrassStart, GrassEnd, GrassColor);
			
			
			//NOTE(moritz): Draw road
			Vector2 RoadStart = {fCurrentCenterX - fRoadWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			Vector2 RoadEnd   = {fCurrentCenterX + fRoadWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			DrawLineV(RoadStart, RoadEnd, RoadColor);
			
			
			//NOTE(moritz): Draw road stripes
			if(fmod(RoadWorldZ + 0.5f, 2.0f) > 1.0f) //TODO(moritz): 0.5f -> is stripe offset
			{
				Vector2 StripeStart = {fCurrentCenterX - fStripeWidth, fScreenHeight - fDepthLineIndex - 0.5f};
				Vector2 StripeEnd   = {fCurrentCenterX + fStripeWidth, fScreenHeight - fDepthLineIndex - 0.5f};
				DrawLineV(StripeStart, StripeEnd, WHITE);
			}
		}
		
		//---------------------------------------------------------
		
		EndDrawing();
	}
	
	CloseWindow();
	return(0);
}