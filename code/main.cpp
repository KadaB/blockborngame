#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "rlgl.h"

#define global static

#define F32Max 3.402823e+38f
#define U32Max ((unsigned int) - 1)

#define Pi32 3.14159f

#define TWEAK_TABLE_SIZE 256

//NOTE(moritz): This macro only works if size of Array is known at compile time
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define CAR_TILT 15.f

#define WEB_BUILD

struct tweak_entry
{
	bool IsInitialised;
	float Value;
	unsigned int LineNumber;
};

global tweak_entry TweakTable[TWEAK_TABLE_SIZE];

struct depth_line
{
	float Depth;
	float Scale;
};

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
operator/(Vector2 A, float Float)
{
	Vector2 Result;
	
	Result.x = A.x/Float;
	Result.y = A.y/Float;
	
	return(Result);
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

inline Vector2 &
operator+=(Vector2 &A, Vector2 B)
{
	A = A + B;
	return(A);
}

inline float
Length(Vector2 A)
{
	float Result = sqrtf(A.x*A.x + A.y*A.y);
	return(Result);
}

inline Vector2
NOZ(Vector2 A)
{
	Vector2 Result = {};
	
	float Denom = Length(A);
	
	if(Denom != 0.0f)
		Result = A/Denom;
	
	return(Result);
}

inline float
Sign(float Float)
{
	if(Float < 0.0f)
		return -1.0f;
	else if(Float > 0.0f)
		return 1.0f;
	else
		return 0.0f;
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

Color
Lerp(Color A, float t, Color B)
{
	float Ar = (float)A.r;
	float Ag = (float)A.g;
	float Ab = (float)A.b;
	
	float Br = (float)B.r;
	float Bg = (float)B.g;
	float Bb = (float)B.b;
	
	float Rr = Lerp(Ar, t, Br);
	float Rg = Lerp(Ag, t, Bg);
	float Rb = Lerp(Ab, t, Bb);
	
	Color Result;
	Result.a = 255;
	
	Result.r = (unsigned char)(floorf(Rr + 1.0f));
	Result.g = (unsigned char)(floorf(Rg + 1.0f));
	Result.b = (unsigned char)(floorf(Rb + 1.0f));
	
	return(Result);
}

struct road_segment
{
	//float Length;
	float Position;
	float EndRelPX;
	
	float ddX;
	
	road_segment *Next;
};

float rand01() {
	return (float)rand() / (float)RAND_MAX;
}

//NOTE(moritz): ddX presets
float RoadPresets[] =
{
	1300.0f/(225.0f*226.0f),
	0.0f,
	-1300.0f/(225.0f*226.0f),
	0.0f,
	800.0f/(225.0f*226.0f),
	-800.0f/(225.0f*226.0f),
	/*2500.0f/(225.0f*226.0f)*/
};

struct road_pool
{
	road_segment *FirstFreeRoad;
	road_pool *Next;
};

struct road_list
{
	road_segment *First;
	road_segment *Last;
};

struct string
{
	int Count;
	char *Data;
};

struct link
{
	link *Next;
	string Line;
};

struct link_pool
{
	link *FirstFreeLink;
	link_pool *Next;
};

struct link_list
{
	link *First;
	link *Last;
};

link *
AllocateLink(link_pool *Pool)
{
	if(!Pool->FirstFreeLink)
	{
		int LinksPerPool = 4096;
		link *NewLinks = (link *)malloc(LinksPerPool*sizeof(link));
		if(NewLinks)
		{
			for(int LinkIndex = 0;
				LinkIndex < (LinksPerPool - 1);
				++LinkIndex)
			{
				NewLinks[LinkIndex].Next = &NewLinks[LinkIndex + 1];
			}
			
			NewLinks[LinksPerPool - 1].Next = 0;
		}
		else
		{
			//TODO(moritz): Error...
		}
		
		Pool->FirstFreeLink = NewLinks;
	}
	
	link *Result = Pool->FirstFreeLink;
	Pool->FirstFreeLink = Result->Next;
	
	Result->Next = 0;
	Result->Line.Count = 0;
	return(Result);
}

void
Append(link_list *List, link *Link)
{
	List->Last = (List->Last ? List->Last->Next : List->First) = Link;
}

#ifndef WEB_BUILD
#define TWEAK(Value) TweakValue(Value, __LINE__)
#else
#define TWEAK(Value) Value
#endif

//NOTE(moritz): Source: https://gist.github.com/badboy/6267743 (Robert Jenkins 32 bit integer hash function)
unsigned int
UIntHash( unsigned int a)
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

float
TweakValue(float Value, unsigned int LineNumber)
{
	unsigned int Index = UIntHash(LineNumber) % TWEAK_TABLE_SIZE;
	
	//NOTE(moritz): Linear probing (I guess). In case of collision
	while(TweakTable[Index].IsInitialised && 
		  (TweakTable[Index].LineNumber != LineNumber))
		Index = (Index + 1) % TWEAK_TABLE_SIZE;
	
	if(!TweakTable[Index].IsInitialised)
	{
		TweakTable[Index].Value = Value;
		TweakTable[Index].IsInitialised = true;
		TweakTable[Index].LineNumber = LineNumber;
	}
	
	return(TweakTable[Index].Value);
}

void
ReloadSourceCode(char **SourceCode, link_pool *LinkPool, link_list *LinkList)
{
	if(LinkList->First)
	{
		free(LinkList->First);
		LinkList->First = LinkList->Last = 0;
		LinkPool->FirstFreeLink = 0;
	}
	
	if(*SourceCode)
	{
		UnloadFileText(*SourceCode);
		*SourceCode = 0;
	}
	
	//TODO(moritz): Clear hashtable as well?
	
	while((*SourceCode) == 0)
		*SourceCode = LoadFileText("../code/main.cpp");
	
	int SourceCodeByteCount = GetFileLength("../code/main.cpp") + 1;
	
	//NOTE(moritz): Debug check
	unsigned int LineCount = 0;
	
	//NOTE(moritz):Split into lines for easier parsing
	int BOL = 0;
	for(int FileAt = 0;
		FileAt < SourceCodeByteCount;
		)
	{
		int C0 = (*SourceCode)[FileAt];
		int C1 = (*SourceCode)[FileAt + 1];
		int HitEOL = 0;
		int EOL = FileAt;
		
		if(((C0 == '\r') && (C1 == '\n')) ||
		   ((C0 == '\n') && (C1 == '\r')))
		{
			FileAt += 2;
			HitEOL = 1;
		}
		else if(C0 == '\n')
		{
			FileAt += 1;
			HitEOL = 1;
		}
		else if(C0 == '\r')
		{
			FileAt += 1;
			HitEOL = 1;
		}
		else if(C1 == 0) 
		{
			FileAt += 1;
			HitEOL = 1;
		}
		else
		{
			++FileAt;
		}
		
		if(HitEOL)
		{
			if(BOL <= EOL) //NOTE(moritz): Process empty lines as lines as well. To "guarantee" parity with C preprocessor
			{
				link *Link = AllocateLink(LinkPool); 
				
				Link->Line.Count = EOL - BOL;
				Link->Line.Data  = (*SourceCode) + BOL;
				
				Append(LinkList, Link);
				++LineCount;
			}
			
			BOL = FileAt;
		}
	}
	
}

float
SloppySteaksStringToFloat(string FloatString)
{
	float Result = F32Max;
	
	//Count digits before dot and after...
	unsigned int PreDotDigitCount  = 0;
	unsigned int PostDotDigitCount = 0;
	bool SawDot = false;
	bool IsNegative = (FloatString.Data[0] == '-');
	for(int CharIndex = 0;
		CharIndex < FloatString.Count;
		++CharIndex)
	{
		if(FloatString.Data[CharIndex] == '.')
		{
			SawDot = true;
		}
		else
		{
			if(!SawDot)
			{
				++PreDotDigitCount;
			}
			else
			{
				++PostDotDigitCount;
			}
		}
	}
	
	//Split into predot digits string and postdot digits string
	string PreDotDigitsString = FloatString;
	PreDotDigitsString.Count = PreDotDigitCount;
	if(IsNegative)
	{
		--PreDotDigitsString.Count;
		PreDotDigitsString.Data++;
	}
	
	string PostDotDigitsString = FloatString;
	PostDotDigitsString.Count = PostDotDigitCount;
	PostDotDigitsString.Data = FloatString.Data + FloatString.Count - PostDotDigitCount;
	
	//Read predot digits string right to left and multiply by 10^0, 10^1 ....
	char *DigitAt = PreDotDigitsString.Data + (PreDotDigitsString.Count - 1);
	float Base = 1;
	float PreDotResult = 0.0f;
	for(int PreDotDigitIndex = 0;
		PreDotDigitIndex < PreDotDigitsString.Count;
		++PreDotDigitIndex)
	{
		unsigned char Value = *DigitAt-- - '0';
		PreDotResult += Base*(float)Value;
		Base *= 10.0f;
	}
	
	//Read postdot digits string left to right and multiply by 10^-1, 10^-2 ...
	DigitAt = PostDotDigitsString.Data;
	Base = 1.0f/10.0f;
	float PostDotResult = 0.0f;
	for(int PostDotDigitIndex = 0;
		PostDotDigitIndex < PostDotDigitsString.Count;
		++PostDotDigitIndex)
	{
		unsigned char Value = *DigitAt++ - '0';
		PostDotResult += Base*(float)Value;
		Base *= (1.0f/10.0f);
	}
	
	Result = PreDotResult + PostDotResult;
	if(IsNegative)
	{
		Result *= -1.0f;
	}
	
	return(Result);
}

void
UpdateTweakTable(link_list *LinkList)
{
	for(int TableIndex = 0;
		TableIndex < TWEAK_TABLE_SIZE;
		++TableIndex)
	{
		if(TweakTable[TableIndex].IsInitialised)
		{
			int LineNumber = TweakTable[TableIndex].LineNumber;
			link *CurrentLink = LinkList->First;
			while(--LineNumber)
			{
				CurrentLink = CurrentLink->Next;
			}
			
			string LineCopy = CurrentLink->Line;
			
			//NOTE(moritz): Go to start of float literal
			while(
				  !((LineCopy.Data[0] == 'T') &&
					(LineCopy.Data[1] == 'W') &&
					(LineCopy.Data[2] == 'E') &&
					(LineCopy.Data[3] == 'A') &&
					(LineCopy.Data[4] == 'K') &&
					(LineCopy.Data[5] == '('))
				  )
			{
				++LineCopy.Data;
				--LineCopy.Count;
			}
			
			LineCopy.Data  += 6;
			LineCopy.Count -= 6;
			
			for(int CharIndex = 0;
				CharIndex < LineCopy.Count;
				++CharIndex)
			{
				if(LineCopy.Data[CharIndex] == 'f')
				{
					LineCopy.Count = CharIndex;
				}
			}
			
			float NewValue = SloppySteaksStringToFloat(LineCopy);
			
			TweakTable[TableIndex].Value = NewValue;
		}
	}
}

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
DeleteHeadSegment(road_list *ActiveRoadList, road_pool *Pool)
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
Append(road_list *ActiveRoadList, road_segment *Segment)
{
	ActiveRoadList->Last = (ActiveRoadList->Last ? ActiveRoadList->Last->Next : ActiveRoadList->First) = Segment;
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
DrawRoad(float PlayerP, float MaxDistance, float fScreenWidth, float fScreenHeight, depth_line *DepthLines,
		 int DepthLineCount, road_list *ActiveRoadList, float PlayerBaseXOffset)
{
	float fDepthLineCount = (float)DepthLineCount;
	float BaseRoadHalfWidth = fScreenWidth*0.6f;
	float BaseStripeHalfWidth = 20.0f;
	
	float AngleOfRoad = PlayerBaseXOffset/fDepthLineCount;
	
	float Offset = PlayerP;
	if(Offset > 8.0f)
		Offset -= 8.0f;
	
	road_segment *CurrentSegment = ActiveRoadList->First;
	float dX = 0.0f;
	float fCurrentCenterOffsetX = 0.0f;
	
	//int DepthSampleIndex = 0;
	//TODO(moritz): This can be modified for hills...
	//Horizon line needs to get adjusted as well
	float DepthSampleIndex = 0.0f;
	
	for(int DepthLineIndex = 0;
		DepthLineIndex < DepthLineCount;
		++DepthLineIndex, DepthSampleIndex += 1.0f)
	{
		float fLineY = (float)DepthLineIndex;
		
		float fRoadWidth   = BaseRoadHalfWidth*DepthLines[(int)DepthSampleIndex/*DepthLineIndex*/].Scale/* + 20.0f*/;
		float fStripeWidth = BaseStripeHalfWidth*DepthLines[(int)DepthSampleIndex/*DepthLineIndex*/].Scale;
		
		float fYLineNorm = fLineY/fDepthLineCount;
		
		if(CurrentSegment->Next)
		{
			if(fYLineNorm > CurrentSegment->Next->Position)
				CurrentSegment = CurrentSegment->Next;
		}
		
		dX += CurrentSegment->ddX;
		fCurrentCenterOffsetX += dX;
		
		//NOTE(moritz): Made up damping... Seems better
		float CurveDamping = sinf(fYLineNorm*0.5f*Pi32);
		CurveDamping = Clamp(0.0f, CurveDamping, 1.0f);
		fCurrentCenterOffsetX *= CurveDamping;
		
		float SteerOffset = AngleOfRoad*(fDepthLineCount - fLineY);
		
		float fCurrentCenterX = 0.5f*fScreenWidth + fCurrentCenterOffsetX + SteerOffset;
		
		float RoadWorldZ = DepthLines[(int)DepthSampleIndex/*DepthLineIndex*/].Depth*MaxDistance + Offset;
		
		Color GrassColor = { 44,  78, 154, 255};
		Color RoadColor  = { 15,   0,  30, 255};
		
		if(fmod(RoadWorldZ, 8.0f) > 4.0f)
		{
			GrassColor = BLANK;
			RoadColor  = { 30,  25,  40, 255};
		}
		
		//NOTE(moritz):Draw grass line first... draw road on top... 
		Vector2 GrassStart = {0.0f, fScreenHeight - fLineY - 0.5f};
		Vector2 GrassEnd   = {fScreenWidth, fScreenHeight - fLineY - 0.5f};
		DrawLineV(GrassStart, GrassEnd, GrassColor);
		
		//NOTE(moritz): Draw road
		Vector2 RoadStart = {fCurrentCenterX - fRoadWidth, fScreenHeight - fLineY - 0.5f};
		Vector2 RoadEnd   = {fCurrentCenterX + fRoadWidth, fScreenHeight - fLineY - 0.5f};
		
		//NOTE(moritz): Regular drawing
		DrawLineV(RoadStart, RoadEnd, RoadColor);
		
		//NOTE(moritz): Draw road stripes
		if(fmod(RoadWorldZ + 0.5f, 2.0f) > 1.0f) //TODO(moritz): 0.5f -> is stripe offset
		{
			Vector2 StripeStart = {fCurrentCenterX - fStripeWidth, fScreenHeight - fLineY - 0.5f};
			Vector2 StripeEnd   = {fCurrentCenterX + fStripeWidth, fScreenHeight - fLineY - 0.5f};
			DrawLineV(StripeStart, StripeEnd, {235, 183,   0, 255});
		}
	}
}

/*
NOTE(moritz):
Drawing billboards is kinda similiar to drawing the road itself, except for the first step.

1. Find the depth line the billboard is on based on its distance.

2. Do the accumulation up until that depth line to figure out where the road is and offset the billboard.
(in the future this could be a single pass for all of the billboards so that the accumulation doesn't
have to be done over and over again for each one)

3. Throw in various lerps to make the billboard spawn and move smoothly...

*/
#if 1
void
DrawBillboard(Texture2D Texture, float Distance, float MaxDistance,
			  float fScreenWidth, float fScreenHeight, depth_line *DepthLines, int DepthLineCount, 
			  float CameraHeight, road_list *ActiveRoadList, float PlayerBaseXOffset, bool DebugText = false)
{
	float fDepthLineCount = (float)DepthLineCount;
	float BaseRoadHalfWidth   = fScreenWidth*0.6f;
	float AngleOfRoad = PlayerBaseXOffset/fDepthLineCount;
	
	//NOTE(moirtz): Test for "scaling in" distant billboards instead of popping them in...
	//float ScaleInDistance = 10.0f;
	float OneOverScaleInDistance = 0.1f;
	float ScaleInT = (MaxDistance - Distance)*OneOverScaleInDistance;
	ScaleInT = Clamp(0.0f, ScaleInT, 1.0f);
	
	if(DebugText)
		DrawText(TextFormat("TreeScaleInT: %f", ScaleInT), 20, 20, 10, RED);
	
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
	
	if(BasePDepthLineIndex == -1)
		return;
	
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
	
	float DepthScale = Lerp(Scale0, t, Scale1);
	
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
	float BasePOffsetX = 0.5f*((float)Texture.width) + TWEAK(300.0f);
	float SpriteScale  = TWEAK(2.0f);
	float SpriteVerticalTweak = TWEAK(0.15f);
	
	//NOTE(moritz): Some more lerping for the X part of BaseP. Taking into account curviness, angle of road and all that nonesense...
	//float fDepthLineCount = (float)DepthLineCount;
	
	//TODO(moritz): Figure out which road segment we are in
	//Used closed formula to determine 
	
	float fBasePDepthLineIndex = (float)BasePDepthLineIndex;
	
	road_segment *CurrentSegment = ActiveRoadList->First;//BottomSegment;
	
	float dX = 0.0f;
	float fCurrentCenterOffsetX = 0.0f;
	
	float X0 = 0.0f;
	float X1 = 0.0f;
	
	int OnePastLastDepthLine = BasePDepthLineIndex + 1;
	
	for(int DepthLineIndex = 0;
		DepthLineIndex < OnePastLastDepthLine;
		++DepthLineIndex)
	{
		float fLineY = (float)DepthLineIndex;
		float fYLineNorm = fLineY/fDepthLineCount;
		
		float fRoadWidth   = BaseRoadHalfWidth*DepthLines[DepthLineIndex].Scale/* + 20.0f*/;
		float Blarg        = BasePOffsetX *DepthLines[DepthLineIndex].Scale;
		
		if(CurrentSegment->Next)
		{
			if(fYLineNorm > CurrentSegment->Next->Position)
				CurrentSegment = CurrentSegment->Next;
		}
		
		dX += CurrentSegment->ddX;
		fCurrentCenterOffsetX += dX;
		
		//NOTE(moritz): Made up damping... Seems better
		float CurveDamping = sinf(fYLineNorm*0.5f*Pi32);
		CurveDamping = Clamp(0.0f, CurveDamping, 1.0f);
		fCurrentCenterOffsetX *= CurveDamping;
		
		float SteerOffset = AngleOfRoad*(fDepthLineCount - fLineY);
		
		if(DepthLineIndex == (OnePastLastDepthLine - 2))
			X0 = 0.5f*fScreenWidth + fCurrentCenterOffsetX + SteerOffset + fRoadWidth + Blarg;
		else if(DepthLineIndex == (OnePastLastDepthLine - 1))
			X1 = 0.5f*fScreenWidth + fCurrentCenterOffsetX + SteerOffset + fRoadWidth + Blarg;
	}
	
	
	Vector2 TestSize = {5.0f, 5.0f};
	
	Vector2 BaseP = {};
	BaseP.x = Lerp(X0, t, X1) /*- AngleOfRoad*(fDepthLineCount - BasePScreenY)*/;
	BaseP.y = BasePScreenY;
	
	
	//NOTE(moritz): Convert from BaseP to draw p... complying with raylib convention
	Vector2 SpriteDrawP = {};
	SpriteDrawP.x = BaseP.x - 0.5f*((float)Texture.width)*DepthScale*SpriteScale*ScaleInT;
	SpriteDrawP.y = BaseP.y - ((float)Texture.height)*DepthScale*SpriteScale*ScaleInT + SpriteVerticalTweak*(float)Texture.height*DepthScale*SpriteScale*ScaleInT;
	
	DrawTextureEx(Texture, SpriteDrawP, 0.0f, DepthScale*SpriteScale*ScaleInT, WHITE);
	
	DrawRectangleV(BaseP, TestSize, RED);
	DrawRectangleV(SpriteDrawP, TestSize, BLACK);
}
#endif

inline void
InitSegment(road_segment *Segment, road_segment *PrevSegment, random_series *Entropy, float MaxDistance)
{
	if(PrevSegment)
		Segment->Position = PrevSegment->Position + 1.0f;//.2f + 0.5f*RandomUnilateral(Entropy); //NOTE(moritz): Float depth map position
	
	//float Rand = RandomBilateral(Entropy)*0.005f;
	//float SignRand = Sign(Rand);
	//Segment->ddX      = 0.005f*SignRand + Rand;
	
	//Segment->Length   = 25.0f + 80.0f*RandomUnilateral(Entropy);
	//Segment->EndRelPX = 250.0f*RandomBilateral(Entropy);
	
	//Segment->ddX      = 2.0f*(Segment->EndRelPX)/(MaxDistance*(MaxDistance + 1.0f));
	
	Segment->ddX = RoadPresets[XORShift32(Entropy) % ArrayCount(RoadPresets)];
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
	
	//TODO(moritz): Do we want to do this or do the d7Samura fatpixel filtering thing?
	//Does it only work for texture sampling? Could be adapted for drawing quads?
	//TODO(moritz): What about the timothy lottes CRT filter thing?
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(ScreenWidth, ScreenHeight, "raylib");
	SetTargetFPS(60);
	
	//---------------------------------------------------------
	
#ifndef WEB_BUILD
	//NOTE(moritz): Load source code for tweak variables
	char *SourceCode = LoadFileText("..\\code\\main.cpp");
	int SourceCodeByteCount = GetFileLength("..\\code\\main.cpp") + 1;
#endif
	
	link_pool LinkPool = {};
	link_list LinkList = {};
	
	
	//---------------------------------------------------------
	
	//NOTE(moritz): Post processing shader
	Shader LottesShader = LoadShader(0, "crt.fs");
	bool Foo = IsShaderReady(LottesShader);
	
	//---------------------------------------------------------
	
	//NOTE(moritz): Load textures
	Texture2D TreeTexture = LoadTexture("building.png");
	SetTextureFilter(TreeTexture, TEXTURE_FILTER_BILINEAR);
	
	Texture2D CarTexture = LoadTexture("player_car.png");
	SetTextureFilter(CarTexture, TEXTURE_FILTER_BILINEAR);
	
	Texture2D SunsetTexture = LoadTexture("sunset.png");
	SetTextureFilter(SunsetTexture, TEXTURE_FILTER_BILINEAR);
	
	Texture2D cross_hair_texture = LoadTexture("crosshair.png");
	SetTextureFilter(cross_hair_texture, TEXTURE_FILTER_BILINEAR);
	
	struct _FireAnimation {
		Vector2 position = {0.0f, 0.0f};
		float frame_duration = 0.1f;
		float runtime = 0.0f;
		float orientation = 0.0f;
		float scale = 2.0f;
		
		struct _Frame {
			Vector2 anchor;
			Texture2D texture;
		} frames[2] = { 
			{ {11, 1}, LoadTexture("fire0.png")}, 
			{ {22, 8}, LoadTexture("fire1.png")}
		};
		
		int calculate_current_frame() {
			return ((int)(runtime / frame_duration)) % 2;
		}
		
		// MARK1
		void draw(float delta_time, float scale_mult) {
			runtime += delta_time;
			_Frame current_frame = frames[calculate_current_frame()];
			const Vector2 &anchor = current_frame.anchor;
			
			// let the fire oscillate a bit
			float osc_scale_x = scale * scale_mult * (1+ (.3 * rand01()));
			float osc_scale_y = scale * scale_mult * (1+ (.4 * rand01()));
			rlPushMatrix();
			
			rlTranslatef(position.x, position.y, 0);
			rlRotatef(orientation, 0, 0, 1);
			rlScalef(osc_scale_x,osc_scale_y, 1.f);
			rlTranslatef(-anchor.x,- anchor.y, 0);
			
			DrawTexture(current_frame.texture, 0, 0, WHITE);
			
			rlPopMatrix();
		}
		
	};
	
	
	struct _Car {
		Vector2 position = {0, 0};
		float orientation = 0.f;
		float scale = 1.f;
		Texture2D texture = LoadTexture("car_plain.png");
		
		float runtime = 0.f;
		float lift_amount = 5;
		
		struct _shadow {
			Vector2 anchor = {72, -15};
			Texture2D shadow_tex = LoadTexture("car_shadow.png");
			
			void draw(float parent_orientation, float lift) {
				rlPushMatrix();
				rlRotatef(-parent_orientation, 0, 0, 1);
				rlTranslatef(-anchor.x,-anchor.y, 0);
				DrawTexture(shadow_tex, 0, -lift*1.5+5, WHITE);
				rlPopMatrix();
			}
		} shadow;
		
		Vector2 anchor = {75, 68};
		
		_FireAnimation fire_animation1 = {7, 72 };
		_FireAnimation fire_animation2 = {142, 72};
		
		void draw(float delta_time) {
			
			runtime += delta_time;
			
			float lift = lift_amount * sin(runtime*3);
			
			rlPushMatrix();
			
			rlTranslatef(position.x, position.y + lift, 0);
			rlScalef(scale, scale, 1.f);
			
			shadow.draw(orientation, lift);
			
			rlRotatef(orientation, 0, 0, 1);
			rlTranslatef(-anchor.x,- anchor.y, 0);
			
			float fire1_tmp_scale = Lerp(fire_animation1.scale,.5+ Clamp(-.5f, orientation / (CAR_TILT*2), .5),2.f);
			fire_animation1.orientation = 2*orientation;//Clamp(0, orientation, 15)*2.;
			fire_animation1.draw(delta_time, fire1_tmp_scale);
			
			float fire2_tmp_scale = Lerp(1.f, .5+Clamp(-.5f, orientation / (-CAR_TILT*2), .5),2.f);
			fire_animation2.orientation = 2*orientation;//Clamp(-15, orientation, 0)*2.;
			fire_animation2.draw(delta_time, fire2_tmp_scale);
			
			DrawTexture(texture, 0, 0, WHITE);
			rlPopMatrix();
		}
	} car;
	
	SetTextureWrap(car.fire_animation1.frames[0].texture, TEXTURE_WRAP_CLAMP);
	SetTextureWrap(car.fire_animation1.frames[1].texture, TEXTURE_WRAP_CLAMP);
	
	SetTextureWrap(car.fire_animation2.frames[0].texture, TEXTURE_WRAP_CLAMP);
	SetTextureWrap(car.fire_animation2.frames[1].texture, TEXTURE_WRAP_CLAMP);
	
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
	float PlayerSpeed = 0.0f;
	float PlayerP     = 0.0f;
	
	float PlayerBaseXOffset = 0.0f;
	
	//---------------------------------------------------------
	//NOTE(moritz): Background gradients
	Color SkyGradientCol0 = { 29,   9,  49, 255};
	Color SkyGradientCol1 = {198,  37,  32, 255};
	
	Color GrassGradientCol0 = { 51,   4, 104, 255};
	Color GrassGradientCol1 = { 38, 104, 143, 255};
	
	
	//---------------------------------------------------------
	
	random_series RoadEntropy = {420};
	road_pool RoadPool      = {};
	road_list ActiveRoadList = {};
	
	road_segment *InitialRoadSegment = AllocateRoadSegment(&RoadPool);
	
	InitialRoadSegment->Position = 0.0f;
	
	InitialRoadSegment->ddX = 0.0f;
	
	Append(&ActiveRoadList, InitialRoadSegment);
	
	//---------------------------------------------------------
	
	float TreeDistance = MaxDistance;
	//road_segment TreeSegment = GetSegmentAtDistance(TreeDistance, BottomSegment, NextSegment);
	
	float lenkVelocity = 0.f;

	float accumulatedVelocity = 0.f;
	float accumulatedVelocityDamping = .02f;
	
	//---------------------------------------------------------
	
	RenderTexture2D TargetTexture = LoadRenderTexture(ScreenWidth, ScreenHeight);
	
	//NOTE(moritz): Main loop
	//TODO(moritz): Mind what is said about main loops for wasm apps...
	while(!WindowShouldClose())
	{
		//NOTE(moritz): Audio stuff has to get initialised like this,
		//Otherwise the browser (Chrome) complains... Audio init after user input
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			//NOTE(moritz): For web builds, disable cursor has to be used
			if(!IsCursorHidden())
				DisableCursor();
			
			if(!AudioIsInitialised)
			{
				AudioIsInitialised = true;
				InitAudioDevice();
				
				//TODO(moritz): Load music and sfx here
			}
		}
		
		float dtForFrame = GetFrameTime();
		
		dtForFrame *= TWEAK(1.0f);
		
		//Max speed ~28.0f
		// PlayerSpeed = TWEAK(10.0f);
		
		float PlayerAcceleration = TWEAK(10.0f)*dtForFrame;
		
		if(IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
			PlayerSpeed += PlayerAcceleration;
			car.fire_animation1.scale = 1.1;
			car.fire_animation2.scale = 1.1;
		}
		else {
			car.fire_animation1.scale = .7;
			car.fire_animation2.scale = .7;
		}
		
		if(IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
			PlayerSpeed -= PlayerAcceleration;
			car.fire_animation1.scale = .2;
			car.fire_animation2.scale = .2;
		}
		
		if(PlayerSpeed <= 0.0f)
			PlayerSpeed = 0.0f;
		
		float SpeedDragForce = TWEAK(0.0125f)*PlayerSpeed*PlayerSpeed*dtForFrame;
		
		PlayerSpeed -= SpeedDragForce;
		
		if(fabs(PlayerSpeed) < TWEAK(0.15f))
			PlayerSpeed = 0.0f;
		
		//NOTE(moritz): Update player position
		float dPlayerP = PlayerSpeed*dtForFrame;
		//TODO(moritz): Floating point precision for PlayerP
		PlayerP += dPlayerP;
		
		float MaxSteerFactor = TWEAK(80.0f);
		float MinSteerFactor = TWEAK(30.0f);
		
		
		//NOTE(moritz): Max speed is currently like ~25.0f
		float SteerFactorT = (PlayerSpeed/28.0f);
		SteerFactorT = Clamp(0.0f, SteerFactorT, 1.0f);
		
		float MinSteering = TWEAK(10.0f);
		
		float SteerFactor = Lerp(MaxSteerFactor, SteerFactorT, MinSteerFactor);
		
		const float steer_speed = 200.f;
		const float max_steer = 50.f;
		float lenkAcceleration = steer_speed * dtForFrame;
		if(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
			// PlayerBaseXOffset += Max(dPlayerP*SteerFactor, MinSteering);
			car.orientation = -CAR_TILT;
			lenkVelocity = Max(lenkVelocity, 0) + lenkAcceleration;
		}
		else if(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
			// PlayerBaseXOffset -= Max(dPlayerP*SteerFactor, MinSteering);
			car.orientation = CAR_TILT;
			lenkVelocity = Min(lenkVelocity, 0) - lenkAcceleration;
		}
		else {
			lenkVelocity += -1*Sign(lenkVelocity)*lenkAcceleration ;
		}
		
		lenkVelocity = Clamp(-max_steer, lenkVelocity, max_steer);
		car.orientation = (-lenkVelocity / max_steer) * 20.f;
		float final_lenkVelocity = lenkVelocity*SteerFactor * dtForFrame;

		PlayerBaseXOffset += final_lenkVelocity;
		accumulatedVelocity += final_lenkVelocity*accumulatedVelocityDamping;
		
		//NOTE(moritz): Update active segments position
		float RoadDelta = TWEAK(1.0f)*dPlayerP/MaxDistance;
		
		for(road_segment *Segment = ActiveRoadList.First;
			Segment;
			Segment = Segment->Next)
		{
			Segment->Position -= RoadDelta;
		}
		
		//NOTE(moritz): Road segment tweaking
		//ActiveRoadList.First->ddX = TWEAK(0.1f);
#if 0
		ActiveRoadList.First->EndRelPX = TWEAK(1300.0f);
		ActiveRoadList.First->ddX      = 2.0f*(ActiveRoadList.First->EndRelPX)/(MaxDistance*(MaxDistance + 1.0f));
#endif
		
		
		if(ActiveRoadList.Last->Position < 1.0f)
		{
			road_segment *NewSegment = AllocateRoadSegment(&RoadPool);
			
			InitSegment(NewSegment, ActiveRoadList.Last, &RoadEntropy, MaxDistance);
			
			Append(&ActiveRoadList, NewSegment);
		}
		
		//NOTE(moritz): Set new base segment and generate new segment
		if(ActiveRoadList.First->Next->Position <= 0.0f)
		{
			DeleteHeadSegment(&ActiveRoadList, &RoadPool);
			
			road_segment *NewSegment = AllocateRoadSegment(&RoadPool);
			
			InitSegment(NewSegment, ActiveRoadList.Last, &RoadEntropy, MaxDistance);
			
			Append(&ActiveRoadList, NewSegment);
		}
		
		float CurveForceT = 1.0f - ActiveRoadList.First->Next->Position;;
		
		CurveForceT = Clamp(0.0f, CurveForceT, 1.0f);
		
		float CurveForce = PlayerSpeed*TWEAK(50.0f)*Lerp(ActiveRoadList.First->ddX, CurveForceT, ActiveRoadList.First->Next->ddX);
		
		PlayerBaseXOffset += CurveForce;
		
		float OffRoadLimit = TWEAK(1000.0f);
		PlayerBaseXOffset = Clamp(-OffRoadLimit, PlayerBaseXOffset, OffRoadLimit);
		
		//NOTE(moritz): Update billboard positions
		TreeDistance -= dPlayerP;
		if((TreeDistance < 0.0f)/* && NewBottomSegment*/) //TODO(moritz): poor man's way of syncing tree spawn with segment swap to avoid weird bug
			TreeDistance = MaxDistance + 1.0f;
		
		//BeginDrawing();
		BeginTextureMode(TargetTexture);
		
		ClearBackground(PINK);
		DrawRectangleGradientV(0, 0, ScreenWidth, ScreenHeight/2, SkyGradientCol0, SkyGradientCol1);
		DrawFPS(30, 10);
		
		//NOTE(moritz): Parallax background
		Vector2 SunsetP;
		SunsetP.x = accumulatedVelocity+ 0.5f*fScreenWidth - 0.5f*(float)SunsetTexture.width;
		SunsetP.y = fScreenHeight - (float)SunsetTexture.height - TWEAK(200.0f);
		DrawTextureEx(SunsetTexture, SunsetP, 0.0f, 1.0f, WHITE);
		
		DrawRectangleGradientV(0, ScreenHeight/2, ScreenWidth, ScreenHeight/2,
							   GrassGradientCol0, GrassGradientCol1);
		
		DrawRoad(PlayerP, MaxDistance, fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
				 &ActiveRoadList, PlayerBaseXOffset);
		
#if 1
		if(TreeDistance > 0.0f)
			DrawBillboard(TreeTexture, TreeDistance, MaxDistance,
						  fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
						  CameraHeight,  &ActiveRoadList, PlayerBaseXOffset);
#endif
		
		//NOTE(moritz): Draw player car
		Vector2 PlayerCarP = 
		{
			0.5f*fScreenWidth, // - 0.5f*(float)CarTexture.width,
			fScreenHeight - 60.0f // - (float)CarTexture.height
		};
		// DrawTextureEx(CarTexture, PlayerCarP, 0.0f, 1.0f, WHITE);
		car.position = PlayerCarP;
		car.draw(dtForFrame);
		
		
#if 0
		//NOTE(moritz): Visualise where the segments are at...
		for(road_segment *Segment = ActiveRoadList.First;
			Segment;
			Segment = Segment->Next)
		{
			float SegmentY = Segment->Position;
			
			Vector2 MarkerStart;
			MarkerStart.x = 0.0f;
			MarkerStart.y = fScreenHeight - SegmentY*MaxDistance;
			
			Vector2 MarkerEnd = MarkerStart;
			MarkerEnd.x       = fScreenWidth;
			
			Color LineColor = ORANGE;
			if(Segment == ActiveRoadList.First->Next)
				LineColor = Lerp(ORANGE, CurveForceT, RED);
			
			DrawLineEx(MarkerStart, MarkerEnd, 4.0f, LineColor);
		}
#endif
		
		// Draw the cross hair
		
		Vector2 crosshair_pos = {(float) GetMouseX(), (float) GetMouseY()};
		Vector2 crosshair_anchor = {cross_hair_texture.width / 2.f, cross_hair_texture.height / 2.f};
		DrawTextureEx(cross_hair_texture, crosshair_pos - crosshair_anchor , 0, 1, WHITE);
		
		//---------------------------------------------------------
		
		DrawText(TextFormat("PlayerSpeed: %f", PlayerSpeed), 20, 20, 10, RED);
		DrawText(TextFormat("SteerFactor: %f", SteerFactor), 20, 30, 10, RED);
		
		//---------------------------------------------------------
		
		//EndDrawing();
		EndTextureMode();
		
		BeginDrawing();
		
		ClearBackground(PINK);
		
		BeginShaderMode(LottesShader);
		
		DrawTextureRec(TargetTexture.texture, /*(Rectangle)*/{ 0, 0, (float)TargetTexture.texture.width, (float)-TargetTexture.texture.height }, /*(Vector2)*/{ 0, 0 }, WHITE);
		
		EndShaderMode();
		
		EndDrawing();
		
		//---------------------------------------------------------
#ifndef WEB_BUILD
		ReloadSourceCode(&SourceCode, &LinkPool, &LinkList);
		UpdateTweakTable(&LinkList);
#endif
	}
	
	CloseWindow();
	return(0);
}