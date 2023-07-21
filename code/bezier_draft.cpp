#define LUT_SIZE 32

float
Bezier2(float Start, float End, float Control, float t)
{
	float P0 = Lerp(Start,   t, Control);
	float P1 = Lerp(Control, t, End);
	
	float Result = Lerp(P0, t, P1);
	return(Result);
}

struct road_segment
{
	//Basis (3rd degree)
	Vector2 Start;
	Vector2 C0;
	Vector2 C1;
	Vector2 End;
	
	//1st derivative (2nd degree) - Velocity
	Vector2 VStart;
	Vector2 VC0;
	Vector2 VEnd;
	
	//2md derivative (1st degree) - Acceleration
	Vector2 AStart;
	Vector2 AEnd;
	
	road_segment *Next;
};

void
SetRoadSegmentDerivatives(road_segment *Segment)
{
	//1st derivative (2nd degree) - Velocity
	Segment->VStart = Segment->C0 - Segment->Start;
	Segment->VC0    = Segment->C1 - Segment->C0;
	Segment->VEnd   = Segment->End - Segment->C1;
	
	//2md derivative (1st degree) - Acceleration
	Segment->AStart = Segment->VC0 - Segment->VStart;
	Segment->AEnd   = Segment->VEnd - Segment->VC0;
}

Vector2
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
	
	return(P0112);
}

Vector2
Bezier3(road_segment RoadSegment, float t)
{
	return(Bezier3(RoadSegment.Start, RoadSegment.C0,
				   RoadSegment.C1, RoadSegment.End, t));
}

Vector2
Bezier3(road_segment *RoadSegment, float t)
{
	return(Bezier3(RoadSegment->Start, RoadSegment->C0,
				   RoadSegment->C1, RoadSegment->End, t));
}

void
DrawLUTs(Vector2 *FirstBezierLUT, Vector2 *SecondBezierLUT, 
		 Vector2 *FirstBezierLUTVel, Vector2 *SecondBezierLUTVel,
		 float FirstLUTBaseY, float SecondLUTBaseY, int PermInitialNormalIndex)
{
	for(int LUTIndex = 0;
		LUTIndex < (LUT_SIZE - 1);
		++LUTIndex)
	{
		Vector2 StartP = FirstBezierLUT[LUTIndex];
		Vector2 EndP   = FirstBezierLUT[LUTIndex + 1];
		
		StartP.x *= 400.0f;
		StartP.x += 400.0f;
		
		StartP.y += FirstLUTBaseY;
		
		StartP.y *= 225.0f;
		StartP.y  = 450.0f - StartP.y; 
		
		
		
		EndP.x *= 400.0f;
		EndP.x += 400.0f;
		
		EndP.y += FirstLUTBaseY;
		
		EndP.y *= 225.0f;
		EndP.y  = 450.0f - EndP.y; 
		
		//NOTE(moritz):Normal (for drawing, the y of the normal has to get flipped)
		Vector2 Normal = FirstBezierLUTVel[LUTIndex];
		Vector2 DrawNormal = Normal;
		DrawNormal.y *= -1.0f;
		Vector2 Normal0Start = Lerp(StartP, 0.5f, EndP);
		Vector2 Normal0End   = Normal0Start + DrawNormal*20.0f;
		
#if 0
		if(LUTIndex == PermInitialNormalIndex)
			DrawLineEx(Normal0Start, Normal0End, 4.0f, BROWN);
		else
			DrawLineEx(Normal0Start, Normal0End, 4.0f, PINK);
#endif
		
		if(Sign(Normal.x) == -1.0f)
			DrawLineEx(Normal0Start, Normal0End, 4.0f, BLACK);
		else if(Sign(Normal.x) == 1.0f)
			DrawLineEx(Normal0Start, Normal0End, 4.0f, PINK);
		
		DrawLineEx(StartP, EndP, 4.0f, RED);
	}
	
#if 1
	for(int LUTIndex = 0;
		LUTIndex < (LUT_SIZE - 1);
		++LUTIndex)
	{
		Vector2 StartP = SecondBezierLUT[LUTIndex];
		Vector2 EndP   = SecondBezierLUT[LUTIndex + 1];
		
		StartP.x *= 400.0f;
		StartP.x += 400.0f;
		
		StartP.y += SecondLUTBaseY;
		
		StartP.y *= 225.0f;
		StartP.y  = 450.0f - StartP.y; 
		
		
		
		EndP.x *= 400.0f;
		EndP.x += 400.0f;
		
		EndP.y += SecondLUTBaseY;
		
		EndP.y *= 225.0f;
		EndP.y  = 450.0f - EndP.y; 
		
		//NOTE(moritz):Normal (for drawing, the y of the normal has to get flipped)
		
		Vector2 Normal = SecondBezierLUTVel[LUTIndex];
		Vector2 DrawNormal = Normal;
		DrawNormal.y *= -1.0f;
		Vector2 Normal0Start = Lerp(StartP, 0.5f, EndP);
		Vector2 Normal0End   = Normal0Start + DrawNormal*20.0f;
		
#if 0
		if(LUTIndex == PermInitialNormalIndex)
			DrawLineEx(Normal0Start, Normal0End, 4.0f, BROWN);
		else
			DrawLineEx(Normal0Start, Normal0End, 4.0f, PINK);
#endif
		
		if(Sign(Normal.x) == -1.0f)
			DrawLineEx(Normal0Start, Normal0End, 4.0f, BLACK);
		else if(Sign(Normal.x) == 1.0f)
			DrawLineEx(Normal0Start, Normal0End, 4.0f, PINK);
		
		DrawLineEx(StartP, EndP, 4.0f, BLUE);
	}
#endif
}

