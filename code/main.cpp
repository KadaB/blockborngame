#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"

#define ROAD_SEGMENT_TYPE_COUNT 2

#define U32Max ((unsigned int) - 1)

struct depth_line
{
	float Depth;
	float Scale;
};

struct road_segment
{
	float Length;
	
	Vector2 Start;
	Vector2 C0;
	Vector2 C1;
	Vector2 End;
	
	road_segment *Next;
};

struct road_pool
{
	road_segment *FirstFreeRoad;
	road_pool *Next;
};

struct list
{
	road_segment *First;
	road_segment *Last;
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

road_segment *
AllocateRoadSegment(road_pool *Pool)
{
	if(!Pool->FirstFreeRoad)
	{
		int RoadsPerPool = 32;
		road_segment *NewSegments = (road_segment *)malloc(RoadsPerPool*sizeof(road_segment));
		ZeroSize(NewSegments, RoadsPerPool*sizeof(road_segment));
		if(NewSegments)
		{
			for(int SegmentIndex = 0;
				SegmentIndex < (RoadsPerPool - 1);
				++SegmentIndex)
			{
				NewSegments[SegmentIndex].Next = &NewSegments[SegmentIndex + 1];
			}
			
			NewSegments[RoadsPerPool - 1].Next = 0;
		}
		
		Pool->FirstFreeRoad = NewSegments;
	}
	
	road_segment *Result = Pool->FirstFreeRoad;
	Pool->FirstFreeRoad = Result->Next;
	
	Result->Next = 0;
	return(Result);
}

road_segment *
DeleteHeadSegment(list *ActiveRoadList, road_pool *Pool)
{
	if(ActiveRoadList->First)
	{
		road_segment *NewHead = ActiveRoadList->First->Next;
		
		ActiveRoadList->First->Next = Pool->FirstFreeRoad;
		Pool->FirstFreeRoad = ActiveRoadList->First;
		
		ActiveRoadList->First = NewHead;
		
		return(ActiveRoadList->First);
	}
	
	return(0);
}

void
Append(list *ActiveRoadList, road_segment *Segment)
{
	ActiveRoadList->Last = (ActiveRoadList->Last ? ActiveRoadList->Last->Next : ActiveRoadList->First) = Segment;
}

inline Vector2
operator*(Vector2 A, float Float)
{
	Vector2 Result;
	
	Result.x = A.x*Float;
	Result.y = A.y*Float;
	
	return(Result);
}

inline Vector2
operator*(float Float, Vector2 A)
{
	return(A*Float);
}

inline Vector2
operator+(Vector2 A, Vector2 B)
{
	Vector2 Result;
	
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	
	return(Result);
}

inline Vector2
operator-(Vector2 A, Vector2 B)
{
	Vector2 Result;
	
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	
	return(Result);
}

float
Clamp(float Min, float Value, float Max)
{
	if(Value < Min)
		return Min;
	
	if(Value > Max)
		return Max;
	
	return Value;
}

float
Max(float A, float B)
{
	if(A > B)
		return A;
	
	return B;
}

float
Min(float A, float B)
{
	if(A < B)
		return A;
	
	return B;
}

float
Lerp(float A, float t, float B)
{
	float Result = (1.0f - t)*A + t*B;
	return(Result);
}

Vector2
Lerp(Vector2 A, float t, Vector2 B)
{
	Vector2 Result = (1.0f - t)*A + t*B;
	return(Result);
}

float
Bezier2(float Start, float End, float Control, float t)
{
	float P0 = Lerp(Start,   t, Control);
	float P1 = Lerp(Control, t, End);
	
	float Result = Lerp(P0, t, P1);
	return(Result);
}

//Vector2
float
Bezier3(Vector2 A, Vector2 B, Vector2 C, Vector2 D, float t)
{
	//NOTE(moritz): A: start, D: end
	//B: control0
	//C: control1
	
	Vector2 P0 = Lerp(A, t, B);
	Vector2 P1 = Lerp(B, t, C);
	Vector2 P2 = Lerp(C, t, D);
	
	Vector2 P01 = Lerp(P0, t, P1);
	Vector2 P12 = Lerp(P1, t, P2);
	
	Vector2 P0112 = Lerp(P01, t, P12);
	
	//TODO(moritz): ?
	//return(P0112);
	return(P0112.x);
}

//Vector2
float
Bezier3(road_segment RoadSegment, float t)
{
	return(Bezier3(RoadSegment.Start, RoadSegment.C0,
				   RoadSegment.C1, RoadSegment.End, t));
}

struct random_series
{
	unsigned int State;
};

unsigned int 
XORShift32(random_series *Series) 
{
	unsigned int X = Series->State;
	X ^= X << 13;
	X ^= X >> 17;
	X ^= X << 5;
	
	Series->State = X;
	
	return(X);
}

float
RandomUnilateral(random_series *Series)
{
	float Result = ((float)XORShift32(Series)/(float)U32Max);
	return(Result);
}

float
RandomBilateral(random_series *Series)
{
	float Result = -1.0f + 2.0f*(RandomUnilateral(Series));
	return(Result);
}

void
DrawRoad(float PlayerP, float MaxDistance, float fScreenWidth, float fScreenHeight, depth_line *DepthLines, int DepthLineCount,
		 list *ActiveRoadList, road_pool *Pool, random_series *RoadEntropy)
{
	float dX = 0.0f; //NOTE(moritz): Per line curve amount. ddX is per segment curve amount
	float fCurrentCenterX     = fScreenWidth*0.5f + 0.5f; //NOTE(moritz): Road center line 
	float BaseRoadHalfWidth   = fScreenWidth*0.5f;
	float BaseStripeHalfWidth = 10.0f;
	
	float fDepthLineCount = (float)DepthLineCount;
	
	float Offset = PlayerP;
	if(Offset > 8.0f)
		Offset -= 8.0f;
	
	road_segment *CurrentSegment = ActiveRoadList->First;
	float BezierTOffset = 0.0f;
	float fSegmentAt = 0.0f;
	
	if(CurrentSegment->Start.y < 0.0f)
		BezierTOffset = (float)fabs(CurrentSegment->Start.y);
	
	Color DebugColor = RED;
	
	for(int DepthLineIndex = 0;
		DepthLineIndex < DepthLineCount;
		++DepthLineIndex)
	{
		float fDepthLineIndex = (float)DepthLineIndex;
		fSegmentAt += 1.0f;
		
		float fRoadWidth   = BaseRoadHalfWidth*DepthLines[DepthLineIndex].Scale;
		float fStripeWidth = BaseStripeHalfWidth*DepthLines[DepthLineIndex].Scale;
		
		//NOTE(moritz): Basic bezier test
		//float BezierT = fDepthLineIndex/fDepthLineCount;
		float BezierT = fSegmentAt/CurrentSegment->Length;
		BezierT += BezierTOffset;
		
		//BezierT = Clamp(0.0f, BezierT, 1.0f);
		if(BezierT > 1.0f)
		{
			DebugColor = BLUE;
			
			fSegmentAt -= CurrentSegment->Length;
			//Get next segment..
			BezierT -= 1.0f;
			
			if(CurrentSegment->Next)
				CurrentSegment = CurrentSegment->Next;
			else
			{
				road_segment *NewSegment = AllocateRoadSegment(Pool);
				NewSegment->Length = MaxDistance;//30.0f + 50.0f*RandomUnilateral(RoadEntropy);
				
				NewSegment->Start = CurrentSegment->End;
				Vector2 Diff = NewSegment->Start - CurrentSegment->C1;
				NewSegment->C0 = NewSegment->Start + Diff;
				
				NewSegment->C1.x = .2f*RandomBilateral(RoadEntropy);
				NewSegment->C1.y = NewSegment->C0.y + 0.1f + RandomUnilateral(RoadEntropy);
				
				NewSegment->End.x = .2f*RandomBilateral(RoadEntropy);
				NewSegment->End.y = NewSegment->C1.y + 0.1f + RandomUnilateral(RoadEntropy);
				
				float OneOverYRange = 1.0f/(NewSegment->End.y - NewSegment->Start.y);
				float YOffset = NewSegment->Start.y;
				
				//NOTE(moritz): Need to renormalise by End.y
				NewSegment->Start.y -= YOffset;
				NewSegment->Start.y *= OneOverYRange;
				NewSegment->Start.y += YOffset;
				
				NewSegment->C0.y -= YOffset;
				NewSegment->C0.y *= OneOverYRange;
				NewSegment->C0.y += YOffset;
				
				NewSegment->C1.y -= YOffset;
				NewSegment->C1.y *= OneOverYRange;
				NewSegment->C1.y += YOffset;
				
				NewSegment->End.y -= YOffset;
				NewSegment->End.y *= OneOverYRange;
				NewSegment->End.y += YOffset;
				
				Append(ActiveRoadList, NewSegment);
				
				CurrentSegment = NewSegment;
			}
		}
		
		fCurrentCenterX = 0.5f*fScreenWidth + Bezier3(*CurrentSegment, BezierT)*0.5f*fScreenWidth;
		
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
		
#if 1
		//NOTE(moritz): Bottom segment / next segment vis
		DrawLineV(RoadStart, RoadEnd, DebugColor);
#endif
		
#if 0
		//NOTE(moritz): Curvature debug vis
		if(CurrentSegment.ddX > 0.0f)
			DrawLineV(RoadStart, RoadEnd, RED);
		else if(CurrentSegment.ddX < 0.0f)
			DrawLineV(RoadStart, RoadEnd, YELLOW);
		else
			DrawLineV(RoadStart, RoadEnd, BLUE);
#endif
		
#if 0
		//NOTE(moritz): Regular drawing
		DrawLineV(RoadStart, RoadEnd, RoadColor);
#endif
		
		//NOTE(moritz): Draw road stripes
		if(fmod(RoadWorldZ + 0.5f, 2.0f) > 1.0f) //TODO(moritz): 0.5f -> is stripe offset
		{
			Vector2 StripeStart = {fCurrentCenterX - fStripeWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			Vector2 StripeEnd   = {fCurrentCenterX + fStripeWidth, fScreenHeight - fDepthLineIndex - 0.5f};
			DrawLineV(StripeStart, StripeEnd, WHITE);
		}
	}
}

#if 0
void
DrawBillboard(Texture2D Texture, float Distance, float MaxDistance,
			  /*float Curviness, */float fScreenWidth, float fScreenHeight, depth_line *DepthLines,
			  int DepthLineCount, float CameraHeight, road_segment BottomSegment,
			  road_segment NextSegment, bool DebugText = false)
{
	//float dX = 0.0f;
	float fCurrentCenterX     = fScreenWidth*0.5f + 0.5f; //NOTE(moritz): Road center line 
	float BaseRoadHalfWidth   = fScreenWidth*0.5f;
	float BaseStripeHalfWidth = 10.0f;
	
	
	//NOTE(moirtz): Test for "scaling in" distant billboards instead of popping them in...
	//float ScaleInDistance = 10.0f;
	float OneOverScaleInDistance = 0.1f;
	float ScaleInT = (MaxDistance - Distance)*OneOverScaleInDistance;
	ScaleInT = Clamp(0.0f, ScaleInT, 1.0f);
	
	if(DebugText)
		DrawText(TextFormat("TreeScaleInT: %f", ScaleInT), 20, 20, 10, RED);
	
	//float MaxDistance = (float)DepthLineCount;
	float OneOverMaxDistance = 1.0f/MaxDistance;
	float BasePDepth = Distance*OneOverMaxDistance;
	
	//NOTE(moritz): Go through Depthlines (back to front) and find the first one with equal or smaller depth
	int BasePDepthLineIndex = -1;
	for(int DepthLineIndex = (DepthLineCount - 1);
		DepthLineIndex >= 0;
		--DepthLineIndex)
	{
		if(DepthLines[DepthLineIndex].Depth <= BasePDepth)
		{
			BasePDepthLineIndex = DepthLineIndex;
			break;
		}
	}
	
	//NOTE(moritz): Lerp the sprite scaling between the base depth line and the next closer one. 
	//Use depth to determine t.
	//TODO(moritz): Handle out of bounds!
	float Depth0 = DepthLines[BasePDepthLineIndex].Depth;
	float Depth1 = DepthLines[BasePDepthLineIndex + 1].Depth;
	
	float DeltaDepth = Depth1     - Depth0;
	float BasePDelta = BasePDepth - Depth0;
	
	float t = BasePDelta/DeltaDepth;
	
	//TODO(moritz): Handle out of bounds!
	float Scale0 = DepthLines[BasePDepthLineIndex].Scale;
	float Scale1 = DepthLines[BasePDepthLineIndex + 1].Scale;
	
	float DepthScale = Scale0 + t*(Scale1 - Scale0);
	
	if(DebugText)
		DrawText(TextFormat("TreeDepthScale: %f", DepthScale), 20, 30, 10, RED);
	
	//NOTE(moritz): Determine screen position of BaseP
	float BasePScreenY = fScreenHeight;
	if(BasePDepth != 0.0f)
		BasePScreenY = (float)DepthLineCount + (CameraHeight/BasePDepth);
	
	//NOTE(moritz):Clamp to screen coords to avoid some invalid memory access bug
	BasePScreenY = Clamp(0.0f, BasePScreenY, fScreenHeight);
	
	//NOTE(moritz): Where the sprite will be at the bottom of the Road
	//float BasePOffsetX = BaseRoadHalfWidth + 300.0f + 0.5f*((float)Texture.width);
	float BasePOffsetX = 3.0f*BaseRoadHalfWidth + 0.5f*((float)Texture.width);
	
	//NOTE(moritz): Some more lerping for the X part of BaseP. Taking into account curviness, angle of road and all that nonesense...
	float fDepthLineCount = (float)DepthLineCount;
	float AngleOfRoad = 1.0f;//fRoadBaseOffsetX/fDepthLineCount;
	
	//TODO(moritz): Figure out which road segment we are in
	//Used closed formula to determine 
	
	float fBasePDepthLineIndex = (float)BasePDepthLineIndex;
	
	road_segment CurrentSegment = BottomSegment;
	
	float fCurveStartX = fCurrentCenterX;
	
#if 0
	
	//if(Distance > NextSegment.Position)
	if(fBasePDepthLineIndex > NextSegment.Position)
	{
		CurrentSegment = NextSegment;
		
		//TODO(moritz): I suspect that this is wrong!
		//This should be more like
		fCurveStartX += BottomSegment.ddX*( (NextSegment.Position*(NextSegment.Position + 1.0f))*0.5f );
		//fCurveStartX  += BottomSegment.EndRelPX; 
		
		if(DebugText)
			DrawText("In Next segment!", 20, 40, 10, RED);
	}
	else
	{
		if(DebugText)
			DrawText("In Bottom segment!", 20, 40, 10, RED);
	}
#endif
	
	
	
	float BillboardCurveAt = Max(fBasePDepthLineIndex, CurrentSegment.Position) - Min(CurrentSegment.Position, fBasePDepthLineIndex);
	//float BillboardCurveAt = CurrentSegment.Position - fBasePDepthLineIndex;
	
	//float BillboardCurveAt = Distance - CurrentSegment.Position;
	
	if(DebugText)
		DrawText(TextFormat("CurveAt: %f", BillboardCurveAt), 20, 50, 10, RED);
	
#if 1
	float X0 = fCurveStartX + CurrentSegment.ddX*( (BillboardCurveAt*(BillboardCurveAt + 1.0f))*0.5f ) + BasePOffsetX*DepthScale;
	float X1 = fCurveStartX + CurrentSegment.ddX*( ((BillboardCurveAt + 1)*(BillboardCurveAt + 1.0f + 1.0f))*0.5f ) + BasePOffsetX*DepthScale;
#else
	float X0 = fCurveStartX + CurrentSegment.ddX*( (float)(BasePDepthLineIndex*(BasePDepthLineIndex + 1))*0.5f ) + BasePOffsetX*DepthScale;
	float X1 = fCurveStartX + CurrentSegment.ddX*( (float)((BasePDepthLineIndex + 1)*(BasePDepthLineIndex + 1 + 1))*0.5f ) + BasePOffsetX*DepthScale;
#endif
	
	Vector2 TestSize = {5.0f, 5.0f};
	
	Vector2 BaseP = {};
	BaseP.x = X0 + t*(X1 - X0) - AngleOfRoad*(fDepthLineCount - BasePScreenY);
	BaseP.y = BasePScreenY;
	
#if 1
	DrawRectangleV(BaseP, TestSize, RED);
#endif
	
	//NOTE(moritz): Convert from BaseP to draw p... complying with raylib convention
	Vector2 SpriteDrawP = {};
	SpriteDrawP.x = BaseP.x - 0.5f*((float)Texture.width)*DepthScale*15.0f*ScaleInT;
	SpriteDrawP.y = BaseP.y - ((float)Texture.height)*DepthScale*15.0f*ScaleInT;
	
	DrawRectangleV(SpriteDrawP, TestSize, BLACK);
	
	
	DrawTextureEx(Texture, SpriteDrawP, 0.0f, DepthScale*15.0f*ScaleInT, WHITE);
}
#endif


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
	
	//TODO(moritz): Do we want to do this or do the d7Samura fatpixel filtering thing?
	//Does it only work for texture sampling? Could be adapted for drawing quads?
	//TODO(moritz): What about the timothy lottes CRT filter thing?
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(ScreenWidth, ScreenHeight, "raylib");
	SetTargetFPS(60);
	
	//---------------------------------------------------------
	
	//NOTE(moritz): Load textures
	Texture2D TreeTexture = LoadTexture("tree.png");
	SetTextureFilter(TreeTexture, TEXTURE_FILTER_BILINEAR);
	
	/*
TODO(moritz): If we want to use mipmaps for our
billboards (I think we should), then it seems like
we can't just generate them at startup... This only
works for native builds of the game. Not for wasm builds...
So the mips need to get generated offline.
And then the game loads in the textures with mipmaps included.
*/
#if 0
	//NOTE(moritz): Load textures
	Texture2D TreeTexture = LoadTexture("tree.png");
	//SetTextureFilter(TreeTexture, TEXTURE_FILTER_BILINEAR);
	GenTextureMipmaps(&TreeTexture);
	SetTextureFilter(TreeTexture, TEXTURE_FILTER_TRILINEAR);
#endif
	
#if 0
	//NOTE(moritz): Alternative attempt at texture loading,
	//since GenTextureMipmaps seems to fail for wasm builds
	Image TreeImage = LoadImage("tree.png");
	ImageMipmaps(&TreeImage);
	Texture2D TreeTexture = LoadTextureFromImage(TreeImage);
	SetTextureFilter(TreeTexture, TEXTURE_FILTER_TRILINEAR);
	SetTextureWrap(TreeTexture, TEXTURE_WRAP_CLAMP);
#endif
	
	
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
	
	random_series RoadEntropy = {420};
	road_pool RoadPool      = {};
	list ActiveRoadList = {};
	
	road_segment *InitialRoadSegment = AllocateRoadSegment(&RoadPool);
	
	InitialRoadSegment->Length  = MaxDistance;//20.0f + RandomUnilateral(&RoadEntropy)*50.0f;
	InitialRoadSegment->Start.x = 0.0f;
	InitialRoadSegment->Start.y = 0.0f;
	InitialRoadSegment->C0.x    = .2f*RandomBilateral(&RoadEntropy);
	InitialRoadSegment->C0.y    = 0.1f + RandomUnilateral(&RoadEntropy);
	InitialRoadSegment->C1.x    = .2f*RandomBilateral(&RoadEntropy);
	InitialRoadSegment->C1.y    = InitialRoadSegment->C0.y + 0.1f + RandomUnilateral(&RoadEntropy);
	InitialRoadSegment->End.x   = .2f*RandomBilateral(&RoadEntropy);
	InitialRoadSegment->End.y   = InitialRoadSegment->C1.y + 0.1f + RandomUnilateral(&RoadEntropy);
	
	float OneOverEndY = 1.0f/InitialRoadSegment->End.y;
	
	//NOTE(moritz): Need to renormalise by End.y
	InitialRoadSegment->Start.y *= OneOverEndY;
	InitialRoadSegment->C0.y    *= OneOverEndY;
	InitialRoadSegment->C1.y    *= OneOverEndY;
	InitialRoadSegment->End.y   *= OneOverEndY;
	
	Append(&ActiveRoadList, InitialRoadSegment);
	
	//---------------------------------------------------------
	
	float TreeDistance = MaxDistance;
	//road_segment TreeSegment = GetSegmentAtDistance(TreeDistance, BottomSegment, NextSegment);
	
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
		
#if 1
		//NOTE(moritz): update road segment position
		for(road_segment *Segment = ActiveRoadList.First;
			Segment;
			Segment = Segment->Next)
		{
			float RoadDelta = dPlayerP/MaxDistance;
			//float RoadDelta = dPlayerP;
			
			RoadDelta *= 4.0f;
			
			Segment->Start.y -= RoadDelta;
			Segment->C0.y    -= RoadDelta;
			Segment->C1.y    -= RoadDelta;
			Segment->End.y   -= RoadDelta;
			
			if(Segment->End.y < 0.0f)
			{
				//Assert(Segment == ActiveRoadList.First):
				Segment = DeleteHeadSegment(&ActiveRoadList, &RoadPool);
			}
		}
#endif
		
		
		//NOTE(moritz): Update billboard positions
		TreeDistance -= dPlayerP;
		if((TreeDistance < 0.0f)/* && NewBottomSegment*/) //TODO(moritz): poor man's way of syncing tree spawn with segment swap to avoid weird bug
			TreeDistance = MaxDistance + 1.0f;
		
		BeginDrawing();
		
		ClearBackground(WHITE);
		//DrawFPS(10, 10);
		
		DrawRoad(PlayerP, MaxDistance, fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
				 /*BottomSegment, NextSegment*/&ActiveRoadList, &RoadPool, &RoadEntropy);
		
#if 0
		if(TreeDistance > 0.0f)
			DrawBillboard(TreeTexture, TreeDistance, MaxDistance,
						  fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
						  CameraHeight, 
						  BottomSegment, NextSegment, true);
#endif
		
		//NOTE(moritz): Debug stuff
		float TestX = fScreenWidth*0.5f + 0.5f;
		
#if 0
		float TestdX = 0.0f;
		road_segment CurrentSegment = BottomSegment;
		for(int DepthLineIndex = 0;
			DepthLineIndex < DepthLineCount;
			++DepthLineIndex)
		{
			float fDepthLineIndex = (float)DepthLineIndex;
			
			if(fDepthLineIndex > NextSegment.Position)
			{
				CurrentSegment = NextSegment;
			}
			
			TestdX += CurrentSegment.ddX;
			TestX += TestdX;
		}
		
		float OtherTestX = fScreenWidth*0.5f + 0.5f;
		
		OtherTestX += BottomSegment.ddX*( (NextSegment.Position*(NextSegment.Position + 1.0f))*0.5f );
		float TestXAt = fDepthLineCount - NextSegment.Position;
		OtherTestX += NextSegment.ddX*( (TestXAt*(TestXAt + 1.0f))*0.5f );
#endif
		
#if 0
		Vector2 TestSize = {5.0f, 5.0f};
		
		Vector2 TestP = {};
		TestP.x = TestX;
		TestP.y = fDepthLineCount;
		DrawRectangleV(TestP, TestSize, PINK);
		
		Vector2 OtherTestP = {};
		TestP.x = OtherTestX;
		TestP.y = fDepthLineCount;
		DrawRectangleV(TestP, TestSize, PURPLE);
		
#endif
		
		//---------------------------------------------------------
		
		EndDrawing();
	}
	
	CloseWindow();
	return(0);
}