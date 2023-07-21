
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"

#define global static

#define F32Max 3.402823e+38f
#define U32Max ((unsigned int) - 1)

#define Pi32 3.14159f

#define TWEAK_TABLE_SIZE 256


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

struct road_segment
{
	float ddX;
	float Position;
	
	road_segment *Next;
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

#define TWEAK(Value) TweakValue(Value, __LINE__)
//#define TWEAK(Value) Value

//NOTE(moritz): Source: https://gist.github.com/badboy/6267743 (Rober Jenkins 32 bit integer hash function)
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
		*SourceCode = LoadFileText("..\\code\\main.cpp");
	
	int SourceCodeByteCount = GetFileLength("..\\code\\main.cpp") + 1;
	
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
	
	int Foo = 0;
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
		 int DepthLineCount, road_list *ActiveRoadList/*Vector2 *FirstBezierLUT, Vector2 *SecondBezierLUT, float FirstLUTBaseY, float SecondLUTBaseY*/)
{
	float fDepthLineCount = (float)DepthLineCount;
	float BaseRoadHalfWidth = fScreenWidth*0.6f;
	float BaseStripeHalfWidth = 20.0f;
	
	float Offset = PlayerP;
	if(Offset > 8.0f)
		Offset -= 8.0f;
	
	road_segment *CurrentSegment = ActiveRoadList->First;
	float dX = 0.0f;
	float fCurrentCenterX = 0.5f*fScreenWidth;
	
	for(int DepthLineIndex = 0;
		DepthLineIndex < DepthLineCount;
		++DepthLineIndex)
	{
		float fLineY = (float)DepthLineIndex;
		
		float fRoadWidth   = BaseRoadHalfWidth*DepthLines[DepthLineIndex].Scale + 20.0f;
		float fStripeWidth = BaseStripeHalfWidth*DepthLines[DepthLineIndex].Scale;
		
		float fYLineNorm = fLineY/fDepthLineCount;
		
		if(CurrentSegment->Next)
		{
			if(fYLineNorm > CurrentSegment->Next->Position)
				CurrentSegment = CurrentSegment->Next;
		}
		
		dX += CurrentSegment->ddX;
		fCurrentCenterX += dX;
		
#if 0
		float fCurrentCenterX = F32Max;
		
		int IGNORED = -1;
		
		LUTLookUp(FirstBezierLUT, FirstLUTBaseY, fYLineNorm, &fCurrentCenterX, &IGNORED);
		
		if(fCurrentCenterX == F32Max)
			LUTLookUp(SecondBezierLUT, SecondLUTBaseY, fYLineNorm, &fCurrentCenterX, &IGNORED);
#endif
		
#if 0
		//NOTE(moritz): Depth based damping
		//Probably too much
		float PerspectiveCurveDamping = sinf( (DepthLines[DepthLineIndex].Depth + TWEAK(0.008f)) * 0.5f*Pi32);
		PerspectiveCurveDamping = Clamp(0.0f, PerspectiveCurveDamping, 1.0f);
		fCurrentCenterX = 0.5f*fScreenWidth + PerspectiveCurveDamping*fCurrentCenterX*0.5f*fScreenWidth;
#else
		//NOTE(moritz): Made up damping... Seems better
		//float CurveDamping = sinf(fYLineNorm*fYLineNorm*fYLineNorm*fYLineNorm*fYLineNorm*fYLineNorm*0.5f*Pi32);
		fYLineNorm -= 0.05f; //NOTE(moritz): This just some fuding for the damping
		float CurveDamping = sinf(fYLineNorm*fYLineNorm*fYLineNorm*fYLineNorm*fYLineNorm*0.5f*Pi32);
		CurveDamping = Clamp(0.0f, CurveDamping, 1.0f);
		//fCurrentCenterX = 0.5f*fScreenWidth + CurveDamping*fCurrentCenterX*0.5f*fScreenWidth;
		
		float CurrentCenterDelta = fCurrentCenterX - 0.5f*fScreenWidth;
		
		//TODO(moritz):Debug
		CurveDamping = 1.0f;
		
		fCurrentCenterX = 0.5f*fScreenWidth + CurveDamping*CurrentCenterDelta;//fCurrentCenterX*0.5f*fScreenWidth;
#endif
		
		float RoadWorldZ = DepthLines[DepthLineIndex].Depth*MaxDistance + Offset;
		
		Color GrassColor = GREEN;
		Color RoadColor  = DARKGRAY;
		
		if(fmod(RoadWorldZ, 8.0f) > 4.0f)
		{
			GrassColor = DARKGREEN;
			RoadColor  = GRAY;
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

inline void
InitSegment(road_segment *Segment, road_segment *PrevSegment, random_series *Entropy)
{
	if(PrevSegment)
		Segment->Position = PrevSegment->Position + 1.0f;//.2f + 0.5f*RandomUnilateral(Entropy); //NOTE(moritz): Float depth map position
	
	float Rand = RandomBilateral(Entropy)*0.005f;
	float SignRand = Sign(Rand);
	Segment->ddX      = 0.005f*SignRand + Rand;
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
	
	//NOTE(moritz): Load source code for tweak variables
	char *SourceCode = LoadFileText("..\\code\\main.cpp");
	int SourceCodeByteCount = GetFileLength("..\\code\\main.cpp") + 1;
	
	link_pool LinkPool = {};
	link_list LinkList = {};
	
	
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
	float PlayerSpeed = 0.0f;
	float PlayerP     = 0.0f;
	
	//---------------------------------------------------------
	
	random_series RoadEntropy = {420};
	road_pool RoadPool      = {};
	road_list ActiveRoadList = {};
	
	road_segment *InitialRoadSegment = AllocateRoadSegment(&RoadPool);
	
	InitialRoadSegment->Position = 0.0f;
	InitialRoadSegment->ddX      = RandomBilateral(&RoadEntropy)*0.005f;
	
	Append(&ActiveRoadList, InitialRoadSegment);
	
	//---------------------------------------------------------
	
	float TreeDistance = MaxDistance;
	//road_segment TreeSegment = GetSegmentAtDistance(TreeDistance, BottomSegment, NextSegment);
	
	//---------------------------------------------------------
	
	//TODO(moritz): Very gross D:
	bool FirstBaseWrapped = true;
	float FirstLUTBaseY  = 0.0f;
	bool SecondBaseWrapped = true;
	float SecondLUTBaseY = 1.0f;
	
	int PermInitialNormalIndex = -1;
	
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
		
		dtForFrame *= TWEAK(1.0f);
		
		PlayerSpeed = TWEAK(10.0f);
		
		//NOTE(moritz): Update player position
		float dPlayerP = PlayerSpeed*dtForFrame;
		//TODO(moritz): Floating point precision for PlayerP
		PlayerP += dPlayerP;
		
		//NOTE(moritz): Update active segments position
		float RoadDelta = TWEAK(0.0f)*dPlayerP/MaxDistance;
		//FirstLUTBaseY  -= RoadDelta;
		//SecondLUTBaseY -= RoadDelta;
		
		for(road_segment *Segment = ActiveRoadList.First;
			Segment;
			Segment = Segment->Next)
		{
			Segment->Position -= RoadDelta;
		}
		
		//NOTE(moritz): Road segment tweaking
		//ActiveRoadList.First->ddX = TWEAK(0.1f);
		
		
#if 1
		if(ActiveRoadList.Last->Position < 1.0f)
		{
			road_segment *NewSegment = AllocateRoadSegment(&RoadPool);
			
			InitSegment(NewSegment, ActiveRoadList.Last, &RoadEntropy);
			
			Append(&ActiveRoadList, NewSegment);
		}
		
		//TODO(moritz): Set new base segment and generate new segment
		//if((SecondLUTBaseY < 0.0f) && SecondBaseWrapped)
		if(ActiveRoadList.First->Next->Position <= 0.0f)
		{
			DeleteHeadSegment(&ActiveRoadList, &RoadPool);
			
			road_segment *NewSegment = AllocateRoadSegment(&RoadPool);
			
			InitSegment(NewSegment, ActiveRoadList.Last, &RoadEntropy);
			
			Append(&ActiveRoadList, NewSegment);
			
			FirstBaseWrapped = true;
		}
		
#if 0
		//TODO(moritz): If last segment < horizon. Add new segment...
		//if(ActiveRoadList.Last->End.y < 1.0f)
		if((FirstLUTBaseY < 0.0f) && FirstBaseWrapped)
		{
			FirstBaseWrapped = false;
			
			road_segment *NewSegment = AllocateRoadSegment(&RoadPool);
			
			InitSegment(NewSegment, &RoadEntropy);
			
			Append(&ActiveRoadList, NewSegment);
		}
#endif
		
#endif
		
		
		//NOTE(moritz): Update billboard positions
		TreeDistance -= dPlayerP;
		if((TreeDistance < 0.0f)/* && NewBottomSegment*/) //TODO(moritz): poor man's way of syncing tree spawn with segment swap to avoid weird bug
			TreeDistance = MaxDistance + 1.0f;
		
		BeginDrawing();
		
		ClearBackground(WHITE);
		DrawFPS(10, 10);
		
		
		
		DrawRoad(PlayerP, MaxDistance, fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
				 &ActiveRoadList/*,FirstBezierLUT, SecondBezierLUT, FirstLUTBaseY, SecondLUTBaseY*/);
		
		
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
			MarkerEnd.x = fScreenWidth;
			
			DrawLineEx(MarkerStart, MarkerEnd, 4.0f, ORANGE);
		}
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
		
		
		//---------------------------------------------------------
		ReloadSourceCode(&SourceCode, &LinkPool, &LinkList);
		UpdateTweakTable(&LinkList);
	}
	
	CloseWindow();
	return(0);
}