void
DrawLUTIntersection(Vector2 *FirstBezierLUT, float FirstLUTBaseY, Vector2 *SecondBezierLUT, float SecondLUTBaseY)
{
	for(int YLineIndex = 0;
		YLineIndex < 255;
		++YLineIndex)
	{
		float fYLineNorm = (float)YLineIndex/255.0f;
		float fYLine = (float)YLineIndex + 0.5f;
		float X      = 0.0f;
		
		
		for(int LUTIndex = 1;
			LUTIndex < LUT_SIZE;
			++LUTIndex)
		{
			Vector2 LUTValue = FirstBezierLUT[LUTIndex];
			Vector2 PrevLUTValue = FirstBezierLUT[LUTIndex - 1];
			
			LUTValue.y     += FirstLUTBaseY;
			PrevLUTValue.y += FirstLUTBaseY;
			
			if(LUTValue.y >= fYLineNorm)
			{
				float LerpT = (fYLineNorm - PrevLUTValue.y)/(LUTValue.y - PrevLUTValue.y);
				
				X = (Lerp(PrevLUTValue.x, LerpT, LUTValue.x));
				
				break;
			}
		}
		
		if(X == 0.0f)
		{
			for(int LUTIndex = 1;
				LUTIndex < LUT_SIZE;
				++LUTIndex)
			{
				Vector2 LUTValue = SecondBezierLUT[LUTIndex];
				Vector2 PrevLUTValue = SecondBezierLUT[LUTIndex - 1];
				
				LUTValue.y     += SecondLUTBaseY;
				PrevLUTValue.y += SecondLUTBaseY;
				
				if(LUTValue.y >= fYLineNorm)
				{
					
					
					float LerpT = (fYLineNorm - PrevLUTValue.y)/(LUTValue.y - PrevLUTValue.y);
					
					X = (Lerp(PrevLUTValue.x, LerpT, LUTValue.x));
					
					break;
				}
			}
		}
		
		Vector2 StartP;
		StartP.x  = X;
		StartP.x *= 400.0f;
		StartP.x += 400.0f - 50.0f;
		
		StartP.y  = fYLineNorm;
		StartP.y *= 225.0f;
		StartP.y = 450.0f - StartP.y;
		
		
		Vector2 EndP;
		EndP.x = StartP.x + 100.0f;
		EndP.y = StartP.y;
		
		DrawLineV(StartP, EndP, GRAY);
		
	}
}

