#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"

#define ROAD_SEGMENT_TYPE_COUNT 2

struct depth_line
{
	float Depth;
	float Scale;
};

struct road_segment
{
	float EndRelPX;
	float Position;
	
	float ddX;
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

void
DrawRoad(float PlayerP, float MaxDistance, float fScreenWidth, float fScreenHeight, depth_line *DepthLines, int DepthLineCount,
		 road_segment BottomSegment, road_segment NextSegment)
{
	float dX = 0.0f;
	float fCurrentCenterX     = fScreenWidth*0.5f + 0.5f; //NOTE(moritz): Road center line 
	float BaseRoadHalfWidth   = fScreenWidth*0.5f;
	float BaseStripeHalfWidth = 10.0f;
	
	float Offset = PlayerP;
	if(Offset > 8.0f)
		Offset -= 8.0f;
	
	road_segment CurrentSegment = BottomSegment;
	
	for(int DepthLineIndex = 0;
		DepthLineIndex < DepthLineCount;
		++DepthLineIndex)
	{
		float fDepthLineIndex = (float)DepthLineIndex;
		
		float fRoadWidth   = BaseRoadHalfWidth*DepthLines[DepthLineIndex].Scale;
		float fStripeWidth = BaseStripeHalfWidth*DepthLines[DepthLineIndex].Scale;
		
		if(fDepthLineIndex > NextSegment.Position)
			CurrentSegment = NextSegment;
		
		dX += CurrentSegment.ddX;
		fCurrentCenterX += dX;
		
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
		
		if(CurrentSegment.ddX > 0.0f)
			DrawLineV(RoadStart, RoadEnd, RED);
		else if(CurrentSegment.ddX < 0.0f)
			DrawLineV(RoadStart, RoadEnd, YELLOW);
		else
			DrawLineV(RoadStart, RoadEnd, BLUE);
		
		//DrawLineV(RoadStart, RoadEnd, RoadColor);
		
		//NOTE(moritz): Draw road stripes
		if(fmod(RoadWorldZ + 0.5f, 2.0f) > 1.0f) //TODO(moritz): 0.5f -> is stripe offset
		{
			Vector2 StripeStart = {fCurrentCenterX - fStripeWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			Vector2 StripeEnd   = {fCurrentCenterX + fStripeWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			DrawLineV(StripeStart, StripeEnd, WHITE);
		}
	}
}

void
DrawBillboard(Texture2D Texture, float Distance, float MaxDistance,
			  float Curviness, int ScreenWidth, int ScreenHeight, float fScreenHeight, bool DebugText = false)
{
	//Assert(Distance <= MaxDistance);
	
	//NOTE(moirtz): Test for "scaling in" distant billboards instead of popping them in...
	//float ScaleInDistance = 10.0f;
	float OneOverScaleInDistance = 0.1f;
	float ScaleInT = (MaxDistance - Distance)*OneOverScaleInDistance;
	ScaleInT = Clamp(0.0f, ScaleInT, 1.0f);
	
	if(DebugText)
		DrawText(TextFormat("TreeScaleInT: %f", ScaleInT), 20, 10, 10, RED);
	
	//float MaxDistance = (float)WorldState->DepthLineCount;
	float OneOverMaxDistance = 1.0f/MaxDistance;
	float BasePDepth = Distance*OneOverMaxDistance;
	
	//NOTE(moritz): Go through Depthlines (back to front) and find the first one with equal or smaller depth
	int BasePDepthLineIndex = -1;
	for(int DepthLineIndex = (WorldState->DepthLineCount - 1);
		DepthLineIndex >= 0;
		--DepthLineIndex)
	{
		if(WorldState->DepthLines[DepthLineIndex].Depth <= BasePDepth)
		{
			BasePDepthLineIndex = DepthLineIndex;
			break;
		}
	}
	
	//NOTE(moritz): Lerp the sprite scaling between the base depth line and the next closer one. 
	//Use depth to determine t.
	//TODO(moritz): Handle out of bounds!
	float Depth0 = WorldState->DepthLines[BasePDepthLineIndex].Depth;
	float Depth1 = WorldState->DepthLines[BasePDepthLineIndex + 1].Depth;
	
	float DeltaDepth = Depth1     - Depth0;
	float BasePDelta = BasePDepth - Depth0;
	
	float t = BasePDelta/DeltaDepth;
	
	//TODO(moritz): Handle out of bounds!
	float Scale0 = WorldState->DepthLines[BasePDepthLineIndex].Scale;
	float Scale1 = WorldState->DepthLines[BasePDepthLineIndex + 1].Scale;
	
	float DepthScale = Scale0 + t*(Scale1 - Scale0);
	
	if(DebugText)
		DrawText(TextFormat("TreeDepthScale: %f", DepthScale), 20, 20, 10, RED);
	
	//NOTE(moritz): Determine screen position of BaseP
	float BasePScreenY = fScreenHeight;
	if(BasePDepth != 0.0f)
		BasePScreenY = (float)WorldState->DepthLineCount + (WorldState->CameraHeight/BasePDepth);
	
	//NOTE(moritz):Clamp to screen coords to avoid some invalid memory access bug
	BasePScreenY = Clamp(0.0f, BasePScreenY, fScreenHeight);
	
	//NOTE(moritz): Where the sprite will be at the bottom of the Road
	//float BasePOffsetX = WorldState->BaseRoadHalfWidth + 300.0f + 0.5f*((float)Texture.width);
	float BasePOffsetX = 3.0f*WorldState->BaseRoadHalfWidth + 0.5f*((float)Texture.width);
	
	//NOTE(moritz): Some more lerping for the X part of BaseP. Taking into account curviness, angle of road and all that nonesense...
	float fDepthLineCount = (float)WorldState->DepthLineCount;
	float AngleOfRoad = WorldState->fRoadBaseOffsetX/fDepthLineCount;
	
	float X0 = WorldState->fCurveStartX + Curviness*( (float)(BasePDepthLineIndex*(BasePDepthLineIndex + 1))*0.5f ) + BasePOffsetX*DepthScale;
	float X1 = WorldState->fCurveStartX + Curviness*( (float)((BasePDepthLineIndex + 1)*(BasePDepthLineIndex + 1 + 1))*0.5f ) + BasePOffsetX*DepthScale;
	
	Vector2 BaseP = {};
	BaseP.x = X0 + t*(X1 - X0) - AngleOfRoad*(fDepthLineCount - BasePScreenY);
	BaseP.y = BasePScreenY;
	
#if 1
	Vector2 TestSize = {5.0f, 5.0f};
	DrawRectangleV(BaseP, TestSize, RED);
#endif
	
	//NOTE(moritz): Convert from BaseP to draw p... complying with raylib convention
	Vector2 SpriteDrawP = {};
	SpriteDrawP.x = BaseP.x - 0.5f*((float)Texture.width)*DepthScale*15.0f*ScaleInT;
	SpriteDrawP.y = BaseP.y - ((float)Texture.height)*DepthScale*15.0f*ScaleInT;
	
	DrawRectangleV(SpriteDrawP, TestSize, BLACK);
	
	
	DrawTextureEx(Texture, SpriteDrawP, 0.0f, DepthScale*15.0f*ScaleInT, WHITE);
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
	
	float fScreenCenterX = 0.5f*fScreenWidth + 0.5f;
	
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
	float PlayerSpeed = 20.0f;
	float PlayerP     = 0.0f;
	
	//---------------------------------------------------------
	road_segment NextSegment = {};
	NextSegment.Position = 50.0f;
	
	road_segment BottomSegment = {};
	
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
		
		//NOTE(moritz): Update segment position of NextSegment
		NextSegment.Position -= dPlayerP;
		
		if(NextSegment.Position < 0.0f)
		{
			BottomSegment = NextSegment;
			NextSegment.Position = MaxDistance;
			
			int Rand = (rand() % 2);
			
			if(Rand == 0)
				NextSegment.EndRelPX =  50.0f;
			else
				NextSegment.EndRelPX = -50.0f;
		}
		
		//NOTE(moritz): Update ddX?
		//TODO(moritz): Maybe only do this, when a new segment enters
		{
			float CurveStartX = fScreenCenterX;
			float CurveEndX   = fScreenCenterX + BottomSegment.EndRelPX;
			
			float DeltaDepthMapP = fDepthLineCount;
			
			BottomSegment.ddX = 2.0f*(CurveEndX - CurveStartX)/((DeltaDepthMapP*(DeltaDepthMapP + 1.0f)));
		}
		
		{
			float CurveStartX = 0.0f;
			float CurveEndX   = NextSegment.EndRelPX;
			
			float DeltaDepthMapP = fDepthLineCount;
			
			NextSegment.ddX = 2.0f*(CurveEndX - CurveStartX)/((DeltaDepthMapP*(DeltaDepthMapP + 1.0f)));
		}
		
		BeginDrawing();
		
		ClearBackground(WHITE);
		DrawFPS(10, 10);
		
		DrawRoad(PlayerP, MaxDistance, fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
				 BottomSegment, NextSegment);
		
		//---------------------------------------------------------
		
		EndDrawing();
	}
	
	CloseWindow();
	return(0);
}