void
LUTLookUp(Vector2 *LUT, float LUTBaseYOffset, float FindY, float *XOut, int *NormalIndexOut)
{
	//NOTE(moritz): Old implementation
	for(int LUTIndex = 1;
		LUTIndex < LUT_SIZE;
		++LUTIndex)
	{
		Vector2 LUTValue     = LUT[LUTIndex];
		Vector2 PrevLUTValue = LUT[LUTIndex - 1];
		
		LUTValue.y     += LUTBaseYOffset;
		PrevLUTValue.y += LUTBaseYOffset;
		
		if(LUTValue.y >= FindY) //TODO(moritz): Should it be previous or next lut value?
		{
			float LerpT = (FindY - PrevLUTValue.y)/(LUTValue.y - PrevLUTValue.y);
			
			*XOut = Lerp(PrevLUTValue.x, LerpT, LUTValue.x);
			
			//*NormalIndexOut = LUTIndex - 1;
			if(LUTIndex <= 30)
				*NormalIndexOut = LUTIndex;
			
			return;
		}
	}
}

void
LUTNormal(Vector2 *LUT, float LUTBaseYOffset, float FindY, float *NormalXOut)
{
	for(int LUTIndex = 1;
		LUTIndex < LUT_SIZE;
		++LUTIndex)
	{
		Vector2 LUTValue     = LUT[LUTIndex];
		Vector2 PrevLUTValue = LUT[LUTIndex - 1];
		
		LUTValue.y     += LUTBaseYOffset;
		PrevLUTValue.y += LUTBaseYOffset;
		
		if(LUTValue.y >= FindY) //TODO(moritz): Should it be previous or next lut value?
		{
#if 0
			float LerpT = (FindY - PrevLUTValue.y)/(LUTValue.y - PrevLUTValue.y);
			
			*XOut = Lerp(PrevLUTValue.x, LerpT, LUTValue.x);
#endif
			
			Vector2 LUTNormal;
			LUTNormal.x = -(LUTValue.y - PrevLUTValue.y);
			LUTNormal.y = LUTValue.x - PrevLUTValue.x;
			
			//*NormalXOut += Sign(PrevLUTValue.x - LUTValue.x)*NOZ(LUTNormal).x;
			*NormalXOut += Sign(LUTValue.x - PrevLUTValue.x)*NOZ(LUTNormal).x;
			
			return;
		}
	}
}

void
SetLUT(Vector2 *LUT, Vector2 *LUTVel, road_segment *Segment, float tStep, float YOffset = 0.0f)
{
	float tLUT = 0.0f;
	for(int LUTIndex = 0;
		LUTIndex < LUT_SIZE;
		++LUTIndex)
	{
		LUT[LUTIndex] = Bezier3(Segment, tLUT);
		
		LUT[LUTIndex].y += YOffset;
		
		tLUT += tStep;
	}
	
	for(int LUTIndex = 0;
		LUTIndex < (LUT_SIZE - 1);
		++LUTIndex)
	{
		Vector2 LUTValue     = LUT[LUTIndex];
		Vector2 NextLUTValue = LUT[LUTIndex + 1];
		
		Vector2 LUTNormal;
		LUTNormal.x = -(NextLUTValue.y - LUTValue.y);
		LUTNormal.y = NextLUTValue.x - LUTValue.x;
		
		LUTVel[LUTIndex] = Sign(NextLUTValue.x - LUTValue.x)*NOZ(LUTNormal);
		//LUTVel[LUTIndex] = NOZ(LUTNormal);
		//LUTVel[LUTIndex].x *= Sign(NextLUTValue.x - LUTValue.x);
	}
}


{
	//NOTE(moritz): Startup
	Vector2 *FirstBezierLUT     = (Vector2 *)malloc(sizeof(Vector2)*LUT_SIZE);
	ZeroSize(FirstBezierLUT, sizeof(Vector2)*LUT_SIZE);
	Vector2 *FirstBezierLUTVel  = (Vector2 *)malloc(sizeof(Vector2)*LUT_SIZE);
	ZeroSize(FirstBezierLUTVel, sizeof(Vector2)*LUT_SIZE);
	Vector2 *FirstBezierLUTAcc  = (Vector2 *)malloc(sizeof(Vector2)*LUT_SIZE);
	ZeroSize(FirstBezierLUTAcc, sizeof(Vector2)*LUT_SIZE);
	
	Vector2 *SecondBezierLUT    = (Vector2 *)malloc(sizeof(Vector2)*LUT_SIZE);
	ZeroSize(SecondBezierLUT, sizeof(Vector2)*LUT_SIZE);
	Vector2 *SecondBezierLUTVel = (Vector2 *)malloc(sizeof(Vector2)*LUT_SIZE);
	ZeroSize(SecondBezierLUTVel, sizeof(Vector2)*LUT_SIZE);
	Vector2 *SecondBezierLUTAcc = (Vector2 *)malloc(sizeof(Vector2)*LUT_SIZE);
	ZeroSize(SecondBezierLUTAcc, sizeof(Vector2)*LUT_SIZE);
	
	float LUTtStep = 1.0f/(float)(LUT_SIZE - 1);
	
	SetLUT(FirstBezierLUT, FirstBezierLUTVel, ActiveRoadList.First, LUTtStep);
	
	//TODO(moritz): Very gross D:
	bool FirstBaseWrapped = true;
	float FirstLUTBaseY  = 0.0f;
	bool SecondBaseWrapped = true;
	float SecondLUTBaseY = 1.0f;
}



{
	//NOTE(moritz): Draw setup and debug vis
	float RoadDelta = TWEAK(1.0f)*dPlayerP/MaxDistance;
	FirstLUTBaseY  -= RoadDelta;
	SecondLUTBaseY -= RoadDelta;
	DrawRoad(PlayerP, MaxDistance, fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
			 FirstBezierLUT, SecondBezierLUT, FirstLUTBaseY, SecondLUTBaseY);
	DrawLUTs(FirstBezierLUT, SecondBezierLUT, FirstBezierLUTVel, SecondBezierLUTVel, FirstLUTBaseY, SecondLUTBaseY, PermInitialNormalIndex);
	
#if 0
	if(TreeDistance > 0.0f)
		DrawBillboard(TreeTexture, TreeDistance, MaxDistance,
					  fScreenWidth, fScreenHeight, DepthLines, DepthLineCount,
					  CameraHeight, 
					  BottomSegment, NextSegment, true);
#endif
	
	//NOTE(moritz): Draw curve force
	float CurveNormalX = 0.0f;
	
	//NOTE(moritz): Maybe average normals across region?
	for(int LineIndex = 165;
		LineIndex < 225;
		++LineIndex)
	{
		float fLineNorm = (float)LineIndex/255.0f;
		
		float IGNORED = 0.0f;
		int   NormalIndex = -1;
		
		Vector2 *NormalLUT = FirstBezierLUTVel;
		LUTLookUp(FirstBezierLUT, FirstLUTBaseY, fLineNorm, &IGNORED, &NormalIndex);
		
		//LUTNormal(FirstBezierLUT, FirstLUTBaseY, fLineNorm, &CurveNormalX);
		
		//if(CurveNormalX == F32Max)
		if(fLineNorm >= (1.0f + FirstLUTBaseY))
		{
			LUTLookUp(SecondBezierLUT, SecondLUTBaseY, fLineNorm, &IGNORED, &NormalIndex);
			NormalLUT = SecondBezierLUTVel;
		}
		
		if(NormalIndex >= 0)
			CurveNormalX += NormalLUT[NormalIndex].x;
	}
	
	
	CurveNormalX /= 60.0f;
	
	//NOTE(moritz): Single sample
	//LUTNormal(FirstBezierLUT, FirstLUTBaseY, 10.0f/255.0f, &CurveNormalX);
	
#if 0
	//NOTE(moritz): Perform two look ups for normal.. one line above, one line below
	//Normal at 10: LUT 9 and 11?
	float X0 = 0.0f;
	LUTLookUp(FirstBezierLUT, FirstLUTBaseY, 0.0f/255.0f, &X0);
	
	float X50 = 0.0f;
	LUTLookUp(FirstBezierLUT, FirstLUTBaseY, 50.0f/255.0f, &X50);
	
	Vector2 LUTNormal;
	LUTNormal.x = -(50.0f/255.0f);
	LUTNormal.y = X50 - X0;
	
	CurveNormalX = Sign(X50 - X0)*NOZ(LUTNormal).x;
#endif
	
	Vector2 NormalStart;
	NormalStart.x = 0.5f*fScreenWidth;
	NormalStart.y = fScreenHeight - 25.0f;
	
	Vector2 NormalEnd;
	NormalEnd.x = NormalStart.x + CurveNormalX*30.0f;
	NormalEnd.y = NormalStart.y;
	
	if(CurveNormalX < 0.0f)
		DrawLineEx(NormalStart, NormalEnd, 4.0f, BLACK);
	else if(CurveNormalX > 0.0f)
		DrawLineEx(NormalStart, NormalEnd, 4.0f, PURPLE);
	
	
#if 0
	//NOTE(moritz): Visualising curve feeling section
	Vector2 SectionStart;
	SectionStart.x = 0.5f;
	SectionStart.y = fScreenHeight - 165.0f;
	
	Vector2 SectionEnd;
	SectionEnd.x = 0.5f;
	SectionEnd.y = fScreenHeight - 225.0f;
	
	DrawLineEx(SectionStart, SectionEnd, 4.0f, YELLOW);
#endif
	
	//NOTE(moritz): Region after curve feeling section
	float FeelingSectionStartY = TWEAK(160.0f);
	int FeelingSectionStartIndex = (int)FeelingSectionStartY;
	
	Vector2 LowerSectionStart;
	LowerSectionStart.x = 0.0f;
	LowerSectionStart.y = fScreenHeight - FeelingSectionStartY;
	
	Vector2 LowerSectionEnd;
	LowerSectionEnd.x = fScreenWidth;
	LowerSectionEnd.y = fScreenHeight - FeelingSectionStartY;
	
	DrawLineEx(LowerSectionStart, LowerSectionEnd, 4.0f, YELLOW);
	
	//NOTE(moritz):Find Upper boundary based on next inflection point (use sign of normal.x to determine..)
	{
		float IGNORED = 0.0f;
		
		Vector2 *InitialNormalLUT = FirstBezierLUTVel;
		
		int InitialNormalIndex = -1;
		LUTLookUp(FirstBezierLUT, FirstLUTBaseY, (FeelingSectionStartY + 0.5f)/255.0f, &IGNORED, &InitialNormalIndex);
		
		if(InitialNormalIndex < 0)
		{
			LUTLookUp(SecondBezierLUT, SecondLUTBaseY, (FeelingSectionStartY + 0.5f)/255.0f, &IGNORED, &InitialNormalIndex);
			InitialNormalLUT = SecondBezierLUTVel;
		}
		
		PermInitialNormalIndex = InitialNormalIndex;
		
		float SectionSign = Sign(InitialNormalLUT[InitialNormalIndex].x);
		
		int SectionEnd = -1;
		
		for(int LineIndex = (FeelingSectionStartIndex + 1);
			LineIndex < 225;
			++LineIndex)
		{
			float fLineNorm = (float)LineIndex/225.0f;
			
			Vector2 *NormalLUT = FirstBezierLUTVel;
			int NormalIndex = -1;
			LUTLookUp(FirstBezierLUT, FirstLUTBaseY, fLineNorm, &IGNORED, &NormalIndex);
			
			if(NormalIndex < 0)
			{
				LUTLookUp(SecondBezierLUT, SecondLUTBaseY, fLineNorm, &IGNORED, &NormalIndex);
				NormalLUT = SecondBezierLUT;
			}
			
			float TestSign = Sign(NormalLUT[NormalIndex].x);
			
			if((TestSign != SectionSign))
			{
				SectionEnd = LineIndex - 1;
				break;
			}
		}
		
		if(SectionEnd == -1)
			SectionEnd = 224;
		
		float FeelingSectionEndY = (float)SectionEnd;
		
		Vector2 UpperSectionStart;
		UpperSectionStart.x = 0.0f;
		UpperSectionStart.y = fScreenHeight - FeelingSectionEndY;
		
		Vector2 UpperSectionEnd;
		UpperSectionEnd.x = fScreenWidth;
		UpperSectionEnd.y = fScreenHeight - FeelingSectionEndY;
		
		DrawLineEx(UpperSectionStart, UpperSectionEnd, 4.0f, ORANGE);
	}
}